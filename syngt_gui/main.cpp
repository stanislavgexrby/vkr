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

static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static char grammarText[1024 * 64] = "";
static char outputText[1024 * 64] = "";
static char currentFile[MAX_PATH] = "";
static std::unique_ptr<syngt::Grammar> grammar;
static syngt::UndoRedo undoRedo;
static std::unique_ptr<syngt::graphics::DrawObjectList> drawObjects;
static int activeNTIndex = 0;
static syngt::SelectionMask selectionMask;
static bool showAbout = false;
static bool showHelp = false;

// Editor state
static bool isDragging = false;
static ImVec2 lastMousePos;
static syngt::graphics::DrawObject* hoveredObject = nullptr;
static syngt::graphics::DrawObject* contextMenuObject = nullptr;
static bool showAddTerminalDialog = false;
static bool showAddNonTerminalDialog = false;
static bool showAddReferenceDialog = false;  // ДОБАВИТЬ
static bool showEditDialog = false;
static char newSymbolName[256] = "";
static bool useOrOperator = false;  // ДОБАВИТЬ

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

// Undo/Redo
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
        AppendOutput("Cannot undo!\n");
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
        
        std::string tempFile = "temp_undo.grm";
        grammar->save(tempFile);
        std::string newText = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());
        
        if (newText.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newText.c_str());
        }
        
        ClearOutput();
        AppendOutput("Undo successful\n");
    }
}

void Redo() {
    if (!grammar || !undoRedo.canRedo()) {
        ClearOutput();
        AppendOutput("Cannot redo!\n");
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
        
        std::string tempFile = "temp_redo.grm";
        grammar->save(tempFile);
        std::string newText = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());
        
        if (newText.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newText.c_str());
        }
        
        ClearOutput();
        AppendOutput("Redo successful\n");
    }
}

// Syntax Grammar
void RenderDiagram(ImDrawList* drawList, const ImVec2& offset) {
    if (!drawObjects || drawObjects->count() == 0) return;
    
    const float scale = 1.0f;
    const ImU32 lineColor = IM_COL32(200, 200, 200, 255);
    const ImU32 selectedColor = IM_COL32(255, 100, 100, 255);
    const ImU32 hoveredColor = IM_COL32(100, 255, 100, 255);
    const ImU32 textColor = IM_COL32(255, 255, 255, 255);
    
    for (int i = 0; i < drawObjects->count(); ++i) {
        syngt::graphics::DrawObject* obj = (*drawObjects)[i];
        if (!obj) continue;
        
        float x = offset.x + obj->x() * scale;
        float y = offset.y + obj->y() * scale;
        
        ImU32 currentColor = lineColor;
        if (obj->selected()) {
            currentColor = selectedColor;
        } else if (obj == hoveredObject) {
            currentColor = hoveredColor;
        }
        
        float thickness = (obj->selected() || obj == hoveredObject) ? 3.0f : 2.0f;
        
        int type = obj->getType();
        
        if (type == syngt::graphics::ctDrawObjectTerminal) {
            float w = obj->getLength() * scale / 2.0f;
            float h = 20.0f * scale;
            drawList->AddEllipse(ImVec2(x + w, y), ImVec2(w, h), currentColor, 0, 0, thickness);
            
            auto leaf = dynamic_cast<syngt::graphics::DrawObjectLeaf*>(obj);
            if (leaf) {
                std::string name = leaf->name();
                ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
                drawList->AddText(ImVec2(x + w - textSize.x/2, y - textSize.y/2), textColor, name.c_str());
            }
        }
        else if (type == syngt::graphics::ctDrawObjectNonTerminal) {
            float w = obj->getLength() * scale;
            float h = 40.0f * scale;
            drawList->AddRect(ImVec2(x, y - h/2), ImVec2(x + w, y + h/2), currentColor, 0.0f, 0, thickness);
            
            auto leaf = dynamic_cast<syngt::graphics::DrawObjectLeaf*>(obj);
            if (leaf) {
                std::string name = leaf->name();
                ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
                drawList->AddText(ImVec2(x + w/2 - textSize.x/2, y - textSize.y/2), textColor, name.c_str());
            }
        }
        else if (type == syngt::graphics::ctDrawObjectFirst) {
            drawList->AddTriangleFilled(
                ImVec2(x, y - 10),
                ImVec2(x, y + 10),
                ImVec2(x + 15, y),
                currentColor
            );
        }
        else if (type == syngt::graphics::ctDrawObjectLast) {
            drawList->AddTriangleFilled(
                ImVec2(x + 15, y - 10),
                ImVec2(x + 15, y + 10),
                ImVec2(x, y),
                currentColor
            );
        }
        else if (type == syngt::graphics::ctDrawObjectPoint || 
                 type == syngt::graphics::ctDrawObjectExtendedPoint) {
            float radius = (obj->selected() || obj == hoveredObject) ? 4.0f : 3.0f;
            drawList->AddCircleFilled(ImVec2(x, y), radius, currentColor);
        }
        
        syngt::graphics::Arrow* arrow = obj->inArrow();
        if (arrow && arrow->getFromDO()) {
            syngt::graphics::DrawObject* from = dynamic_cast<syngt::graphics::DrawObject*>(arrow->getFromDO());
            if (from) {
                float x1 = offset.x + from->endX() * scale;
                float y1 = offset.y + from->y() * scale;
                drawList->AddLine(ImVec2(x1, y1), ImVec2(x, y), currentColor, thickness);
            }
        }
        
        if (type == syngt::graphics::ctDrawObjectPoint) {
            auto point = dynamic_cast<syngt::graphics::DrawObjectPoint*>(obj);
            if (point && point->secondInArrow()) {
                syngt::graphics::Arrow* arrow2 = point->secondInArrow();
                if (arrow2->getFromDO()) {
                    syngt::graphics::DrawObject* from = dynamic_cast<syngt::graphics::DrawObject*>(arrow2->getFromDO());
                    if (from) {
                        float x1 = offset.x + from->endX() * scale;
                        float y1 = offset.y + from->y() * scale;
                        drawList->AddLine(ImVec2(x1, y1), ImVec2(x, y), currentColor, thickness);
                    }
                }
            }
        }
    }
}

void UpdateDiagram() {
    if (!grammar) {
        drawObjects.reset();
        return;
    }
    
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
    
    drawObjects = std::make_unique<syngt::graphics::DrawObjectList>(grammar.get());
    syngt::Creator::createDrawObjects(drawObjects.get(), item->root());
}

syngt::graphics::DrawObject* FindDrawObjectAt(const ImVec2& pos, const ImVec2& offset) {
    if (!drawObjects || drawObjects->count() == 0) return nullptr;
    
    const float scale = 1.0f;
    
    for (int i = drawObjects->count() - 1; i >= 0; --i) {
        syngt::graphics::DrawObject* obj = (*drawObjects)[i];
        if (!obj) continue;
        
        int screenX = static_cast<int>((pos.x - offset.x) / scale);
        int screenY = static_cast<int>((pos.y - offset.y) / scale);
        
        if (obj->internalPoint(screenX, screenY)) {
            return obj;
        }
    }
    
    return nullptr;
}

void SyncGrammarFromDiagram() {
    if (!grammar || !drawObjects) return;
    
    // TODO: Implement reverse conversion from diagram to grammar text
    // This is complex and requires analyzing the DrawObject graph structure
    // For now, just save the current state
    SaveCurrentState();
}

void RebuildGrammarFromText() {
    try {
        std::string tempFile = "temp_grammar.grm";
        SaveTextFile(tempFile.c_str(), grammarText);
        
        grammar = std::make_unique<syngt::Grammar>();
        grammar->load(tempFile);
        std::remove(tempFile.c_str());
        
        UpdateDiagram();
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Rebuild Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void UpdateGrammarText() {
    if (!grammar) return;
    
    try {
        std::string tempFile = "temp_update.grm";
        grammar->save(tempFile);
        std::string newText = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());
        
        if (newText.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newText.c_str());
        }
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Update Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

bool RenameTerminal(int oldId, const std::string& newName) {
    if (!grammar) return false;
    
    auto terminals = grammar->getTerminals();
    if (oldId < 0 || oldId >= static_cast<int>(terminals.size())) {
        return false;
    }
    
    // Проверяем, не существует ли уже такое имя
    for (const auto& term : terminals) {
        if (term == newName) {
            ClearOutput();
            AppendOutput("Terminal '");
            AppendOutput(newName.c_str());
            AppendOutput("' already exists!\n");
            return false;
        }
    }
    
    // Заменяем в тексте грамматики
    std::string oldName = terminals[oldId];
    std::string text = grammarText;
    
    // Заменяем 'oldName' на 'newName' с кавычками
    std::string oldQuoted = "'" + oldName + "'";
    std::string newQuoted = "'" + newName + "'";
    
    size_t pos = 0;
    while ((pos = text.find(oldQuoted, pos)) != std::string::npos) {
        text.replace(pos, oldQuoted.length(), newQuoted);
        pos += newQuoted.length();
    }
    
    if (text.size() < sizeof(grammarText)) {
        strcpy_s(grammarText, sizeof(grammarText), text.c_str());
        RebuildGrammarFromText();
        SaveCurrentState();
        return true;
    }
    
    return false;
}

bool RenameNonTerminal(int oldId, const std::string& newName) {
    if (!grammar) return false;
    
    auto nts = grammar->getNonTerminals();
    if (oldId < 0 || oldId >= static_cast<int>(nts.size())) {
        return false;
    }
    
    // Проверяем, не существует ли уже такое имя
    for (const auto& nt : nts) {
        if (nt == newName) {
            ClearOutput();
            AppendOutput("Non-terminal '");
            AppendOutput(newName.c_str());
            AppendOutput("' already exists!\n");
            return false;
        }
    }
    
    // Заменяем в тексте грамматики
    std::string oldName = nts[oldId];
    std::string text = grammarText;
    
    // Ищем все вхождения (в определении и в ссылках)
    size_t pos = 0;
    while ((pos = text.find(oldName, pos)) != std::string::npos) {
        // Проверяем, что это отдельное слово (не часть другого идентификатора)
        bool isStart = (pos == 0 || !isalnum(text[pos-1]));
        bool isEnd = (pos + oldName.length() >= text.length() || 
                      !isalnum(text[pos + oldName.length()]));
        
        if (isStart && isEnd) {
            text.replace(pos, oldName.length(), newName);
            pos += newName.length();
        } else {
            pos++;
        }
    }
    
    if (text.size() < sizeof(grammarText)) {
        strcpy_s(grammarText, sizeof(grammarText), text.c_str());
        RebuildGrammarFromText();
        SaveCurrentState();
        
        // Обновляем активный индекс, если переименовали текущий нетерминал
        auto newNts = grammar->getNonTerminals();
        for (int i = 0; i < static_cast<int>(newNts.size()); ++i) {
            if (newNts[i] == newName) {
                activeNTIndex = i;
                break;
            }
        }
        
        return true;
    }
    
    return false;
}

void DeleteSelectedObjects() {
    if (!drawObjects || !grammar) return;
    
    int selectedCount = 0;
    std::vector<syngt::graphics::DrawObject*> toDelete;
    
    for (int i = 0; i < drawObjects->count(); ++i) {
        if ((*drawObjects)[i]->selected()) {
            selectedCount++;
            toDelete.push_back((*drawObjects)[i]);
        }
    }
    
    if (selectedCount == 0) {
        ClearOutput();
        AppendOutput("No objects selected!\n");
        return;
    }
    
    ClearOutput();
    AppendOutput("Deleting ");
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", selectedCount);
    AppendOutput(buf);
    AppendOutput(" object(s)...\n");
    
    // Собираем имена для удаления
    std::vector<std::pair<std::string, int>> symbolsToRemove; // имя, тип
    
    for (auto* obj : toDelete) {
        auto leaf = dynamic_cast<syngt::graphics::DrawObjectLeaf*>(obj);
        if (leaf) {
            std::string name = leaf->name();
            int type = obj->getType();
            if (!name.empty() && name != "eps") {
                symbolsToRemove.push_back({name, type});
                AppendOutput("  - ");
                AppendOutput(name.c_str());
                AppendOutput("\n");
            }
        }
    }
    
    if (symbolsToRemove.empty()) {
        AppendOutput("Only structural elements selected (cannot delete)\n");
        drawObjects->unselectAll();
        return;
    }
    
    // Работаем с текущим правилом
    auto nts = grammar->getNonTerminals();
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
        AppendOutput("No active non-terminal!\n");
        return;
    }
    
    std::string activeNT = nts[activeNTIndex];
    std::string text = grammarText;
    
    // Ищем правило активного нетерминала
    size_t ruleStart = text.find(activeNT + " :");
    if (ruleStart == std::string::npos) {
        ruleStart = text.find(activeNT + ":");
    }
    
    if (ruleStart == std::string::npos) {
        AppendOutput("Cannot find rule for ");
        AppendOutput(activeNT.c_str());
        AppendOutput("\n");
        return;
    }
    
    // Находим конец правила (точку)
    size_t colonPos = text.find(":", ruleStart);
    size_t ruleEnd = text.find(".", colonPos);
    
    if (colonPos == std::string::npos || ruleEnd == std::string::npos) {
        AppendOutput("Malformed rule!\n");
        return;
    }
    
    // Извлекаем тело правила
    std::string ruleBody = text.substr(colonPos + 1, ruleEnd - colonPos - 1);
    
    AppendOutput("\nOriginal rule body: ");
    AppendOutput(ruleBody.c_str());
    AppendOutput("\n");
    
    // Удаляем каждый символ из тела правила
    for (const auto& [name, type] : symbolsToRemove) {
        if (type == syngt::graphics::ctDrawObjectTerminal) {
            // Терминал в кавычках
            std::string pattern = "'" + name + "'";
            size_t pos = 0;
            while ((pos = ruleBody.find(pattern, pos)) != std::string::npos) {
                size_t endPos = pos + pattern.length();
                
                // Удаляем пробелы после
                while (endPos < ruleBody.length() && isspace(ruleBody[endPos])) {
                    endPos++;
                }
                
                // Удаляем запятую или точку с запятой
                if (endPos < ruleBody.length() && (ruleBody[endPos] == ',' || ruleBody[endPos] == ';')) {
                    endPos++;
                    // Удаляем пробелы после разделителя
                    while (endPos < ruleBody.length() && isspace(ruleBody[endPos])) {
                        endPos++;
                    }
                } else {
                    // Может быть разделитель перед символом
                    if (pos > 0) {
                        int beforePos = static_cast<int>(pos) - 1;
                        while (beforePos > 0 && isspace(ruleBody[beforePos])) {
                            beforePos--;
                        }
                        if (beforePos >= 0 && (ruleBody[beforePos] == ',' || ruleBody[beforePos] == ';')) {
                            // Удаляем разделитель и пробелы перед символом
                            pos = beforePos;
                            // Удаляем пробелы перед разделителем
                            while (pos > 0 && isspace(ruleBody[pos - 1])) {
                                pos--;
                            }
                        }
                    }
                }
                
                ruleBody.erase(pos, endPos - pos);
            }
        } else if (type == syngt::graphics::ctDrawObjectNonTerminal) {
            // Нетерминал без кавычек
            size_t pos = 0;
            while ((pos = ruleBody.find(name, pos)) != std::string::npos) {
                // Проверяем границы слова
                bool isStart = (pos == 0 || !isalnum(ruleBody[pos-1]));
                bool isEnd = (pos + name.length() >= ruleBody.length() || 
                              !isalnum(ruleBody[pos + name.length()]));
                
                if (isStart && isEnd) {
                    size_t endPos = pos + name.length();
                    
                    // Удаляем пробелы после
                    while (endPos < ruleBody.length() && isspace(ruleBody[endPos])) {
                        endPos++;
                    }
                    
                    // Удаляем разделитель
                    if (endPos < ruleBody.length() && (ruleBody[endPos] == ',' || ruleBody[endPos] == ';')) {
                        endPos++;
                        while (endPos < ruleBody.length() && isspace(ruleBody[endPos])) {
                            endPos++;
                        }
                    } else if (pos > 0) {
                        // Проверяем разделитель перед
                        int beforePos = static_cast<int>(pos) - 1;
                        while (beforePos > 0 && isspace(ruleBody[beforePos])) {
                            beforePos--;
                        }
                        if (beforePos >= 0 && (ruleBody[beforePos] == ',' || ruleBody[beforePos] == ';')) {
                            pos = beforePos;
                            while (pos > 0 && isspace(ruleBody[pos - 1])) {
                                pos--;
                            }
                        }
                    }
                    
                    ruleBody.erase(pos, endPos - pos);
                } else {
                    pos++;
                }
            }
        }
    }
    
    // Очищаем лишние пробелы и разделители
    // Удаляем начальные пробелы
    size_t firstNonSpace = ruleBody.find_first_not_of(" \t\n\r");
    if (firstNonSpace != std::string::npos) {
        ruleBody = ruleBody.substr(firstNonSpace);
    } else {
        ruleBody = "";
    }
    
    // Удаляем конечные пробелы
    size_t lastNonSpace = ruleBody.find_last_not_of(" \t\n\r");
    if (lastNonSpace != std::string::npos) {
        ruleBody = ruleBody.substr(0, lastNonSpace + 1);
    }
    
    // Удаляем начальные/конечные разделители
    if (!ruleBody.empty() && (ruleBody[0] == ',' || ruleBody[0] == ';')) {
        ruleBody = ruleBody.substr(1);
        // Удаляем пробелы после
        firstNonSpace = ruleBody.find_first_not_of(" \t\n\r");
        if (firstNonSpace != std::string::npos) {
            ruleBody = ruleBody.substr(firstNonSpace);
        }
    }
    
    if (!ruleBody.empty() && (ruleBody[ruleBody.length()-1] == ',' || ruleBody[ruleBody.length()-1] == ';')) {
        ruleBody = ruleBody.substr(0, ruleBody.length() - 1);
        // Удаляем пробелы перед
        lastNonSpace = ruleBody.find_last_not_of(" \t\n\r");
        if (lastNonSpace != std::string::npos) {
            ruleBody = ruleBody.substr(0, lastNonSpace + 1);
        }
    }
    
    // Если тело пустое, ставим eps
    if (ruleBody.empty()) {
        ruleBody = " eps";
    }
    
    AppendOutput("New rule body: ");
    AppendOutput(ruleBody.c_str());
    AppendOutput("\n");
    
    // Заменяем тело правила в тексте
    text.replace(colonPos + 1, ruleEnd - colonPos - 1, ruleBody);
    
    if (text.size() < sizeof(grammarText)) {
        strcpy_s(grammarText, sizeof(grammarText), text.c_str());
        
        AppendOutput("\nUpdated grammar:\n");
        AppendOutput(text.c_str());
        AppendOutput("\n\n");
        
        RebuildGrammarFromText();
        SaveCurrentState();
        AppendOutput("Deleted successfully!\n");
    } else {
        AppendOutput("Error: Text too large after deletion\n");
    }
}

void AddTerminalToGrammar(const std::string& name) {
    if (!grammar) {
        ClearOutput();
        AppendOutput("No grammar loaded!\n");
        return;
    }
    
    auto terminals = grammar->getTerminals();
    for (const auto& term : terminals) {
        if (term == name) {
            ClearOutput();
            AppendOutput("Terminal '");
            AppendOutput(name.c_str());
            AppendOutput("' already exists!\n");
            return;
        }
    }
    
    auto nts = grammar->getNonTerminals();
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
        ClearOutput();
        AppendOutput("No active non-terminal!\n");
        return;
    }
    
    std::string text = grammarText;
    std::string ntName = nts[activeNTIndex];
    
    // Ищем правило
    size_t pos = text.find(ntName + " :");
    if (pos == std::string::npos) {
        pos = text.find(ntName + ":");
    }
    
    if (pos == std::string::npos) {
        ClearOutput();
        AppendOutput("ERROR: Cannot find rule for '");
        AppendOutput(ntName.c_str());
        AppendOutput("'\n");
        return;
    }
    
    size_t colonPos = text.find(":", pos);
    size_t dotPos = text.find(".", colonPos);
    
    if (colonPos == std::string::npos || dotPos == std::string::npos) {
        ClearOutput();
        AppendOutput("ERROR: Malformed rule\n");
        return;
    }
    
    std::string ruleBody = text.substr(colonPos + 1, dotPos - colonPos - 1);
    
    // Удаляем все вхождения eps
    size_t epsPos = 0;
    while ((epsPos = ruleBody.find("eps", epsPos)) != std::string::npos) {
        // Проверяем что это отдельное слово
        bool isStart = (epsPos == 0 || !isalnum(ruleBody[epsPos-1]));
        bool isEnd = (epsPos + 3 >= ruleBody.length() || !isalnum(ruleBody[epsPos + 3]));
        
        if (isStart && isEnd) {
            size_t endPos = epsPos + 3;
            
            // Удаляем пробелы после eps
            while (endPos < ruleBody.length() && isspace(ruleBody[endPos])) {
                endPos++;
            }
            
            // Удаляем разделители после eps
            if (endPos < ruleBody.length() && (ruleBody[endPos] == ',' || ruleBody[endPos] == ';')) {
                endPos++;
                while (endPos < ruleBody.length() && isspace(ruleBody[endPos])) {
                    endPos++;
                }
            }
            
            ruleBody.erase(epsPos, endPos - epsPos);
        } else {
            epsPos++;
        }
    }
    
    // Trim
    size_t start = ruleBody.find_first_not_of(" \t\n\r");
    size_t end = ruleBody.find_last_not_of(" \t\n\r");
    
    std::string trimmedBody;
    if (start != std::string::npos && end != std::string::npos) {
        trimmedBody = ruleBody.substr(start, end - start + 1);
    } else {
        trimmedBody = "";
    }
    
    // Формируем новое тело правила
    std::string newBody;
    if (trimmedBody.empty()) {
        newBody = " '" + name + "'";
    } else {
        newBody = " " + trimmedBody + ", '" + name + "'";
    }
    
    // Заменяем
    text.replace(colonPos + 1, dotPos - colonPos - 1, newBody);
    
    if (text.size() >= sizeof(grammarText)) {
        ClearOutput();
        AppendOutput("ERROR: Text too large!\n");
        return;
    }
    
    strcpy_s(grammarText, sizeof(grammarText), text.c_str());
    RebuildGrammarFromText();
    SaveCurrentState();
    
    ClearOutput();
    AppendOutput("Added terminal '");
    AppendOutput(name.c_str());
    AppendOutput("' to '");
    AppendOutput(ntName.c_str());
    AppendOutput("'\n");
}

void AddNonTerminalReference(const std::string& ntRef, bool useOr) {
    if (!grammar) {
        ClearOutput();
        AppendOutput("No grammar loaded!\n");
        return;
    }
    
    // Проверяем, существует ли такой нетерминал
    auto nts = grammar->getNonTerminals();
    bool found = false;
    for (const auto& nt : nts) {
        if (nt == ntRef) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        ClearOutput();
        AppendOutput("Non-terminal '");
        AppendOutput(ntRef.c_str());
        AppendOutput("' does not exist!\n");
        return;
    }
    
    // Работаем с активным правилом
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
        ClearOutput();
        AppendOutput("No active non-terminal!\n");
        return;
    }
    
    std::string text = grammarText;
    std::string ntName = nts[activeNTIndex];
    
    // Ищем правило
    size_t pos = text.find(ntName + " :");
    if (pos == std::string::npos) {
        pos = text.find(ntName + ":");
    }
    
    if (pos == std::string::npos) {
        ClearOutput();
        AppendOutput("ERROR: Cannot find rule\n");
        return;
    }
    
    size_t colonPos = text.find(":", pos);
    size_t dotPos = text.find(".", colonPos);
    
    if (colonPos == std::string::npos || dotPos == std::string::npos) {
        ClearOutput();
        AppendOutput("ERROR: Malformed rule\n");
        return;
    }
    
    std::string ruleBody = text.substr(colonPos + 1, dotPos - colonPos - 1);
    
    // Удаляем eps если есть
    size_t epsPos = 0;
    while ((epsPos = ruleBody.find("eps", epsPos)) != std::string::npos) {
        bool isStart = (epsPos == 0 || !isalnum(ruleBody[epsPos-1]));
        bool isEnd = (epsPos + 3 >= ruleBody.length() || !isalnum(ruleBody[epsPos + 3]));
        
        if (isStart && isEnd) {
            size_t endPos = epsPos + 3;
            while (endPos < ruleBody.length() && isspace(ruleBody[endPos])) {
                endPos++;
            }
            if (endPos < ruleBody.length() && (ruleBody[endPos] == ',' || ruleBody[endPos] == ';')) {
                endPos++;
                while (endPos < ruleBody.length() && isspace(ruleBody[endPos])) {
                    endPos++;
                }
            }
            ruleBody.erase(epsPos, endPos - epsPos);
        } else {
            epsPos++;
        }
    }
    
    // Trim
    size_t start = ruleBody.find_first_not_of(" \t\n\r");
    size_t end = ruleBody.find_last_not_of(" \t\n\r");
    
    std::string trimmedBody;
    if (start != std::string::npos && end != std::string::npos) {
        trimmedBody = ruleBody.substr(start, end - start + 1);
    } else {
        trimmedBody = "";
    }
    
    // Формируем новое тело с выбранным оператором
    std::string separator = useOr ? " ; " : ", ";
    std::string newBody;
    
    if (trimmedBody.empty()) {
        newBody = " " + ntRef;
    } else {
        newBody = " " + trimmedBody + separator + ntRef;
    }
    
    // Заменяем
    text.replace(colonPos + 1, dotPos - colonPos - 1, newBody);
    
    if (text.size() >= sizeof(grammarText)) {
        ClearOutput();
        AppendOutput("ERROR: Text too large!\n");
        return;
    }
    
    strcpy_s(grammarText, sizeof(grammarText), text.c_str());
    RebuildGrammarFromText();
    SaveCurrentState();
    
    ClearOutput();
    AppendOutput("Added reference to '");
    AppendOutput(ntRef.c_str());
    AppendOutput("' ");
    AppendOutput(useOr ? "(OR)" : "(AND)");
    AppendOutput(" to '");
    AppendOutput(ntName.c_str());
    AppendOutput("'\n");
}

void AddSymbolWithOperator(const std::string& symbol, bool isTerminal, bool useOr) {
    if (isTerminal) {
        // Модифицируем AddTerminalToGrammar с поддержкой OR
        if (!grammar) return;
        
        auto nts = grammar->getNonTerminals();
        if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) return;
        
        std::string text = grammarText;
        std::string ntName = nts[activeNTIndex];
        
        size_t pos = text.find(ntName + " :");
        if (pos == std::string::npos) pos = text.find(ntName + ":");
        if (pos == std::string::npos) return;
        
        size_t colonPos = text.find(":", pos);
        size_t dotPos = text.find(".", colonPos);
        if (colonPos == std::string::npos || dotPos == std::string::npos) return;
        
        std::string ruleBody = text.substr(colonPos + 1, dotPos - colonPos - 1);
        
        // Удаляем eps
        size_t epsPos = 0;
        while ((epsPos = ruleBody.find("eps", epsPos)) != std::string::npos) {
            bool isStart = (epsPos == 0 || !isalnum(ruleBody[epsPos-1]));
            bool isEnd = (epsPos + 3 >= ruleBody.length() || !isalnum(ruleBody[epsPos + 3]));
            if (isStart && isEnd) {
                size_t endPos = epsPos + 3;
                while (endPos < ruleBody.length() && isspace(ruleBody[endPos])) endPos++;
                if (endPos < ruleBody.length() && (ruleBody[endPos] == ',' || ruleBody[endPos] == ';')) {
                    endPos++;
                    while (endPos < ruleBody.length() && isspace(ruleBody[endPos])) endPos++;
                }
                ruleBody.erase(epsPos, endPos - epsPos);
            } else {
                epsPos++;
            }
        }
        
        size_t start = ruleBody.find_first_not_of(" \t\n\r");
        size_t end = ruleBody.find_last_not_of(" \t\n\r");
        std::string trimmedBody;
        if (start != std::string::npos && end != std::string::npos) {
            trimmedBody = ruleBody.substr(start, end - start + 1);
        }
        
        std::string separator = useOr ? " ; " : ", ";
        std::string newBody;
        if (trimmedBody.empty()) {
            newBody = " '" + symbol + "'";
        } else {
            newBody = " " + trimmedBody + separator + "'" + symbol + "'";
        }
        
        text.replace(colonPos + 1, dotPos - colonPos - 1, newBody);
        if (text.size() >= sizeof(grammarText)) return;
        
        strcpy_s(grammarText, sizeof(grammarText), text.c_str());
        RebuildGrammarFromText();
        SaveCurrentState();
        
        ClearOutput();
        AppendOutput("Added '");
        AppendOutput(symbol.c_str());
        AppendOutput("' ");
        AppendOutput(useOr ? "(OR)" : "(AND)");
        AppendOutput("\n");
    } else {
        AddNonTerminalReference(symbol, useOr);
    }
}

void AddNonTerminalToGrammar(const std::string& name) {
    if (!grammar) {
        ClearOutput();
        AppendOutput("No grammar loaded!\n");
        return;
    }
    
    // Проверяем, не существует ли уже
    auto nts = grammar->getNonTerminals();
    for (const auto& nt : nts) {
        if (nt == name) {
            ClearOutput();
            AppendOutput("Non-terminal '");
            AppendOutput(name.c_str());
            AppendOutput("' already exists!\n");
            return;
        }
    }
    
    // Добавляем новое правило
    std::string text = grammarText;
    
    // Находим EOGram!
    size_t eogramPos = text.find("EOGram!");
    if (eogramPos != std::string::npos) {
        // Добавляем перевод строки перед если нужно
        if (eogramPos > 0 && text[eogramPos - 1] != '\n') {
            text.insert(eogramPos, "\n");
            eogramPos++;
        }
        
        std::string newRule = name + " : eps.\n";
        text.insert(eogramPos, newRule);
        
        if (text.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), text.c_str());
            RebuildGrammarFromText();
            
            // Находим индекс нового нетерминала и делаем его активным
            if (grammar) {
                auto newNts = grammar->getNonTerminals();
                for (int i = 0; i < static_cast<int>(newNts.size()); ++i) {
                    if (newNts[i] == name) {
                        activeNTIndex = i;
                        UpdateDiagram();
                        break;
                    }
                }
            }
            
            SaveCurrentState();
            
            ClearOutput();
            AppendOutput("Added non-terminal '");
            AppendOutput(name.c_str());
            AppendOutput("' with empty rule\n");
        }
    }
}

// Grammar Operations
void ParseGrammar() {
    try {
        ClearOutput();
        
        std::string tempFile = "temp_grammar.grm";
        SaveTextFile(tempFile.c_str(), grammarText);
        
        grammar = std::make_unique<syngt::Grammar>();
        grammar->load(tempFile);
        
        std::remove(tempFile.c_str());
        
        AppendOutput("Grammar parsed successfully!\n\n");
        
        char stats[512];
        snprintf(stats, sizeof(stats), 
                 "Terminals: %d\nNon-terminals: %zu\nSemantics: %d\nMacros: %d\n",
                 grammar->terminals()->getCount(),
                 grammar->getNonTerminals().size(),
                 grammar->semantics()->getCount(),
                 grammar->macros()->getCount());
        AppendOutput(stats);
        
        SaveCurrentState();
        
        UpdateDiagram();
        
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Parse Error:\n");
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
            AppendOutput("File too large!\n");
            return;
        }
        
        strcpy_s(grammarText, sizeof(grammarText), content.c_str());
        strcpy_s(currentFile, sizeof(currentFile), filename.c_str());
        
        AppendOutput("File loaded: ");
        AppendOutput(filename.c_str());
        AppendOutput("\n");
        
        ParseGrammar();
        
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Load Error:\n");
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
        AppendOutput("File saved: ");
        AppendOutput(filename.c_str());
        AppendOutput("\n");
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Save Error:\n");
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
        AppendOutput("Please parse grammar first!\n");
        return;
    }
    
    try {
        ClearOutput();
        syngt::LeftElimination::eliminate(grammar.get());
        
        std::string tempFile = "temp_result.grm";
        grammar->save(tempFile);
        std::string newGrammar = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());
        
        if (newGrammar.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
        }
        
        AppendOutput("Left recursion eliminated!\n");
        
        SaveCurrentState();
        UpdateDiagram();
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void LeftFactorize() {
    if (!grammar) {
        ClearOutput();
        AppendOutput("Please parse grammar first!\n");
        return;
    }
    
    try {
        ClearOutput();
        syngt::LeftFactorization::factorizeAll(grammar.get());
        
        std::string tempFile = "temp_result.grm";
        grammar->save(tempFile);
        std::string newGrammar = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());
        
        if (newGrammar.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
        }
        
        AppendOutput("Left factorization completed!\n");
        
        SaveCurrentState();
        UpdateDiagram();
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void RemoveUselessSymbols() {
    if (!grammar) {
        ClearOutput();
        AppendOutput("Please parse grammar first!\n");
        return;
    }
    
    try {
        ClearOutput();
        syngt::RemoveUseless::remove(grammar.get());
        
        std::string tempFile = "temp_result.grm";
        grammar->save(tempFile);
        std::string newGrammar = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());
        
        if (newGrammar.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
        }
        
        AppendOutput("Useless symbols removed!\n");
        
        SaveCurrentState();
        UpdateDiagram();
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void CheckLL1() {
    if (!grammar) {
        ClearOutput();
        AppendOutput("Please parse grammar first!\n");
        return;
    }
    
    try {
        ClearOutput();
        
        auto firstSets = syngt::FirstFollow::computeFirst(grammar.get());
        auto followSets = syngt::FirstFollow::computeFollow(grammar.get(), firstSets);
        
        auto table = syngt::ParsingTable::build(grammar.get());
        
        if (table->getConflicts().empty()) {
            AppendOutput("Grammar IS LL(1)!\n\n");
        } else {
            AppendOutput("Grammar IS NOT LL(1)!\n\n");
            AppendOutput("Conflicts:\n");
            for (const auto& conflict : table->getConflicts()) {
                AppendOutput("  ");
                AppendOutput(conflict.c_str());
                AppendOutput("\n");
            }
            AppendOutput("\n");
        }
        
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
        AppendOutput("Error:\n");
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
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), 
                       nullptr, nullptr, nullptr, nullptr, L"SynGT", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"SynGT - Syntax Grammar Transformation", 
                                 WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, 
                                 nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = "syngt.ini";

    // Setup style
    ImGui::StyleColorsDark();
    
    // Color style
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\consola.ttf", 16.0f);
    
    // Default example
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
            
            ImGui::SameLine(io.DisplaySize.x - 400);
            if (currentFile[0] != '\0') {
                ImGui::Text("File: %s", currentFile);
            } else {
                ImGui::Text("No file loaded");
            }
            
            ImGui::EndMainMenuBar();
        }

        // Hot keys
        ImGuiIO& io = ImGui::GetIO();
        
        // Delete
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && drawObjects) {
            int selectedCount = 0;
            for (int i = 0; i < drawObjects->count(); ++i) {
                if ((*drawObjects)[i]->selected()) {
                    selectedCount++;
                }
            }
            if (selectedCount > 0) {
                DeleteSelectedObjects();
            }
        }
        
        // Ctrl+A
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A) && drawObjects) {
            for (int i = 0; i < drawObjects->count(); ++i) {
                (*drawObjects)[i]->setSelected(true);
            }
        }
        
        // Escape
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) && drawObjects) {
            drawObjects->unselectAll();
        }

        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - ImGui::GetFrameHeight()));
        ImGui::Begin("SynGT Main", nullptr, 
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

        // Splitter
        static float leftWidth = io.DisplaySize.x * 0.55f;
        
        ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, 0), true);
        
        if (ImGui::BeginTabBar("LeftTabs")) {
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
            
            if (ImGui::BeginTabItem("Syntax Diagram")) {
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
                    
                    ImGui::BeginChild("DiagramCanvas", ImVec2(0, 0), false);
                    
                    if (drawObjects && drawObjects->count() > 0) {
                        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
                        ImVec2 offset = ImVec2(canvasPos.x + 20, canvasPos.y + 50);
                        
                        // Canvas как кликабельная область
                        ImGui::InvisibleButton("##canvas", canvasSize);
                        bool isHovered = ImGui::IsItemHovered();
                        
                        ImVec2 mousePos = ImGui::GetMousePos();
                        
                        // Обработка наведения
                        if (isHovered && !isDragging) {
                            hoveredObject = FindDrawObjectAt(mousePos, offset);
                            if (hoveredObject) {
                                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                            }
                        } else if (!isHovered) {
                            hoveredObject = nullptr;
                        }
                        
                        // Обработка левого клика
                        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            syngt::graphics::DrawObject* clicked = FindDrawObjectAt(mousePos, offset);
                            
                            if (clicked) {
                                isDragging = true;
                                lastMousePos = mousePos;
                                
                                if (!ImGui::GetIO().KeyCtrl) {
                                    drawObjects->unselectAll();
                                }
                                drawObjects->changeSelection(clicked);
                            } else {
                                drawObjects->unselectAll();
                            }
                        }
                        
                        // Обработка перетаскивания
                        if (isDragging) {
                            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                                ImVec2 delta = ImVec2(
                                    mousePos.x - lastMousePos.x,
                                    mousePos.y - lastMousePos.y
                                );
                                
                                drawObjects->selectedMove(
                                    static_cast<int>(delta.x),
                                    static_cast<int>(delta.y)
                                );
                                
                                lastMousePos = mousePos;
                            } else {
                                isDragging = false;
                            }
                        }
                        
                        // Обработка правого клика
                        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                            contextMenuObject = FindDrawObjectAt(mousePos, offset);
                            ImGui::OpenPopup("DiagramContextMenu");
                        }

                        if (showAddReferenceDialog) {
                            ImGui::OpenPopup("Add Reference");
                            showAddReferenceDialog = false;
                        }
                        
                        if (ImGui::BeginPopupModal("Add Reference", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                            ImGui::Text("Select non-terminal to reference:");
                            
                            if (grammar) {
                                auto nts = grammar->getNonTerminals();
                                for (const auto& nt : nts) {
                                    if (ImGui::Selectable(nt.c_str())) {
                                        AddNonTerminalReference(nt, useOrOperator);
                                        ImGui::CloseCurrentPopup();
                                    }
                                }
                            }
                            
                            ImGui::Separator();
                            if (ImGui::Button("Cancel")) {
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }
                        
                        // Контекстное меню
                        if (ImGui::BeginPopup("DiagramContextMenu")) {
                            if (contextMenuObject) {
                                if (ImGui::MenuItem("Edit Symbol")) {
                                    showEditDialog = true;
                                    auto leaf = dynamic_cast<syngt::graphics::DrawObjectLeaf*>(contextMenuObject);
                                    if (leaf) {
                                        strcpy_s(newSymbolName, sizeof(newSymbolName), leaf->name().c_str());
                                    }
                                }
                                if (ImGui::MenuItem("Delete Selected", "Del")) {
                                    DeleteSelectedObjects();
                                }
                                ImGui::Separator();
                            }
                            
                            if (ImGui::BeginMenu("Add Terminal")) {
                                if (ImGui::MenuItem("Add (AND)")) {
                                    showAddTerminalDialog = true;
                                    useOrOperator = false;
                                    newSymbolName[0] = '\0';
                                }
                                if (ImGui::MenuItem("Add (OR)")) {
                                    showAddTerminalDialog = true;
                                    useOrOperator = true;
                                    newSymbolName[0] = '\0';
                                }
                                ImGui::EndMenu();
                            }
                            
                            if (ImGui::BeginMenu("Add Non-Terminal")) {
                                if (ImGui::MenuItem("Create New")) {
                                    showAddNonTerminalDialog = true;
                                    newSymbolName[0] = '\0';
                                }
                                if (ImGui::MenuItem("Add Reference (AND)")) {
                                    showAddReferenceDialog = true;
                                    useOrOperator = false;
                                    newSymbolName[0] = '\0';
                                }
                                if (ImGui::MenuItem("Add Reference (OR)")) {
                                    showAddReferenceDialog = true;
                                    useOrOperator = true;
                                    newSymbolName[0] = '\0';
                                }
                                ImGui::EndMenu();
                            }
                            
                            ImGui::EndPopup();
                        }
                        
                        // Рендеринг
                        ImDrawList* drawList = ImGui::GetWindowDrawList();
                        drawList->PushClipRect(canvasPos, 
                                             ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), 
                                             true);
                        
                        drawList->AddRectFilled(canvasPos, 
                                               ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                                               IM_COL32(30, 30, 35, 255));
                        
                        RenderDiagram(drawList, offset);
                        
                        drawList->PopClipRect();
                        
                        // Информация о выделении
                        int selectedCount = 0;
                        for (int i = 0; i < drawObjects->count(); ++i) {
                            if ((*drawObjects)[i]->selected()) {
                                selectedCount++;
                            }
                        }
                        if (selectedCount > 0) {
                            ImGui::SetCursorPos(ImVec2(10, canvasSize.y - 30));
                            ImGui::Text("Selected: %d", selectedCount);
                            ImGui::SameLine();
                            if (ImGui::SmallButton("Deselect")) {
                                drawObjects->unselectAll();
                            }
                        }
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

        // Editing dialogues
        if (showAddTerminalDialog) {
            ImGui::OpenPopup("Add Terminal");
            showAddTerminalDialog = false;
        }
        
        if (ImGui::BeginPopupModal("Add Terminal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter terminal name:");
            ImGui::InputText("##terminal", newSymbolName, sizeof(newSymbolName));
            
            if (ImGui::Button("Add") && newSymbolName[0] != '\0') {
                AddTerminalToGrammar(newSymbolName);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        if (showAddNonTerminalDialog) {
            ImGui::OpenPopup("Add Non-Terminal");
            showAddNonTerminalDialog = false;
        }
        
        if (ImGui::BeginPopupModal("Add Non-Terminal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter non-terminal name:");
            ImGui::InputText("##nonterminal", newSymbolName, sizeof(newSymbolName));
            
            if (ImGui::Button("Add") && newSymbolName[0] != '\0') {
                AddNonTerminalToGrammar(newSymbolName);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        if (showEditDialog) {
            ImGui::OpenPopup("Edit Symbol");
            showEditDialog = false;
        }
        
        if (ImGui::BeginPopupModal("Edit Symbol", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (contextMenuObject) {
                auto leaf = dynamic_cast<syngt::graphics::DrawObjectLeaf*>(contextMenuObject);
                if (leaf) {
                    int objType = contextMenuObject->getType();
                    
                    if (objType == syngt::graphics::ctDrawObjectTerminal) {
                        ImGui::Text("Edit terminal name:");
                    } else if (objType == syngt::graphics::ctDrawObjectNonTerminal) {
                        ImGui::Text("Edit non-terminal name:");
                    }
                    
                    ImGui::InputText("##edit", newSymbolName, sizeof(newSymbolName));
                    
                    if (ImGui::Button("Save") && newSymbolName[0] != '\0') {
                        bool success = false;
                        
                        if (objType == syngt::graphics::ctDrawObjectTerminal) {
                            success = RenameTerminal(leaf->id(), newSymbolName);
                        } else if (objType == syngt::graphics::ctDrawObjectNonTerminal) {
                            success = RenameNonTerminal(leaf->id(), newSymbolName);
                        }
                        
                        if (success) {
                            ClearOutput();
                            AppendOutput("Symbol renamed to: ");
                            AppendOutput(newSymbolName);
                            AppendOutput("\n");
                        }
                        
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            ImGui::EndPopup();
        }

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