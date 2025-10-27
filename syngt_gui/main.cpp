#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <d3d11.h>
#include <tchar.h>
#include <windows.h>
#include <commdlg.h>

#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/parser/Parser.h>
#include <syngt/transform/LeftElimination.h>
#include <syngt/transform/LeftFactorization.h>
#include <syngt/transform/RemoveUseless.h>
#include <syngt/transform/FirstFollow.h>
#include <syngt/analysis/ParsingTable.h>
#include <syngt/utils/UndoRedo.h>
#include <syngt/utils/Creator.h>
#include <syngt/graphics/DrawObject.h>

#include <fstream>
#include <sstream>
#include <memory>

// DirectX данные
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Данные приложения
static char grammarText[1024 * 64] = ""; // 64KB для грамматики
static char outputText[1024 * 64] = "";
static char currentFile[MAX_PATH] = "";
static std::unique_ptr<syngt::Grammar> grammar;
static syngt::UndoRedo undoRedo;
static std::unique_ptr<syngt::graphics::DrawObjectList> drawObjects;
static int activeNTIndex = 0;
static syngt::SelectionMask selectionMask;
static bool showAbout = false;
static bool showHelp = false;

// Вспомогательные функции
std::string LoadTextFile(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Cannot open file: ") + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void SaveTextFile(const char* filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Cannot save file: ") + filename);
    }
    file << content;
}

std::string OpenFileDialog() {
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "Grammar Files (*.grm)\0*.grm\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "grm";
    
    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
    return "";
}

std::string SaveFileDialog() {
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "Grammar Files (*.grm)\0*.grm\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = "grm";
    
    if (GetSaveFileNameA(&ofn)) {
        return std::string(filename);
    }
    return "";
}

void AppendOutput(const char* text) {
    size_t currentLen = strlen(outputText);
    size_t textLen = strlen(text);
    if (currentLen + textLen + 1 < sizeof(outputText)) {
        strcat_s(outputText, sizeof(outputText), text);
    }
}

void ClearOutput() {
    outputText[0] = '\0';
}

// Undo/Redo функции
void SaveCurrentState() {
    if (!grammar) return;
    
    auto nts = grammar->getNonTerminals();
    std::vector<std::string> names;
    std::vector<std::string> values;
    
    for (const auto& ntName : nts) {
        names.push_back(ntName);
        syngt::NTListItem* item = grammar->getNTItem(ntName);
        if (item) {
            values.push_back(item->value());
        } else {
            values.push_back("");
        }
    }
    
    undoRedo.addState(names, values, activeNTIndex, selectionMask);
}

void Undo() {
    if (!grammar || !undoRedo.canUndo()) {
        ClearOutput();
        AppendOutput("✗ Cannot undo!\n");
        return;
    }
    
    std::vector<std::string> names;
    std::vector<std::string> values;
    int index;
    syngt::SelectionMask selection;
    
    if (undoRedo.stepBack(names, values, index, selection)) {
        grammar->fillNew();
        
        for (size_t i = 0; i < names.size() && i < values.size(); ++i) {
            grammar->addNonTerminal(names[i]);
            grammar->setNTRule(names[i], values[i]);
        }
        
        activeNTIndex = index;
        selectionMask = selection;
        
        // Обновляем текстовое представление
        std::string tempFile = "temp_undo.grm";
        grammar->save(tempFile);
        std::string newText = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());
        
        if (newText.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newText.c_str());
        }
        
        ClearOutput();
        AppendOutput("✓ Undo successful\n");
    }
}

void Redo() {
    if (!grammar || !undoRedo.canRedo()) {
        ClearOutput();
        AppendOutput("✗ Cannot redo!\n");
        return;
    }
    
    std::vector<std::string> names;
    std::vector<std::string> values;
    int index;
    syngt::SelectionMask selection;
    
    if (undoRedo.stepForward(names, values, index, selection)) {
        grammar->fillNew();
        
        for (size_t i = 0; i < names.size() && i < values.size(); ++i) {
            grammar->addNonTerminal(names[i]);
            grammar->setNTRule(names[i], values[i]);
        }
        
        activeNTIndex = index;
        selectionMask = selection;
        
        // Обновляем текстовое представление
        std::string tempFile = "temp_redo.grm";
        grammar->save(tempFile);
        std::string newText = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());
        
        if (newText.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newText.c_str());
        }
        
        ClearOutput();
        AppendOutput("✓ Redo successful\n");
    }
}

// Визуализация диаграмм
void RenderDiagram(ImDrawList* drawList, const ImVec2& offset) {
    if (!drawObjects || drawObjects->count() == 0) return;
    
    const float scale = 1.0f;
    const ImU32 lineColor = IM_COL32(200, 200, 200, 255);
    const ImU32 textColor = IM_COL32(255, 255, 255, 255);
    
    // Рисуем все объекты
    for (int i = 0; i < drawObjects->count(); ++i) {
        syngt::graphics::DrawObject* obj = (*drawObjects)[i];
        if (!obj) continue;
        
        float x = offset.x + obj->x() * scale;
        float y = offset.y + obj->y() * scale;
        
        int type = obj->getType();
        
        // Рисуем в зависимости от типа
        if (type == syngt::graphics::ctDrawObjectTerminal) {
            // Терминал - овал
            float w = obj->getLength() * scale / 2.0f;
            float h = 20.0f * scale;
            drawList->AddEllipse(ImVec2(x + w, y), ImVec2(w, h), lineColor, 0, 0, 2.0f);
            
            // Текст
            auto leaf = dynamic_cast<syngt::graphics::DrawObjectLeaf*>(obj);
            if (leaf) {
                std::string name = leaf->name();
                ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
                drawList->AddText(ImVec2(x + w - textSize.x/2, y - textSize.y/2), textColor, name.c_str());
            }
        }
        else if (type == syngt::graphics::ctDrawObjectNonTerminal) {
            // Нетерминал - прямоугольник
            float w = obj->getLength() * scale;
            float h = 40.0f * scale;
            drawList->AddRect(ImVec2(x, y - h/2), ImVec2(x + w, y + h/2), lineColor, 0.0f, 0, 2.0f);
            
            auto leaf = dynamic_cast<syngt::graphics::DrawObjectLeaf*>(obj);
            if (leaf) {
                std::string name = leaf->name();
                ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
                drawList->AddText(ImVec2(x + w/2 - textSize.x/2, y - textSize.y/2), textColor, name.c_str());
            }
        }
        else if (type == syngt::graphics::ctDrawObjectFirst) {
            // Начало - треугольник
            drawList->AddTriangleFilled(
                ImVec2(x, y - 10),
                ImVec2(x, y + 10),
                ImVec2(x + 15, y),
                lineColor
            );
        }
        else if (type == syngt::graphics::ctDrawObjectLast) {
            // Конец - треугольник
            drawList->AddTriangleFilled(
                ImVec2(x + 15, y - 10),
                ImVec2(x + 15, y + 10),
                ImVec2(x, y),
                lineColor
            );
        }
        
        // Рисуем входящую стрелку
        syngt::graphics::Arrow* arrow = obj->inArrow();
        if (arrow && arrow->getFromDO()) {
            syngt::graphics::DrawObject* from = dynamic_cast<syngt::graphics::DrawObject*>(arrow->getFromDO());
            if (from) {
                float x1 = offset.x + from->endX() * scale;
                float y1 = offset.y + from->y() * scale;
                float x2 = x;
                float y2 = y;
                drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), lineColor, 2.0f);
            }
        }
    }
}

void UpdateDiagram() {
    if (!grammar) {
        drawObjects.reset();
        return;
    }
    
    // Получаем активный нетерминал
    auto nts = grammar->getNonTerminals();
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
        activeNTIndex = 0;
    }
    
    if (nts.empty()) {
        drawObjects.reset();
        return;
    }
    
    syngt::NTListItem* item = grammar->getNTItem(nts[activeNTIndex]);
    if (!item || !item->hasRoot()) {
        drawObjects.reset();
        return;
    }
    
    // Создаем визуализацию
    drawObjects = std::make_unique<syngt::graphics::DrawObjectList>(grammar.get());
    syngt::Creator::createDrawObjects(drawObjects.get(), item->root());
}

// Операции с грамматикой
void ParseGrammar() {
    try {
        ClearOutput();
        
        // Создаем временный файл для парсинга
        std::string tempFile = "temp_grammar.grm";
        SaveTextFile(tempFile.c_str(), grammarText);
        
        grammar = std::make_unique<syngt::Grammar>();
        grammar->load(tempFile);
        
        std::remove(tempFile.c_str());
        
        AppendOutput("✓ Grammar parsed successfully!\n\n");
        
        // Выводим статистику
        char stats[512];
        snprintf(stats, sizeof(stats), 
                 "Terminals: %d\nNon-terminals: %zu\nSemantics: %d\nMacros: %d\n",
                 grammar->terminals()->getCount(),
                 grammar->getNonTerminals().size(),
                 grammar->semantics()->getCount(),
                 grammar->macros()->getCount());
        AppendOutput(stats);
        
        // Сохраняем состояние для Undo/Redo
        SaveCurrentState();
        
        // Обновляем диаграмму
        UpdateDiagram();
        
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("✗ Parse Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
        grammar.reset();
        drawObjects.reset();
    }
}

void LoadFile() {
    std::string filename = OpenFileDialog();
    if (filename.empty()) return;
    
    try {
        ClearOutput();
        std::string content = LoadTextFile(filename.c_str());
        
        if (content.size() >= sizeof(grammarText)) {
            AppendOutput("✗ File too large!\n");
            return;
        }
        
        strcpy_s(grammarText, sizeof(grammarText), content.c_str());
        strcpy_s(currentFile, sizeof(currentFile), filename.c_str());
        
        AppendOutput("✓ File loaded: ");
        AppendOutput(filename.c_str());
        AppendOutput("\n");
        
        // Автоматически парсим
        ParseGrammar();
        
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("✗ Load Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void SaveFile() {
    std::string filename;
    
    if (currentFile[0] == '\0') {
        filename = SaveFileDialog();
        if (filename.empty()) return;
        strcpy_s(currentFile, sizeof(currentFile), filename.c_str());
    } else {
        filename = currentFile;
    }
    
    try {
        SaveTextFile(filename.c_str(), grammarText);
        ClearOutput();
        AppendOutput("✓ File saved: ");
        AppendOutput(filename.c_str());
        AppendOutput("\n");
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("✗ Save Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void SaveFileAs() {
    std::string filename = SaveFileDialog();
    if (filename.empty()) return;
    
    strcpy_s(currentFile, sizeof(currentFile), filename.c_str());
    SaveFile();
}

void EliminateLeftRecursion() {
    if (!grammar) {
        ClearOutput();
        AppendOutput("✗ Please parse grammar first!\n");
        return;
    }
    
    try {
        ClearOutput();
        syngt::LeftElimination::eliminate(grammar.get());
        
        // Сохраняем результат обратно в текст
        std::string tempFile = "temp_result.grm";
        grammar->save(tempFile);
        std::string newGrammar = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());
        
        if (newGrammar.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
        }
        
        AppendOutput("✓ Left recursion eliminated!\n");
        
        // Сохраняем состояние и обновляем диаграмму
        SaveCurrentState();
        UpdateDiagram();
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("✗ Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void LeftFactorize() {
    if (!grammar) {
        ClearOutput();
        AppendOutput("✗ Please parse grammar first!\n");
        return;
    }
    
    try {
        ClearOutput();
        syngt::LeftFactorization::factorizeAll(grammar.get());
        
        // Сохраняем результат
        std::string tempFile = "temp_result.grm";
        grammar->save(tempFile);
        std::string newGrammar = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());
        
        if (newGrammar.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
        }
        
        AppendOutput("✓ Left factorization completed!\n");
        
        SaveCurrentState();
        UpdateDiagram();
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("✗ Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void RemoveUselessSymbols() {
    if (!grammar) {
        ClearOutput();
        AppendOutput("✗ Please parse grammar first!\n");
        return;
    }
    
    try {
        ClearOutput();
        syngt::RemoveUseless::remove(grammar.get());
        
        // Сохраняем результат
        std::string tempFile = "temp_result.grm";
        grammar->save(tempFile);
        std::string newGrammar = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());
        
        if (newGrammar.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
        }
        
        AppendOutput("✓ Useless symbols removed!\n");
        
        SaveCurrentState();
        UpdateDiagram();
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("✗ Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void CheckLL1() {
    if (!grammar) {
        ClearOutput();
        AppendOutput("✗ Please parse grammar first!\n");
        return;
    }
    
    try {
        ClearOutput();
        
        // Вычисляем First и Follow
        auto firstSets = syngt::FirstFollow::computeFirst(grammar.get());
        auto followSets = syngt::FirstFollow::computeFollow(grammar.get(), firstSets);
        
        // Строим таблицу разбора
        auto table = syngt::ParsingTable::build(grammar.get());
        
        if (table->getConflicts().empty()) {
            AppendOutput("✓ Grammar IS LL(1)!\n\n");
        } else {
            AppendOutput("✗ Grammar IS NOT LL(1)!\n\n");
            AppendOutput("Conflicts:\n");
            for (const auto& conflict : table->getConflicts()) {
                AppendOutput("  ");
                AppendOutput(conflict.c_str());
                AppendOutput("\n");
            }
            AppendOutput("\n");
        }
        
        // Выводим First sets
        AppendOutput("FIRST sets:\n");
        for (const auto& [nt, firstSet] : firstSets) {
            char line[256];
            snprintf(line, sizeof(line), "  %s: { ", nt.c_str());
            AppendOutput(line);
            
            bool first = true;
            for (int termId : firstSet) {
                if (!first) AppendOutput(", ");
                if (termId == 0) {
                    AppendOutput("ε");
                } else {
                    AppendOutput(grammar->terminals()->getString(termId).c_str());
                }
                first = false;
            }
            AppendOutput(" }\n");
        }
        
        AppendOutput("\nFOLLOW sets:\n");
        for (const auto& [nt, followSet] : followSets) {
            char line[256];
            snprintf(line, sizeof(line), "  %s: { ", nt.c_str());
            AppendOutput(line);
            
            bool first = true;
            for (int termId : followSet) {
                if (!first) AppendOutput(", ");
                if (termId == -1) {
                    AppendOutput("$");
                } else {
                    AppendOutput(grammar->terminals()->getString(termId).c_str());
                }
                first = false;
            }
            AppendOutput(" }\n");
        }
        
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("✗ Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

// Main entry point
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

int main(int, char**)
{
    // Создаем окно
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), 
                       nullptr, nullptr, nullptr, nullptr, L"SynGT", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"SynGT - Syntax Grammar Transformation", 
                                 WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, 
                                 nullptr, nullptr, wc.hInstance, nullptr);

    // Инициализация Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = "syngt.ini"; // Сохраняем layout

    // Setup style
    ImGui::StyleColorsDark();
    
    // Настройка цветовой схемы
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Загружаем шрифт побольше для удобства
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\consola.ttf", 16.0f);
    
    // Пример грамматики по умолчанию
    const char* exampleGrammar = 
        "# Example grammar\n"
        "S : A, B.\n"
        "A : 'a' ; eps.\n"
        "B : 'b'.\n"
        "EOGram!\n";
    strcpy_s(grammarText, sizeof(grammarText), exampleGrammar);

    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open...", "Ctrl+O")) LoadFile();
                if (ImGui::MenuItem("Save", "Ctrl+S")) SaveFile();
                if (ImGui::MenuItem("Save As...")) SaveFileAs();
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) done = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, undoRedo.canUndo())) Undo();
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, undoRedo.canRedo())) Redo();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Grammar")) {
                if (ImGui::MenuItem("Parse", "F5")) ParseGrammar();
                ImGui::Separator();
                if (ImGui::MenuItem("Eliminate Left Recursion")) EliminateLeftRecursion();
                if (ImGui::MenuItem("Left Factorization")) LeftFactorize();
                if (ImGui::MenuItem("Remove Useless")) RemoveUselessSymbols();
                ImGui::Separator();
                if (ImGui::MenuItem("Check LL(1)")) CheckLL1();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Help")) showHelp = true;
                if (ImGui::MenuItem("About")) showAbout = true;
                ImGui::EndMenu();
            }
            
            // Показываем текущий файл
            ImGui::SameLine(io.DisplaySize.x - 400);
            if (currentFile[0] != '\0') {
                ImGui::Text("File: %s", currentFile);
            } else {
                ImGui::Text("No file loaded");
            }
            
            ImGui::EndMainMenuBar();
        }

        // Главное окно
        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - ImGui::GetFrameHeight()));
        ImGui::Begin("SynGT Main", nullptr, 
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

        // Splitter
        static float leftWidth = io.DisplaySize.x * 0.55f;
        
        // Левая панель - редактор грамматики и диаграмма
        ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, 0), true);
        
        // Вкладки
        if (ImGui::BeginTabBar("LeftTabs")) {
            // Вкладка редактора
            if (ImGui::BeginTabItem("Grammar Editor")) {
                ImGui::Text("Editor");
                ImGui::SameLine();
                if (ImGui::Button("Parse (F5)")) ParseGrammar();
                ImGui::SameLine();
                if (ImGui::Button("Clear")) { grammarText[0] = '\0'; ClearOutput(); }
                
                ImGui::Separator();
                
                ImGui::InputTextMultiline("##grammar", grammarText, sizeof(grammarText), 
                                           ImVec2(-FLT_MIN, -FLT_MIN),
                                           ImGuiInputTextFlags_AllowTabInput);
                
                ImGui::EndTabItem();
            }
            
            // Вкладка диаграммы
            if (ImGui::BeginTabItem("Syntax Diagram")) {
                // Список нетерминалов для выбора
                if (grammar && !grammar->getNonTerminals().empty()) {
                    auto nts = grammar->getNonTerminals();
                    
                    ImGui::Text("Select non-terminal:");
                    ImGui::SameLine();
                    
                    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
                        activeNTIndex = 0;
                    }
                    
                    if (ImGui::BeginCombo("##nt_select", nts[activeNTIndex].c_str())) {
                        for (int i = 0; i < static_cast<int>(nts.size()); ++i) {
                            bool is_selected = (activeNTIndex == i);
                            if (ImGui::Selectable(nts[i].c_str(), is_selected)) {
                                activeNTIndex = i;
                                UpdateDiagram();
                            }
                            if (is_selected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    
                    ImGui::Separator();
                    
                    // Область для рисования диаграммы
                    ImGui::BeginChild("DiagramCanvas", ImVec2(0, 0), false);
                    
                    if (drawObjects && drawObjects->count() > 0) {
                        ImDrawList* drawList = ImGui::GetWindowDrawList();
                        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
                        
                        // Белый фон
                        drawList->AddRectFilled(canvasPos, 
                                               ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                                               IM_COL32(30, 30, 35, 255));
                        
                        // Рисуем диаграмму
                        RenderDiagram(drawList, ImVec2(canvasPos.x + 20, canvasPos.y + 50));
                    } else {
                        ImGui::Text("No diagram to display. Parse grammar first.");
                    }
                    
                    ImGui::EndChild();
                } else {
                    ImGui::Text("Parse grammar to see diagrams");
                }
                
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::EndChild();

        ImGui::SameLine();

        // Правая панель - операции и вывод
        ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
        
        ImGui::Text("Operations");
        ImGui::Separator();
        
        ImGui::BeginGroup();
        if (ImGui::Button("Eliminate Left Recursion", ImVec2(200, 0))) {
            EliminateLeftRecursion();
        }
        if (ImGui::Button("Left Factorization", ImVec2(200, 0))) {
            LeftFactorize();
        }
        if (ImGui::Button("Remove Useless", ImVec2(200, 0))) {
            RemoveUselessSymbols();
        }
        if (ImGui::Button("Check LL(1)", ImVec2(200, 0))) {
            CheckLL1();
        }
        ImGui::EndGroup();
        
        ImGui::Separator();
        ImGui::Text("Output:");
        ImGui::Separator();
        
        ImGui::BeginChild("Output", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextWrapped("%s", outputText);
        ImGui::EndChild();
        
        ImGui::EndChild();

        ImGui::End();

        // About dialog
        if (showAbout) {
            ImGui::OpenPopup("About");
            if (ImGui::BeginPopupModal("About", &showAbout, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("SynGT - Syntax Grammar Transformation Tool");
                ImGui::Text("Version 1.0.0");
                ImGui::Separator();
                ImGui::Text("A tool for working with formal grammars and syntax diagrams.");
                ImGui::Text("Ported from Delphi to C++17 with Dear ImGui.");
                ImGui::Separator();
                if (ImGui::Button("Close")) showAbout = false;
                ImGui::EndPopup();
            }
        }

        // Help dialog
        if (showHelp) {
            ImGui::OpenPopup("Help");
            if (ImGui::BeginPopupModal("Help", &showHelp, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Grammar Syntax:");
                ImGui::BulletText("rule : definition.");
                ImGui::BulletText("'terminal' - terminal symbol");
                ImGui::BulletText(", - sequence (concatenation)");
                ImGui::BulletText("; - alternative (OR)");
                ImGui::BulletText("@* - iteration (zero or more)");
                ImGui::BulletText("@ - optional");
                ImGui::BulletText("EOGram! - end of grammar marker");
                ImGui::Separator();
                ImGui::Text("Keyboard Shortcuts:");
                ImGui::BulletText("Ctrl+O - Open file");
                ImGui::BulletText("Ctrl+S - Save file");
                ImGui::BulletText("F5 - Parse grammar");
                ImGui::Separator();
                if (ImGui::Button("Close")) showHelp = false;
                ImGui::EndPopup();
            }
        }

        // Rendering
        ImGui::Render();
        const float clear_color[4] = { 0.1f, 0.1f, 0.12f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// DirectX Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, 
                                                 featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, 
                                                 &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, 
                                           featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, 
                                           &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}