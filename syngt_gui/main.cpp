#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <d3d11.h>
#include <tchar.h>
#include <windows.h>
#include <commdlg.h>

#include <syngt/core/Grammar.h>
#include <syngt/parser/Parser.h>
#include <syngt/transform/LeftElimination.h>
#include <syngt/transform/LeftFactorization.h>
#include <syngt/transform/RemoveUseless.h>
#include <syngt/transform/FirstFollow.h>
#include <syngt/analysis/ParsingTable.h>

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

// Операции с грамматикой
void ParseGrammar() {
    try {
        ClearOutput();
        grammar = std::make_unique<syngt::Grammar>();
        
        syngt::Parser parser;
        parser.Parse(grammarText, *grammar);
        
        AppendOutput("✓ Grammar parsed successfully!\n\n");
        
        // Выводим статистику
        char stats[512];
        snprintf(stats, sizeof(stats), 
                 "Terminals: %zu\nNon-terminals: %zu\nSemantics: %zu\nMacros: %zu\n",
                 grammar->getTerminals().GetCount(),
                 grammar->getNonTerminals().GetCount(),
                 grammar->getSemantics().GetCount(),
                 grammar->getMacros().GetCount());
        AppendOutput(stats);
        
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("✗ Parse Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
        grammar.reset();
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
        syngt::LeftElimination eliminator;
        eliminator.EliminateLeftRecursion(*grammar);
        
        // Обновляем текст грамматики
        std::string newGrammar = grammar->ToString();
        if (newGrammar.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
        }
        
        AppendOutput("✓ Left recursion eliminated!\n");
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
        syngt::LeftFactorization factorizer;
        factorizer.Factorize(*grammar);
        
        // Обновляем текст грамматики
        std::string newGrammar = grammar->ToString();
        if (newGrammar.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
        }
        
        AppendOutput("✓ Left factorization completed!\n");
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
        syngt::RemoveUseless remover;
        remover.RemoveUseless(*grammar);
        
        // Обновляем текст грамматики
        std::string newGrammar = grammar->ToString();
        if (newGrammar.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
        }
        
        AppendOutput("✓ Useless symbols removed!\n");
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
        syngt::FirstFollow ff;
        auto firstSets = ff.ComputeFirst(*grammar);
        auto followSets = ff.ComputeFollow(*grammar);
        
        // Проверяем LL(1)
        syngt::ParsingTable table;
        bool isLL1 = table.IsLL1(*grammar, firstSets, followSets);
        
        if (isLL1) {
            AppendOutput("✓ Grammar IS LL(1)!\n\n");
        } else {
            AppendOutput("✗ Grammar IS NOT LL(1)!\n\n");
            AppendOutput("Conflicts detected in parsing table.\n");
        }
        
        // Выводим First sets
        AppendOutput("FIRST sets:\n");
        for (const auto& [nt, firstSet] : firstSets) {
            char line[256];
            snprintf(line, sizeof(line), "  %s: { ", nt.c_str());
            AppendOutput(line);
            
            bool first = true;
            for (const auto& sym : firstSet) {
                if (!first) AppendOutput(", ");
                AppendOutput(sym.c_str());
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
            for (const auto& sym : followSet) {
                if (!first) AppendOutput(", ");
                AppendOutput(sym.c_str());
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
        
        // Левая панель - редактор грамматики
        ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, 0), true);
        
        ImGui::Text("Grammar Editor");
        ImGui::SameLine();
        if (ImGui::Button("Parse (F5)")) ParseGrammar();
        ImGui::SameLine();
        if (ImGui::Button("Clear")) { grammarText[0] = '\0'; ClearOutput(); }
        
        ImGui::Separator();
        
        ImGui::InputTextMultiline("##grammar", grammarText, sizeof(grammarText), 
                                   ImVec2(-FLT_MIN, -FLT_MIN),
                                   ImGuiInputTextFlags_AllowTabInput);
        
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