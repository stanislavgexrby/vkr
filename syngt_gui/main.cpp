#ifdef _WIN32
    #include <imgui.h>
    #include <backends/imgui_impl_win32.h>
    #include <backends/imgui_impl_dx11.h>
    #include <d3d11.h>
    #include <tchar.h>
    #include <windows.h>
    #include <commdlg.h>
#else
    #include <imgui.h>
    #include <backends/imgui_impl_glfw.h>
    #include <backends/imgui_impl_opengl3.h>
    #include <GLFW/glfw3.h>
    #include <cstring>
    #include <limits.h>
    #define MAX_PATH 260
#endif

#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/parser/Parser.h>
#include <syngt/transform/LeftElimination.h>
#include <syngt/transform/RightElimination.h>
#include <syngt/transform/Regularize.h>
#include <syngt/transform/LeftFactorization.h>
#include <syngt/transform/RemoveUseless.h>
#include <syngt/transform/FirstFollow.h>
#include <syngt/analysis/ParsingTable.h>
#include <syngt/analysis/Minimize.h>
#include <syngt/analysis/RecursionAnalyzer.h>
#include <syngt/utils/UndoRedo.h>
#include <syngt/utils/Creator.h>
#include <syngt/graphics/DrawObject.h>
#include <syngt/regex/REBinaryOp.h>
#include <syngt/regex/RENonTerminal.h>

#include <fstream>
#include <sstream>
#include <memory>
#include <cmath>
#include <vector>
#include <string>
#include <unordered_map>
#include <limits>

#ifdef _WIN32
    static ID3D11Device*            g_pd3dDevice = nullptr;
    static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
    static IDXGISwapChain*          g_pSwapChain = nullptr;
    static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
#else
    static GLFWwindow* g_Window = nullptr;
#endif

void EliminateRightRecursion();
void EliminateBothRecursions();
void MinimizeGrammar();
void AnalyzeRecursion();
static void RefreshRecursionResults();
void ExtractRule();
void Substitute();
void ToggleMacro();
void OpenAllMacros();
void CloseAllDefinitions();
void NewFile();
void ImportFromGEdit();
void DiagramAddExtendedPoint();
void AddNonTerminalReferenceToGrammar(const std::string& name);
void FindAndCreateMissingNonTerminals(const std::string& rule);
void DrawArrowhead(ImDrawList* drawList, ImVec2 from, ImVec2 to, ImU32 color, float scale = 1.0f);
void LoadRuleToEditor();
void BuildRule();
void SaveCurrentDiagramToCache();
void ClearDiagramLayouts();

#ifdef _WIN32
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#else
    static void glfw_error_callback(int error, const char* description) {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    }
#endif

#ifndef _WIN32
    #include <cerrno>
    #include <cstring>
    
    /**
     * @brief Safe string copy for non-Windows platforms
     * @param dest Destination buffer
     * @param destsz Size of destination buffer
     * @param src Source string
     * @return 0 on success, EINVAL if parameters invalid, ERANGE if buffer too small
     */
    inline errno_t strcpy_s(char* dest, size_t destsz, const char* src) {
        if (!dest || destsz == 0) {
            return EINVAL;
        }
        
        if (!src) {
            dest[0] = '\0';
            return EINVAL;
        }
        
        size_t len = std::strlen(src);
        if (len >= destsz) {
            // Buffer too small - truncate and set error
            dest[0] = '\0';
            return ERANGE;
        }
        
        // Safe copy including null terminator
        std::memcpy(dest, src, len + 1);
        return 0;
    }
#endif

static char ruleText[4096] = "";
static char grammarText[1024 * 64] = "";
static int  grammarWidgetVersion = 0;  // increment to force InputTextMultiline reload
static char outputText[1024 * 64] = "";
static char currentFile[MAX_PATH] = "";
static std::unique_ptr<syngt::Grammar> grammar;
static syngt::UndoRedo undoRedo;
static std::unique_ptr<syngt::graphics::DrawObjectList> drawObjects;
static int activeNTIndex = 0;
static syngt::SelectionMask selectionMask;
static bool showAbout = false;
static bool showHelp = false;
static bool showRecursionWindow = false;
static std::vector<syngt::RecursionResult> recursionResults;
static bool showExtractRuleDialog = false;
static char extractNewNTName[256] = "";
static std::string s_extractCoverStr;   // display: serialised cover
static std::string s_extractActiveNT;
static const syngt::RETree* s_extractCoverNode = nullptr; // pointer into active NT's tree

// Editor state
static int activeLeftTab = 1; // 0 = Grammar Editor, 1 = Syntax Diagram
static bool isDragging = false;
static ImVec2 lastMousePos;
static float diagramScale = 1.0f;
// Stable bounds — updated only when not dragging to prevent offset jumps during drag
static int s_stableMinX = 0, s_stableMinY = 0;
static int s_stableMaxX = 100, s_stableMaxY = 100;
static bool s_stableBoundsValid = false;
static syngt::graphics::DrawObject* hoveredObject = nullptr;

// Diagram layout cache: NT name → serialized layout text
static std::unordered_map<std::string, std::string> ntDiagramLayouts;
static std::string s_currentDiagramNT; // which NT's diagram is in drawObjects
static syngt::graphics::DrawObject* contextMenuObject = nullptr;
static bool showAddTerminalDialog = false;
static bool showAddNonTerminalDialog = false;
static bool showAddReferenceDialog = false;
static bool showEditDialog = false;
static char newSymbolName[256] = "";
static bool useOrOperator = false;

// Layout sizes
static float leftWidth = 800.0f;
static float rightPanelButtonsHeight = 430.0f;

// Editor state
static bool isBoxSelecting = false;
static ImVec2 boxSelectStart;
static bool skipHistorySave = false;

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

#ifdef _WIN32
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

    std::string OpenGEditFileDialog() {
        char filename[MAX_PATH] = "";
        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = "GEdit Files (*.grw)\0*.grw\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = "grw";

        if (GetOpenFileNameA(&ofn)) {
            return std::string(filename);
        }
        return "";
    }
#else
    std::string OpenFileDialog() {
        char filename[MAX_PATH] = "";
        FILE* f = popen("zenity --file-selection --file-filter='Grammar files | *.grm' --file-filter='All files | *' 2>/dev/null", "r");
        if (f && fgets(filename, sizeof(filename), f)) {
            pclose(f);
            size_t len = strlen(filename);
            if (len > 0 && filename[len-1] == '\n')
                filename[len-1] = '\0';
            return std::string(filename);
        }
        if (f) pclose(f);
        
        printf("Enter filename to open (.grm): ");
        if (fgets(filename, sizeof(filename), stdin)) {
            size_t len = strlen(filename);
            if (len > 0 && filename[len-1] == '\n')
                filename[len-1] = '\0';
            return std::string(filename);
        }
        return "";
    }

    std::string SaveFileDialog() {
        char filename[MAX_PATH] = "";
        FILE* f = popen("zenity --file-selection --save --file-filter='Grammar files | *.grm' --file-filter='All files | *' 2>/dev/null", "r");
        if (f && fgets(filename, sizeof(filename), f)) {
            pclose(f);
            size_t len = strlen(filename);
            if (len > 0 && filename[len-1] == '\n')
                filename[len-1] = '\0';
            return std::string(filename);
        }
        if (f) pclose(f);
        
        printf("Enter filename to save (.grm): ");
        if (fgets(filename, sizeof(filename), stdin)) {
            size_t len = strlen(filename);
            if (len > 0 && filename[len-1] == '\n')
                filename[len-1] = '\0';
            return std::string(filename);
        }
        return "";
    }

    std::string OpenGEditFileDialog() {
        char filename[MAX_PATH] = "";
        FILE* f = popen("zenity --file-selection --file-filter='GEdit files | *.grw' --file-filter='All files | *' 2>/dev/null", "r");
        if (f && fgets(filename, sizeof(filename), f)) {
            pclose(f);
            size_t len = strlen(filename);
            if (len > 0 && filename[len-1] == '\n')
                filename[len-1] = '\0';
            return std::string(filename);
        }
        if (f) pclose(f);

        printf("Enter filename to open (.grw): ");
        if (fgets(filename, sizeof(filename), stdin)) {
            size_t len = strlen(filename);
            if (len > 0 && filename[len-1] == '\n')
                filename[len-1] = '\0';
            return std::string(filename);
        }
        return "";
    }
#endif

void LoadRuleToEditor() {
    if (!grammar) {
        ruleText[0] = '\0';
        return;
    }
    
    auto nts = grammar->getNonTerminals();
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
        ruleText[0] = '\0';
        return;
    }
    
    std::string ntName = nts[activeNTIndex];
    syngt::NTListItem* item = grammar->getNTItem(ntName);
    
    if (!item) {
        strcpy_s(ruleText, sizeof(ruleText), "eps");
        return;
    }
    
    if (item->hasRoot()) {
        std::string value = item->value();
        if (value.size() < sizeof(ruleText)) {
            strcpy_s(ruleText, sizeof(ruleText), value.c_str());
        }
    } else {
        strcpy_s(ruleText, sizeof(ruleText), "eps");
    }
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

// Syntax Grammar
void DrawArrowhead(ImDrawList* drawList, ImVec2 from, ImVec2 to, ImU32 color, float scale) {
    ImVec2 dir = ImVec2(to.x - from.x, to.y - from.y);
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (length < 0.001f) return;

    dir.x /= length;
    dir.y /= length;

    const float arrowSize = 8.0f * scale;
    const float arrowAngle = 0.4f;

    ImVec2 tip = ImVec2(to.x - dir.x * 2 * scale, to.y - dir.y * 2 * scale);

    ImVec2 perp = ImVec2(-dir.y, dir.x);

    ImVec2 left = ImVec2(
        tip.x - dir.x * arrowSize + perp.x * arrowSize * arrowAngle,
        tip.y - dir.y * arrowSize + perp.y * arrowSize * arrowAngle
    );
    ImVec2 right = ImVec2(
        tip.x - dir.x * arrowSize - perp.x * arrowSize * arrowAngle,
        tip.y - dir.y * arrowSize - perp.y * arrowSize * arrowAngle
    );

    drawList->AddTriangleFilled(tip, left, right, color);
}

void RenderDiagram(ImDrawList* drawList, const ImVec2& offset, float scale) {
    if (!drawObjects || drawObjects->count() == 0) return;

    const ImU32 lineColor = IM_COL32(0, 0, 0, 255);
    const ImU32 selectedColor = IM_COL32(25, 55, 95, 255);
    const ImU32 hoveredColor = IM_COL32(100, 150, 255, 255);
    const ImU32 textColor = IM_COL32(80, 80, 80, 255);
    
    // DEBUG
    // static bool debugPrinted = false;
    // if (!debugPrinted && drawObjects->count() > 0) {
    //     ClearOutput();
    //     AppendOutput("=== Arrow directions debug with coordinates ===\n");
    //     for (int i = 0; i < drawObjects->count(); ++i) {
    //         syngt::graphics::DrawObject* obj = (*drawObjects)[i];
    //         if (!obj) continue;
            
    //         char buf[512];
    //         snprintf(buf, sizeof(buf), "Object %d: type=%d, pos=(%d,%d)\n", 
    //                  i, obj->getType(), obj->x(), obj->y());
    //         AppendOutput(buf);
            
    //         syngt::graphics::Arrow* arrow = obj->inArrow();
    //         if (arrow && arrow->getFromDO()) {
    //             auto* from = arrow->getFromDO();
    //             snprintf(buf, sizeof(buf), "  inArrow: ward=%d, from=(%d,%d) to=(%d,%d) ", 
    //                      arrow->ward(), from->endX(), from->y(), obj->x(), obj->y());
    //             AppendOutput(buf);
                
    //             int dx = obj->x() - from->endX();
    //             int dy = obj->y() - from->y();
    //             snprintf(buf, sizeof(buf), "delta=(%d,%d)\n", dx, dy);
    //             AppendOutput(buf);
    //         }
            
    //         if (obj->getType() == syngt::graphics::ctDrawObjectPoint) {
    //             auto point = dynamic_cast<syngt::graphics::DrawObjectPoint*>(obj);
    //             if (point && point->secondInArrow()) {
    //                 auto* arrow2 = point->secondInArrow();
    //                 if (arrow2->getFromDO()) {
    //                     auto* from = arrow2->getFromDO();
    //                     snprintf(buf, sizeof(buf), "  secondInArrow: ward=%d, from=(%d,%d) to=(%d,%d) ", 
    //                              arrow2->ward(), from->endX(), from->y(), obj->x(), obj->y());
    //                     AppendOutput(buf);
                        
    //                     int dx = obj->x() - from->endX();
    //                     int dy = obj->y() - from->y();
    //                     snprintf(buf, sizeof(buf), "delta=(%d,%d)\n", dx, dy);
    //                     AppendOutput(buf);
    //                 }
    //             }
    //         }
    //     }
    //     debugPrinted = true;
    // }
    
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
        
        syngt::graphics::Arrow* arrow = obj->inArrow();
        if (arrow && arrow->getFromDO()) {
            syngt::graphics::DrawObject* from = dynamic_cast<syngt::graphics::DrawObject*>(arrow->getFromDO());
            if (from) {
                float x1 = offset.x + from->endX() * scale;
                float y1 = offset.y + from->y() * scale;

                ImVec2 fromPos = ImVec2(x1, y1);
                ImVec2 toPos = ImVec2(x, y);

                drawList->AddLine(fromPos, toPos, currentColor, thickness);

                int ward = arrow->ward();
                if (ward > 0) {  // cwFORWARD: arrowhead at destination
                    DrawArrowhead(drawList, fromPos, toPos, currentColor, scale);
                } else if (ward < 0) {  // cwBACKWARD: arrowhead at source
                    DrawArrowhead(drawList, toPos, fromPos, currentColor, scale);
                }

                auto* semArrow = dynamic_cast<syngt::graphics::SemanticArrow*>(arrow);
                if (semArrow && grammar) {
                    syngt::SemanticIDList* semList = semArrow->getSemantics();
                    if (semList && !semList->isEmpty()) {
                        std::string label;
                        for (int si = 0; si < semList->count(); si++) {
                            if (si > 0) label += ',';
                            label += grammar->semantics()->getString(semList->getID(si));
                        }
                        // Pascal: text drawn near arrow start (fromPos), above the line.
                        // For cwBACKWARD the spike is at source, so shift right past it.
                        float textX = fromPos.x + syngt::graphics::SpaceLength * scale;
                        if (arrow->ward() < 0) {  // cwBACKWARD: spike at source
                            textX += syngt::graphics::SpikeLength * scale;
                        }
                        float textY = fromPos.y;
                        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
                        float fontSize = ImGui::GetFontSize() * scale;
                        textSize.y *= scale;
                        const ImU32 semColor = IM_COL32(0, 100, 180, 255);
                        drawList->AddText(ImGui::GetFont(), fontSize,
                            ImVec2(textX, textY - textSize.y - 2.0f * scale),
                            semColor, label.c_str());
                    }
                }
            }
        }

        int type = obj->getType();
        if (type == syngt::graphics::ctDrawObjectPoint) {
            auto point = dynamic_cast<syngt::graphics::DrawObjectPoint*>(obj);
            if (point && point->secondInArrow()) {
                syngt::graphics::Arrow* arrow2 = point->secondInArrow();
                if (arrow2 && arrow2->getFromDO()) {
                    syngt::graphics::DrawObject* from = dynamic_cast<syngt::graphics::DrawObject*>(arrow2->getFromDO());
                    if (from) {
                        float x1 = offset.x + from->endX() * scale;
                        float y1 = offset.y + from->y() * scale;

                        ImVec2 fromPos = ImVec2(x1, y1);
                        ImVec2 toPos = ImVec2(x, y);

                        drawList->AddLine(fromPos, toPos, currentColor, thickness);

                        int ward = arrow2->ward();
                        if (ward > 0) {  // cwFORWARD: arrowhead at destination
                            DrawArrowhead(drawList, fromPos, toPos, currentColor, scale);
                        } else if (ward < 0) {  // cwBACKWARD: arrowhead at source
                            DrawArrowhead(drawList, toPos, fromPos, currentColor, scale);
                        }
                    }
                }
            }
        }
    }
    
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
                float fontSize = ImGui::GetFontSize() * scale;
                ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
                textSize.x *= scale;
                textSize.y *= scale;
                drawList->AddText(ImGui::GetFont(), fontSize,
                    ImVec2(x + w - textSize.x/2, y - textSize.y/2), textColor, name.c_str());
            }
        }
        else if (type == syngt::graphics::ctDrawObjectNonTerminal) {
            float w = obj->getLength() * scale;
            float h = 40.0f * scale;
            drawList->AddRect(ImVec2(x, y - h/2), ImVec2(x + w, y + h/2), currentColor, 0.0f, 0, thickness);

            // Double border for macro NTs
            auto leaf = dynamic_cast<syngt::graphics::DrawObjectLeaf*>(obj);
            if (leaf && grammar) {
                syngt::NTListItem* ntItem = grammar->getNTItem(leaf->name());
                if (ntItem && ntItem->isMacro()) {
                    float gap = 3.0f * scale;
                    drawList->AddRect(
                        ImVec2(x + gap, y - h/2 + gap),
                        ImVec2(x + w - gap, y + h/2 - gap),
                        currentColor, 0.0f, 0, thickness
                    );
                }
            }

            if (leaf) {
                std::string name = leaf->name();
                float fontSize = ImGui::GetFontSize() * scale;
                ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
                textSize.x *= scale;
                textSize.y *= scale;
                drawList->AddText(ImGui::GetFont(), fontSize,
                    ImVec2(x + w/2 - textSize.x/2, y - textSize.y/2), textColor, name.c_str());
            }
        }
        else if (type == syngt::graphics::ctDrawObjectFirst) {
            drawList->AddTriangleFilled(
                ImVec2(x, y - 10 * scale),
                ImVec2(x, y + 10 * scale),
                ImVec2(x + 15 * scale, y),
                currentColor
            );
        }
        else if (type == syngt::graphics::ctDrawObjectLast) {
            drawList->AddTriangleFilled(
                ImVec2(x + 15 * scale, y - 10 * scale),
                ImVec2(x + 15 * scale, y + 10 * scale),
                ImVec2(x, y),
                currentColor
            );
        }
        else if (type == syngt::graphics::ctDrawObjectPoint ||
                 type == syngt::graphics::ctDrawObjectExtendedPoint) {
            float radius = ((obj->selected() || obj == hoveredObject) ? 4.0f : 3.0f) * scale;
            drawList->AddCircleFilled(ImVec2(x, y), radius, currentColor);
        }
    }
}

void UpdateDiagram() {
    if (!grammar) {
        drawObjects.reset();
        s_stableBoundsValid = false;
        return;
    }

    // Save current diagram layout before switching to a new NT
    SaveCurrentDiagramToCache();

    auto nts = grammar->getNonTerminals();
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
        activeNTIndex = 0;
    }

    if (nts.empty()) {
        drawObjects.reset();
        s_currentDiagramNT.clear();
        return;
    }

    std::string activeNT = nts[activeNTIndex];

    if (activeNT == "EOGram" || activeNT == "EOGram!") {
        drawObjects.reset();
        s_currentDiagramNT.clear();
        ClearOutput();
        AppendOutput("EOGram! is not a displayable rule\n");
        return;
    }

    syngt::NTListItem* item = grammar->getNTItem(activeNT);
    if (!item) {
        drawObjects.reset();
        s_currentDiagramNT.clear();
        ClearOutput();
        AppendOutput("Cannot find rule for '");
        AppendOutput(activeNT.c_str());
        AppendOutput("'\n");
        return;
    }

    // Try to restore saved layout for this NT first
    auto layoutIt = ntDiagramLayouts.find(activeNT);
    if (layoutIt != ntDiagramLayouts.end()) {
        auto restoredDO = std::make_unique<syngt::graphics::DrawObjectList>(grammar.get());
        std::istringstream iss(layoutIt->second);
        if (restoredDO->loadLayout(iss, grammar.get())) {
            drawObjects = std::move(restoredDO);
            s_currentDiagramNT = activeNT;
            s_stableBoundsValid = false;
            LoadRuleToEditor();
            return;
        }
        // If load failed, fall through to Creator
    }

    if (!item->hasRoot()) {
        drawObjects = std::make_unique<syngt::graphics::DrawObjectList>(grammar.get());

        auto firstDO = std::make_unique<syngt::graphics::DrawObjectFirst>();
        firstDO->place();

        syngt::graphics::DrawObjectFirst* firstPtr = firstDO.get();
        drawObjects->add(std::move(firstDO));

        auto lastDO = std::make_unique<syngt::graphics::DrawObjectLast>();
        auto arrow = std::make_unique<syngt::graphics::Arrow>(syngt::graphics::cwFORWARD, firstPtr);
        lastDO->setInArrow(std::move(arrow));

        int lastX = firstPtr->endX() + 20;
        int lastY = firstPtr->y();
        lastDO->setPositionForCreator(lastX, lastY);

        drawObjects->add(std::move(lastDO));

        drawObjects->setWidth(lastX + syngt::graphics::NS_Radius + syngt::graphics::HorizontalSkipFromBorder);
        drawObjects->setHeight(firstPtr->y() + syngt::graphics::VerticalSkipFromBorder + syngt::graphics::NS_Radius);

        s_currentDiagramNT = activeNT;
        LoadRuleToEditor();
        return;
    }

    try {
        drawObjects = std::make_unique<syngt::graphics::DrawObjectList>(grammar.get());
        syngt::Creator::createDrawObjects(drawObjects.get(), item->root());
    } catch (const std::exception& e) {
        drawObjects.reset();
        s_currentDiagramNT.clear();
        ClearOutput();
        AppendOutput("Error creating diagram:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
        return;
    }

    s_currentDiagramNT = activeNT;
    // Invalidate stable bounds so they're recomputed on next frame
    s_stableBoundsValid = false;

    LoadRuleToEditor();
}

syngt::graphics::DrawObject* FindDrawObjectAt(const ImVec2& pos, const ImVec2& offset, float scale) {
    if (!drawObjects || drawObjects->count() == 0) return nullptr;

    int logicX = static_cast<int>((pos.x - offset.x) / scale);
    int logicY = static_cast<int>((pos.y - offset.y) / scale);

    for (int i = drawObjects->count() - 1; i >= 0; --i) {
        syngt::graphics::DrawObject* obj = (*drawObjects)[i];
        if (!obj) continue;

        if (obj->internalPoint(logicX, logicY)) {
            return obj;
        }
    }

    return nullptr;
}

// Undo/Redo
void SaveCurrentState() {
    if (!grammar) return;
    
    if (skipHistorySave) {
        return;
    }
    
    auto names = grammar->getNonTerminals();
    std::vector<std::string> values;
    std::vector<bool> macroFlags;

    for (const auto& name : names) {
        auto* item = grammar->getNTItem(name);
        if (item && item->hasRoot()) {
            values.push_back(item->value());
        } else {
            values.push_back("eps");
        }
        macroFlags.push_back(item ? item->isMacro() : false);
    }

    undoRedo.addState(names, values, macroFlags, activeNTIndex, selectionMask, grammarText);
}

void SaveCurrentDiagramToCache() {
    if (drawObjects && !s_currentDiagramNT.empty()) {
        std::ostringstream oss;
        drawObjects->saveLayout(oss);
        ntDiagramLayouts[s_currentDiagramNT] = oss.str();
    }
}

void ClearDiagramLayouts() {
    ntDiagramLayouts.clear();
    s_currentDiagramNT.clear();
}

void RebuildGrammarFromText() {
    try {
        std::string tempFile = "temp_parse.grm";
        
        std::ofstream out(tempFile);
        if (!out) return;
        out << grammarText;
        out.close();
        
        grammar = std::make_unique<syngt::Grammar>();
        grammar->load(tempFile);
        std::remove(tempFile.c_str());
        
        auto nts = grammar->getNonTerminals();
        if (nts.empty()) {
            activeNTIndex = -1;
        } else {
            if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
                activeNTIndex = 0;
            }
        }

        ClearDiagramLayouts();
        UpdateDiagram();

        if (!skipHistorySave) {
            SaveCurrentState();
        }
        
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Parse error: ");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void SyncGrammarFromDiagram() {
    if (!grammar || !drawObjects) return;
    
    // TODO: Implement reverse conversion from diagram to grammar text
    // This is complex and requires analyzing the DrawObject graph structure
    // For now, just save the current state
    SaveCurrentState();
}

static std::string buildRestoredText(const std::vector<std::string>& names,
                                     const std::vector<std::string>& values,
                                     const std::vector<bool>& macroFlags) {
    std::string text;
    for (size_t i = 0; i < names.size() && i < values.size(); ++i) {
        // Skip NTs that have no real rule (value == "eps" means no root).
        // Writing "ntName : eps." would cause the parser to add a spurious
        // non-terminal named "eps", shifting all indices.
        if (values[i] == "eps" || values[i].empty()) {
            continue;
        }
        text += names[i] + " : " + values[i];
        if (values[i].back() != '.') {
            text += ".";
        }
        text += "\n";
    }
    // Append AUXILIARYNOTIONS if any macros present
    std::vector<std::string> macroNames;
    for (size_t i = 0; i < names.size() && i < macroFlags.size(); ++i) {
        if (macroFlags[i]) macroNames.push_back(names[i]);
    }
    if (!macroNames.empty()) {
        text += "AUXILIARYNOTIONS:";
        for (const auto& mn : macroNames) text += " " + mn;
        text += ".\n";
    }
    text += "EOGram!\n";
    return text;
}

void Undo() {
    if (!undoRedo.canUndo()) {
        ClearOutput();
        AppendOutput("Nothing to undo\n");
        return;
    }

    if (grammar) {
        std::vector<std::string> names;
        std::vector<std::string> values;
        std::vector<bool> macroFlags;
        int index = activeNTIndex;
        syngt::SelectionMask selection = selectionMask;
        std::string restoredGrammarText;

        undoRedo.stepBack(names, values, macroFlags, index, selection, restoredGrammarText);

        if (restoredGrammarText.empty()) {
            restoredGrammarText = buildRestoredText(names, values, macroFlags);
        }

        if (restoredGrammarText.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), restoredGrammarText.c_str());
            grammarWidgetVersion++;
        }

        activeNTIndex = index;
        selectionMask = selection;

        skipHistorySave = true;
        RebuildGrammarFromText();
        skipHistorySave = false;

        ClearOutput();
        AppendOutput("Undo successful\n");
    }
}

void Redo() {
    if (!undoRedo.canRedo()) {
        ClearOutput();
        AppendOutput("Nothing to redo\n");
        return;
    }

    if (grammar) {
        std::vector<std::string> names;
        std::vector<std::string> values;
        std::vector<bool> macroFlags;
        int index = activeNTIndex;
        syngt::SelectionMask selection = selectionMask;
        std::string restoredGrammarText;

        undoRedo.stepForward(names, values, macroFlags, index, selection, restoredGrammarText);

        if (restoredGrammarText.empty()) {
            restoredGrammarText = buildRestoredText(names, values, macroFlags);
        }

        if (restoredGrammarText.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), restoredGrammarText.c_str());
            grammarWidgetVersion++;
        }

        activeNTIndex = index;
        selectionMask = selection;

        skipHistorySave = true;
        RebuildGrammarFromText();
        skipHistorySave = false;

        ClearOutput();
        AppendOutput("Redo successful\n");
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
    
    for (const auto& term : terminals) {
        if (term == newName) {
            ClearOutput();
            AppendOutput("Terminal '");
            AppendOutput(newName.c_str());
            AppendOutput("' already exists!\n");
            return false;
        }
    }
    
    std::string oldName = terminals[oldId];
    std::string text = grammarText;
    
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
    
    for (const auto& nt : nts) {
        if (nt == newName) {
            ClearOutput();
            AppendOutput("Non-terminal '");
            AppendOutput(newName.c_str());
            AppendOutput("' already exists!\n");
            return false;
        }
    }
    
    std::string oldName = nts[oldId];
    std::string text = grammarText;
    
    size_t pos = 0;
    while ((pos = text.find(oldName, pos)) != std::string::npos) {
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

void DeleteNonTerminal(int id) {
    if (!grammar) return;
    auto nts = grammar->getNonTerminals();
    if (id < 0 || id >= static_cast<int>(nts.size())) return;

    std::string ntName = nts[id];
    std::string text = grammarText;

    // Find "ntName :" or "ntName:" and the closing "." for that rule
    size_t ruleStart = std::string::npos;
    size_t searchPos = 0;
    while (searchPos < text.size()) {
        size_t found = text.find(ntName, searchPos);
        if (found == std::string::npos) break;
        bool atStart = (found == 0 || (!isalnum(text[found - 1]) && text[found - 1] != '_'));
        size_t afterName = found + ntName.size();
        // Skip whitespace
        size_t ws = afterName;
        while (ws < text.size() && (text[ws] == ' ' || text[ws] == '\t')) ws++;
        if (atStart && ws < text.size() && text[ws] == ':') {
            ruleStart = found;
            break;
        }
        searchPos = found + 1;
    }

    if (ruleStart == std::string::npos) {
        ClearOutput();
        AppendOutput("Could not find rule for '");
        AppendOutput(ntName.c_str());
        AppendOutput("'\n");
        return;
    }

    // Find the closing '.' — skip nested parens/brackets and comments
    size_t colonPos = text.find(':', ruleStart);
    if (colonPos == std::string::npos) return;

    size_t dotPos = std::string::npos;
    int depth = 0;
    for (size_t i = colonPos + 1; i < text.size(); i++) {
        char c = text[i];
        if (c == '(' || c == '[') depth++;
        else if (c == ')' || c == ']') depth--;
        else if (c == '{') {
            // skip comment
            while (i < text.size() && text[i] != '}') i++;
        } else if (c == '.' && depth == 0) {
            dotPos = i;
            break;
        }
    }
    if (dotPos == std::string::npos) return;

    // Erase from ruleStart to dotPos+1, and any trailing newline
    size_t eraseEnd = dotPos + 1;
    while (eraseEnd < text.size() && (text[eraseEnd] == '\r' || text[eraseEnd] == '\n'))
        eraseEnd++;
    // Also trim leading newlines before ruleStart that belong to the blank line
    size_t eraseStart = ruleStart;
    while (eraseStart > 0 && (text[eraseStart - 1] == '\r' || text[eraseStart - 1] == '\n'))
        eraseStart--;
    // Keep one newline separator if there's content before
    if (eraseStart > 0) eraseStart++; // leave one '\n' from previous line

    text.erase(eraseStart, eraseEnd - eraseStart);

    if (text.size() < sizeof(grammarText)) {
        strcpy_s(grammarText, sizeof(grammarText), text.c_str());
        grammarWidgetVersion++;
        if (activeNTIndex >= id && activeNTIndex > 0)
            activeNTIndex--;
        RebuildGrammarFromText();
        SaveCurrentState();
        ClearOutput();
        AppendOutput("Deleted non-terminal '");
        AppendOutput(ntName.c_str());
        AppendOutput("'\n");
    }
}

void DeleteTerminal(int id) {
    if (!grammar) return;
    auto terminals = grammar->getTerminals();
    if (id < 0 || id >= static_cast<int>(terminals.size())) return;

    std::string termName = terminals[id];
    std::string quoted = "'" + termName + "'";
    std::string text = grammarText;

    size_t pos = 0;
    while ((pos = text.find(quoted, pos)) != std::string::npos) {
        size_t eraseStart = pos;
        size_t eraseEnd = pos + quoted.size();
        // Try to eat a preceding ", " separator
        size_t before = eraseStart;
        while (before > 0 && text[before - 1] == ' ') before--;
        if (before > 0 && text[before - 1] == ',') {
            eraseStart = before - 1;
        } else {
            // Try to eat a trailing " , " separator
            size_t after = eraseEnd;
            while (after < text.size() && text[after] == ' ') after++;
            if (after < text.size() && text[after] == ',') {
                eraseEnd = after + 1;
            }
        }
        text.erase(eraseStart, eraseEnd - eraseStart);
        pos = eraseStart;
    }

    if (text.size() < sizeof(grammarText)) {
        strcpy_s(grammarText, sizeof(grammarText), text.c_str());
        grammarWidgetVersion++;
        RebuildGrammarFromText();
        SaveCurrentState();
        ClearOutput();
        AppendOutput("Deleted terminal '");
        AppendOutput(termName.c_str());
        AppendOutput("'\n");
    }
}

void DeleteSemantic(const std::string& name) {
    if (!grammar) return;
    // name already includes '$' prefix (stored as "$add" etc.)
    std::string token = name;
    std::string text = grammarText;

    size_t pos = 0;
    while ((pos = text.find(token, pos)) != std::string::npos) {
        // Check word boundary after token
        size_t afterToken = pos + token.size();
        bool wordEnd = (afterToken >= text.size() || (!isalnum(text[afterToken]) && text[afterToken] != '_'));
        if (!wordEnd) { pos++; continue; }

        size_t eraseStart = pos;
        size_t eraseEnd = afterToken;
        // Eat preceding ", " separator
        size_t before = eraseStart;
        while (before > 0 && text[before - 1] == ' ') before--;
        if (before > 0 && text[before - 1] == ',') {
            eraseStart = before - 1;
        }
        text.erase(eraseStart, eraseEnd - eraseStart);
        pos = eraseStart;
    }

    if (text.size() < sizeof(grammarText)) {
        strcpy_s(grammarText, sizeof(grammarText), text.c_str());
        grammarWidgetVersion++;
        RebuildGrammarFromText();
        SaveCurrentState();
        ClearOutput();
        AppendOutput("Deleted semantic '");
        AppendOutput(name.c_str());
        AppendOutput("'\n");
    }
}

void DeleteSelectedObjects() {
    if (!drawObjects || !grammar) {
        ClearOutput();
        AppendOutput("No diagram or grammar loaded!\n");
        return;
    }
    
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
    
    struct SymbolOccurrence {
        std::string name;
        int type;
        int occurrence;
    };
    
    std::vector<SymbolOccurrence> occurrencesToDelete;
    
    for (auto* obj : toDelete) {
        auto leaf = dynamic_cast<syngt::graphics::DrawObjectLeaf*>(obj);
        if (!leaf) continue;
        
        std::string name = leaf->name();
        int type = obj->getType();
        
        if (name.empty() || name == "eps") continue;
        
        int objIndex = -1;
        for (int i = 0; i < drawObjects->count(); ++i) {
            if ((*drawObjects)[i] == obj) {
                objIndex = i;
                break;
            }
        }
        
        if (objIndex < 0) continue;
        
        int occurrence = 0;
        for (int i = 0; i < objIndex; ++i) {
            auto checkLeaf = dynamic_cast<syngt::graphics::DrawObjectLeaf*>((*drawObjects)[i]);
            if (checkLeaf && 
                (*drawObjects)[i]->getType() == type &&
                checkLeaf->name() == name) {
                occurrence++;
            }
        }
        
        occurrencesToDelete.push_back({name, type, occurrence});
    }
    
    if (occurrencesToDelete.empty()) {
        AppendOutput("Only structural elements selected (cannot delete)\n");
        drawObjects->unselectAll();
        return;
    }
    
    std::map<std::pair<std::string, int>, std::set<int>> toDeleteMap;
    for (const auto& occ : occurrencesToDelete) {
        toDeleteMap[{occ.name, occ.type}].insert(occ.occurrence);
        
        AppendOutput("  - ");
        AppendOutput(occ.name.c_str());
        char buf2[32];
        snprintf(buf2, sizeof(buf2), " (occurrence #%d)", occ.occurrence);
        AppendOutput(buf2);
        AppendOutput("\n");
    }
    
    auto nts = grammar->getNonTerminals();
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
        AppendOutput("No active non-terminal!\n");
        return;
    }
    
    std::string activeNT = nts[activeNTIndex];
    std::string text = grammarText;
    
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
    
    size_t colonPos = text.find(":", ruleStart);
    size_t ruleEnd = text.find(".", colonPos);
    
    if (colonPos == std::string::npos || ruleEnd == std::string::npos) {
        AppendOutput("Malformed rule!\n");
        return;
    }
    
    std::string ruleBody = text.substr(colonPos + 1, ruleEnd - colonPos - 1);
    
    AppendOutput("\nOriginal rule body: ");
    AppendOutput(ruleBody.c_str());
    AppendOutput("\n");
    
    for (const auto& [key, occurrences] : toDeleteMap) {
        const std::string& name = key.first;
        int type = key.second;
        
        struct Position {
            size_t start;
            size_t end;
            int occurrence;
        };
        std::vector<Position> positions;
        
        if (type == syngt::graphics::ctDrawObjectTerminal) {
            // Для терминалов ищем 'name' или "name"
            std::string pattern1 = "'" + name + "'";
            std::string pattern2 = "\"" + name + "\"";
            
            size_t pos = 0;
            int currentOcc = 0;
            while (pos < ruleBody.length()) {
                size_t found1 = ruleBody.find(pattern1, pos);
                size_t found2 = ruleBody.find(pattern2, pos);
                size_t found = std::min(found1, found2);
                
                if (found == std::string::npos) break;
                
                size_t patternLen = (found == found1) ? pattern1.length() : pattern2.length();
                positions.push_back({found, found + patternLen, currentOcc});
                currentOcc++;
                pos = found + 1;
            }
        } else if (type == syngt::graphics::ctDrawObjectNonTerminal) {
            size_t pos = 0;
            int currentOcc = 0;
            while ((pos = ruleBody.find(name, pos)) != std::string::npos) {
                bool isWordStart = (pos == 0) || !isalnum(ruleBody[pos - 1]);
                bool isWordEnd = (pos + name.length() >= ruleBody.length()) || 
                                 !isalnum(ruleBody[pos + name.length()]);
                
                if (isWordStart && isWordEnd) {
                    positions.push_back({pos, pos + name.length(), currentOcc});
                    currentOcc++;
                }
                pos++;
            }
        }
        
        for (auto it = positions.rbegin(); it != positions.rend(); ++it) {
            if (occurrences.count(it->occurrence) > 0) {
                size_t start = it->start;
                size_t end = it->end;
                
                while (end < ruleBody.length() && isspace(ruleBody[end])) {
                    end++;
                }

                if (end < ruleBody.length() && ruleBody[end] == ',') {
                    // Following AND separator: consume it ("sym," → "")
                    end++;
                    while (end < ruleBody.length() && isspace(ruleBody[end])) {
                        end++;
                    }
                } else {
                    // No following ',' (either ';' or end of alternative):
                    // consume the preceding separator instead (",sym" or ";sym" → "")
                    if (start > 0) {
                        size_t beforePos = start - 1;
                        while (beforePos > 0 && isspace(ruleBody[beforePos])) {
                            beforePos--;
                        }
                        if (ruleBody[beforePos] == ',' || ruleBody[beforePos] == ';') {
                            start = beforePos;
                            while (start > 0 && isspace(ruleBody[start - 1])) {
                                start--;
                            }
                        }
                    }
                }

                ruleBody.erase(start, end - start);
            }
        }
    }
    
    size_t firstNonSpace = ruleBody.find_first_not_of(" \t\n\r");
    size_t lastNonSpace = ruleBody.find_last_not_of(" \t\n\r");
    
    if (firstNonSpace != std::string::npos && lastNonSpace != std::string::npos) {
        ruleBody = ruleBody.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
    } else {
        ruleBody = "";
    }
    
    if (!ruleBody.empty() && (ruleBody[0] == ',' || ruleBody[0] == ';')) {
        ruleBody = ruleBody.substr(1);
        firstNonSpace = ruleBody.find_first_not_of(" \t\n\r");
        if (firstNonSpace != std::string::npos) {
            ruleBody = ruleBody.substr(firstNonSpace);
        }
    }
    
    if (!ruleBody.empty() && (ruleBody[ruleBody.length()-1] == ',' || ruleBody[ruleBody.length()-1] == ';')) {
        ruleBody = ruleBody.substr(0, ruleBody.length() - 1);
        lastNonSpace = ruleBody.find_last_not_of(" \t\n\r");
        if (lastNonSpace != std::string::npos) {
            ruleBody = ruleBody.substr(0, lastNonSpace + 1);
        }
    }
    
    if (ruleBody.empty()) {
        ruleBody = " eps";
    }
    
    AppendOutput("New rule body: ");
    AppendOutput(ruleBody.c_str());
    AppendOutput("\n");
    
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
    ClearOutput();
    
    if (!grammar) {
        return;
    }
    
    auto nts = grammar->getNonTerminals();
    
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
        return;
    }
    
    std::string text = grammarText;
    std::string ntName = nts[activeNTIndex];
    
    size_t pos = text.find(ntName + " :");
    if (pos == std::string::npos) pos = text.find(ntName + ":");
    
    if (pos == std::string::npos) {
        return;
    }
    
    size_t colonPos = text.find(":", pos);
    size_t dotPos = text.find(".", colonPos);
    
    if (colonPos == std::string::npos || dotPos == std::string::npos) {
        return;
    }
    
    std::string ruleBody = text.substr(colonPos + 1, dotPos - colonPos - 1);
    
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
    
    std::string newBody;
    std::string terminalStr = "'" + name + "'";
    
    if (trimmedBody.empty()) {
        newBody = " " + terminalStr;
    } else if (useOrOperator) {
        newBody = " " + trimmedBody + " ; " + terminalStr;
    } else {
        size_t lastSemicolon = trimmedBody.rfind(';');
        
        if (lastSemicolon != std::string::npos) {
            std::string beforeLast = trimmedBody.substr(0, lastSemicolon + 1);
            std::string lastAlt = trimmedBody.substr(lastSemicolon + 1);
            
            size_t altStart = lastAlt.find_first_not_of(" \t\n\r");
            if (altStart != std::string::npos) {
                lastAlt = lastAlt.substr(altStart);
            }
            
            newBody = " " + beforeLast + " " + lastAlt + ", " + terminalStr;
        } else {
            newBody = " " + trimmedBody + ", " + terminalStr;
        }
    }
    
    text.replace(colonPos + 1, dotPos - colonPos - 1, newBody);
    
    if (text.size() >= sizeof(grammarText)) {
        return;
    }
    
    strcpy_s(grammarText, sizeof(grammarText), text.c_str());
    
    RebuildGrammarFromText();
    
    SaveCurrentState();
    
    UpdateDiagram();
    
    ClearOutput();
    AppendOutput("Added '");
    AppendOutput(name.c_str());
    AppendOutput("' ");
    AppendOutput(useOrOperator ? "(OR)" : "(AND)");
    AppendOutput("\n");
}

void AddNonTerminalReference(const std::string& ntRef, bool useOr) {
    if (!grammar) {
        ClearOutput();
        AppendOutput("No grammar loaded!\n");
        return;
    }
    
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
    
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
        ClearOutput();
        AppendOutput("No active non-terminal!\n");
        return;
    }
    
    std::string text = grammarText;
    std::string ntName = nts[activeNTIndex];
    
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
    
    size_t start = ruleBody.find_first_not_of(" \t\n\r");
    size_t end = ruleBody.find_last_not_of(" \t\n\r");
    
    std::string trimmedBody;
    if (start != std::string::npos && end != std::string::npos) {
        trimmedBody = ruleBody.substr(start, end - start + 1);
    } else {
        trimmedBody = "";
    }
    
    std::string separator = useOr ? " ; " : ", ";
    std::string newBody;
    
    if (trimmedBody.empty()) {
        newBody = " " + ntRef;
    } else {
        newBody = " " + trimmedBody + separator + ntRef;
    }
    
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
    
    std::string text = grammarText;
    
    size_t eogramPos = text.find("EOGram!");
    if (eogramPos != std::string::npos) {
        if (eogramPos > 0 && text[eogramPos - 1] != '\n') {
            text.insert(eogramPos, "\n");
            eogramPos++;
        }
        
        std::string newRule = name + " : eps.\n";
        text.insert(eogramPos, newRule);
        
        if (text.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), text.c_str());
            RebuildGrammarFromText();
            
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

void AddNonTerminalReferenceToGrammar(const std::string& name) {
    ClearOutput();
    AppendOutput("AddNonTerminalReferenceToGrammar: START\n");
    
    if (!grammar) {
        AppendOutput("ERROR: grammar is null\n");
        return;
    }
    
    auto nts = grammar->getNonTerminals();
    
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
        AppendOutput("ERROR: invalid activeNTIndex\n");
        return;
    }
    
    if (grammar->findNonTerminal(name) < 0) {
        AppendOutput("ERROR: non-terminal not found\n");
        return;
    }
    
    char buf[256];
    snprintf(buf, sizeof(buf), "Adding reference to: %s\n", name.c_str());
    AppendOutput(buf);
    
    std::string text = grammarText;
    std::string ntName = nts[activeNTIndex];
    
    snprintf(buf, sizeof(buf), "Current rule: %s\n", ntName.c_str());
    AppendOutput(buf);
    
    size_t pos = text.find(ntName + " :");
    if (pos == std::string::npos) pos = text.find(ntName + ":");
    if (pos == std::string::npos) {
        AppendOutput("ERROR: cannot find rule\n");
        return;
    }
    
    size_t colonPos = text.find(":", pos);
    size_t dotPos = text.find(".", colonPos);
    if (colonPos == std::string::npos || dotPos == std::string::npos) {
        AppendOutput("ERROR: cannot find colon or dot\n");
        return;
    }
    
    std::string ruleBody = text.substr(colonPos + 1, dotPos - colonPos - 1);
    
    snprintf(buf, sizeof(buf), "Rule body: [%s]\n", ruleBody.c_str());
    AppendOutput(buf);
    
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
    
    snprintf(buf, sizeof(buf), "Trimmed body: [%s]\n", trimmedBody.c_str());
    AppendOutput(buf);
    
    std::string newBody;
    
    if (trimmedBody.empty()) {
        newBody = " " + name;
    } else if (useOrOperator) {
        newBody = " " + trimmedBody + " ; " + name;
    } else {
        size_t lastSemicolon = trimmedBody.rfind(';');
        
        if (lastSemicolon != std::string::npos) {
            std::string beforeLast = trimmedBody.substr(0, lastSemicolon + 1);
            std::string lastAlt = trimmedBody.substr(lastSemicolon + 1);
            
            size_t altStart = lastAlt.find_first_not_of(" \t\n\r");
            if (altStart != std::string::npos) {
                lastAlt = lastAlt.substr(altStart);
            }
            
            newBody = " " + beforeLast + " " + lastAlt + ", " + name;
        } else {
            newBody = " " + trimmedBody + ", " + name;
        }
    }
    
    snprintf(buf, sizeof(buf), "New body: [%s]\n", newBody.c_str());
    AppendOutput(buf);
    
    text.replace(colonPos + 1, dotPos - colonPos - 1, newBody);
    
    if (text.size() >= sizeof(grammarText)) {
        AppendOutput("ERROR: text too large\n");
        return;
    }
    
    strcpy_s(grammarText, sizeof(grammarText), text.c_str());
    
    AppendOutput("Rebuilding grammar\n");
    RebuildGrammarFromText();
    SaveCurrentState();
    
    ClearOutput();
    AppendOutput("Added reference '");
    AppendOutput(name.c_str());
    AppendOutput("' ");
    AppendOutput(useOrOperator ? "(OR)" : "(AND)");
    AppendOutput("\n");
}

void FindAndCreateMissingNonTerminals(const std::string& rule) {
    if (!grammar) return;
    
    auto existingNTs = grammar->getNonTerminals();
    std::vector<std::string> foundNTs;
    
    bool inQuotes = false;
    std::string currentId;
    
    for (size_t i = 0; i < rule.length(); ++i) {
        char c = rule[i];
        
        if (c == '\'' || c == '"') {
            inQuotes = !inQuotes;
            currentId.clear();
        } else if (!inQuotes) {
            if (isalnum(c) || c == '_') {
                currentId += c;
            } else {
                if (!currentId.empty()) {
                    if (currentId != "eps") {
                        foundNTs.push_back(currentId);
                    }
                    currentId.clear();
                }
            }
        }
    }
    
    if (!currentId.empty() && currentId != "eps") {
        foundNTs.push_back(currentId);
    }
    
    for (const auto& nt : foundNTs) {
        if (grammar->findTerminal(nt) >= 0) continue;
        
        if (std::find(existingNTs.begin(), existingNTs.end(), nt) != existingNTs.end()) {
            continue;
        }
        
        std::string text = grammarText;
        
        size_t eogramPos = text.find("EOGram!");
        if (eogramPos != std::string::npos) {
            std::string newNTRule = nt + " : eps.\n";
            text.insert(eogramPos, newNTRule);
            
            if (text.size() < sizeof(grammarText)) {
                strcpy_s(grammarText, sizeof(grammarText), text.c_str());
                
                ClearOutput();
                AppendOutput("Auto-created non-terminal '");
                AppendOutput(nt.c_str());
                AppendOutput("'\n");
            }
        }
    }
}

// Grammar Operations
void ParseGrammar(bool clearLayouts = true) {
    if (clearLayouts) {
        ClearDiagramLayouts();
    }
    try {
        ClearOutput();
        
        std::string tempFile = "temp_grammar.grm";
        SaveTextFile(tempFile.c_str(), grammarText);
        
        grammar = std::make_unique<syngt::Grammar>();
        grammar->load(tempFile);
        
        std::remove(tempFile.c_str());
        
        AppendOutput("Grammar parsed successfully!\n\n");
        
        char stats[512];
        // Subtract 1 to exclude the implicit epsilon terminal at index 0
        int terminalCount = std::max(0, grammar->terminals()->getCount() - 1);
        int macroCount = 0;
        for (const auto& nt : grammar->getNonTerminals()) {
            auto* ntItem = grammar->getNTItem(nt);
            if (ntItem && ntItem->isMacro()) macroCount++;
        }
        snprintf(stats, sizeof(stats),
                 "Terminals: %d\nNon-terminals: %zu\nSemantics: %d\nMacros: %d\n",
                 terminalCount,
                 grammar->getNonTerminals().size(),
                 grammar->semantics()->getCount(),
                 macroCount);
        AppendOutput(stats);

        // Set activeNTIndex to the first NT that actually has a rule,
        // skipping the dummy "S" that fillNew() inserts at index 0.
        {
            auto nts = grammar->getNonTerminals();
            activeNTIndex = 0;
            for (int i = 0; i < static_cast<int>(nts.size()); ++i) {
                auto* ntItem = grammar->getNTItem(nts[i]);
                if (ntItem && ntItem->hasRoot()) {
                    activeNTIndex = i;
                    break;
                }
            }
        }

        SaveCurrentState();

        UpdateDiagram();
        RefreshRecursionResults();

    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Parse Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
        grammar.reset();
        drawObjects.reset();
    }
}

// Parse DIAGRAM_LAYOUT: section from file content after EOGram!
static void ParseDiagramLayoutSection(const std::string& layoutSection) {
    ntDiagramLayouts.clear();
    std::istringstream in(layoutSection);
    std::string token;
    if (!(in >> token) || token != "DIAGRAM_LAYOUT:") return;

    int ntCount;
    if (!(in >> ntCount)) return;
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    for (int i = 0; i < ntCount; ++i) {
        std::string ntName;
        if (!std::getline(in, ntName)) break;
        // trim whitespace
        while (!ntName.empty() && (ntName.front() == ' ' || ntName.front() == '\r')) ntName.erase(ntName.begin());
        while (!ntName.empty() && (ntName.back() == ' ' || ntName.back() == '\r')) ntName.pop_back();
        if (ntName.empty()) { --i; continue; }

        // Read the layout data until we hit END_NT or next NT marker
        // We use a sentinel count approach: read height/width/count first, then objects+arrows
        // Easiest: capture the rest as a sub-stream until END_NT
        std::string layoutData;
        std::string line;
        while (std::getline(in, line)) {
            if (line == "END_NT" || line == "END_NT\r") break;
            layoutData += line + "\n";
        }
        ntDiagramLayouts[ntName] = layoutData;
    }
}

void LoadFile() {
    std::string filename = OpenFileDialog();
    if (filename.empty()) return;

    try {
        ClearOutput();
        std::string content = LoadTextFile(filename.c_str());

        // Split at DIAGRAM_LAYOUT: section (if present)
        std::string grammarPart = content;
        std::string layoutPart;
        const std::string layoutMarker = "\nDIAGRAM_LAYOUT:";
        size_t layoutPos = content.find(layoutMarker);
        if (layoutPos != std::string::npos) {
            grammarPart = content.substr(0, layoutPos + 1); // keep the \n before marker
            layoutPart = content.substr(layoutPos + 1);     // "DIAGRAM_LAYOUT:..."
        }

        if (grammarPart.size() >= sizeof(grammarText)) {
            AppendOutput("File too large!\n");
            return;
        }

        strcpy_s(grammarText, sizeof(grammarText), grammarPart.c_str());
        grammarWidgetVersion++;
        strcpy_s(currentFile, sizeof(currentFile), filename.c_str());

        // Load layout data into map before parsing (so UpdateDiagram can use it)
        ntDiagramLayouts.clear();
        s_currentDiagramNT.clear();
        if (!layoutPart.empty()) {
            ParseDiagramLayoutSection(layoutPart);
        }

        AppendOutput("File loaded: ");
        AppendOutput(filename.c_str());
        AppendOutput("\n");

        ParseGrammar(false); // don't clear layouts we just loaded

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
        // Save current diagram to cache before writing
        SaveCurrentDiagramToCache();

        std::ofstream file(filename);
        if (!file.is_open()) throw std::runtime_error("Cannot create file: " + filename);

        file << grammarText;

        // Append DIAGRAM_LAYOUT section if we have any layouts
        if (!ntDiagramLayouts.empty()) {
            file << "DIAGRAM_LAYOUT:\n";
            file << static_cast<int>(ntDiagramLayouts.size()) << "\n";
            for (const auto& pair : ntDiagramLayouts) {
                file << pair.first << "\n";
                file << pair.second;
                file << "END_NT\n";
            }
            file << "END_DIAGRAM_LAYOUT\n";
        }

        file.close();

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
        ClearDiagramLayouts();
        UpdateDiagram();
        RefreshRecursionResults();
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void EliminateRightRecursion() {
    if (!grammar) {
        ClearOutput();
        AppendOutput("Please parse grammar first!\n");
        return;
    }

    try {
        ClearOutput();
        syngt::RightElimination::eliminate(grammar.get());

        std::string tempFile = "temp_result.grm";
        grammar->save(tempFile);
        std::string newGrammar = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());

        if (newGrammar.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
        }

        AppendOutput("Right recursion eliminated!\n");

        SaveCurrentState();
        ClearDiagramLayouts();
        UpdateDiagram();
        RefreshRecursionResults();
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void EliminateBothRecursions() {
    if (!grammar) {
        ClearOutput();
        AppendOutput("Please parse grammar first!\n");
        return;
    }

    try {
        ClearOutput();
        syngt::Regularize::regularize(grammar.get());

        std::string tempFile = "temp_result.grm";
        grammar->save(tempFile);
        std::string newGrammar = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());

        if (newGrammar.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
        }

        AppendOutput("Grammar regularized (left + right recursion eliminated)!\n");

        SaveCurrentState();
        ClearDiagramLayouts();
        UpdateDiagram();
        RefreshRecursionResults();
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void MinimizeGrammar() {
    if (!grammar) {
        ClearOutput();
        AppendOutput("Please parse grammar first!\n");
        return;
    }

    try {
        ClearOutput();
        syngt::Minimize::minimize(grammar.get());

        std::string tempFile = "temp_result.grm";
        grammar->save(tempFile);
        std::string newGrammar = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());

        if (newGrammar.size() < sizeof(grammarText)) {
            strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
        }

        AppendOutput("Grammar minimized (DFA state minimization)!\n");

        SaveCurrentState();
        ClearDiagramLayouts();
        UpdateDiagram();
        RefreshRecursionResults();
    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

static void RefreshRecursionResults() {
    if (!grammar) return;
    try {
        recursionResults = syngt::RecursionAnalyzer::analyze(grammar.get());
    } catch (...) {}
}

void AnalyzeRecursion() {
    if (!grammar) {
        ClearOutput();
        AppendOutput("Please parse grammar first!\n");
        return;
    }
    RefreshRecursionResults();
    showRecursionWindow = true;
}

// ---------------------------------------------------------------------------
// ExtractRule helpers
// ---------------------------------------------------------------------------

static syngt::SelectionMask GetSelMas() {
    syngt::SelectionMask sm;
    if (!drawObjects) return sm;
    for (int i = 0; i < drawObjects->count(); ++i) {
        if ((*drawObjects)[i]->selected()) {
            sm.push_back(i);
        }
    }
    return sm;
}

static const syngt::RETree* FindSelectionCover(const syngt::RETree* node,
                                                const syngt::SelectionMask& sm) {
    if (!node) return nullptr;
    int doId = node->drawObj();
    for (int id : sm) {
        if (id == doId) return node;
    }
    const syngt::RETree* lc = FindSelectionCover(node->left(), sm);
    const syngt::RETree* rc = FindSelectionCover(node->right(), sm);
    if (!lc && !rc) return nullptr;
    if (!lc) return rc;
    if (!rc) return lc;
    return node;
}

// Replace the node pointed to by `target` inside `node`'s subtree with a new
// RENonTerminal. Works by identity comparison (pointer), so handles duplicate
// sub-expressions correctly. Returns true if the replacement was made.
static bool ReplaceNodeInTree(syngt::RETree* node, const syngt::RETree* target,
                               syngt::Grammar* g, int newNTId) {
    if (!node) return false;
    auto* bin = dynamic_cast<syngt::REBinaryOp*>(node);
    if (!bin) return false;
    if (bin->left() == target) {
        bin->setFirst(std::make_unique<syngt::RENonTerminal>(g, newNTId, false));
        return true;
    }
    if (bin->right() == target) {
        bin->setSecond(std::make_unique<syngt::RENonTerminal>(g, newNTId, false));
        return true;
    }
    return ReplaceNodeInTree(bin->left(),  target, g, newNTId) ||
           ReplaceNodeInTree(bin->right(), target, g, newNTId);
}

static std::string GetFreeNonTerminalName() {
    int index = 1;
    while (true) {
        std::string name = "NT" + std::to_string(index);
        if (grammar->findNonTerminal(name) < 0) return name;
        ++index;
    }
}

// Recursively copy a RE tree, inlining any RENonTerminal whose drawObj() is
// in the selection mask with the body of that NT.
static std::unique_ptr<syngt::RETree> SubstituteInTree(
    const syngt::RETree* node,
    const syngt::SelectionMask& sm)
{
    if (!node) return nullptr;

    // RENonTerminal: inline if selected
    if (auto* nt = dynamic_cast<const syngt::RENonTerminal*>(node)) {
        if (nt->drawObj() >= 0) {
            for (int id : sm) {
                if (id == nt->drawObj()) {
                    syngt::NTListItem* item = grammar->getNTItem(nt->nameID());
                    if (item && item->hasRoot()) return item->root()->copy();
                    break;
                }
            }
        }
        return node->copy();
    }

    // Binary ops: copy the node, then replace children with substituted versions.
    // copy() does a deep copy; we overwrite children immediately after.
    if (dynamic_cast<const syngt::REBinaryOp*>(node)) {
        auto nodeCopy = node->copy();
        auto* bin = dynamic_cast<syngt::REBinaryOp*>(nodeCopy.get());
        if (bin) {
            bin->setFirst(SubstituteInTree(node->left(), sm));
            bin->setSecond(SubstituteInTree(node->right(), sm));
        }
        return nodeCopy;
    }

    // Leaves (Terminal, Semantic): copy as-is
    return node->copy();
}

void ExtractRule() {
    if (!grammar || !drawObjects) {
        ClearOutput();
        AppendOutput("No grammar loaded!\n");
        return;
    }

    auto nts = grammar->getNonTerminals();
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) return;

    std::string activeNT = nts[activeNTIndex];
    syngt::NTListItem* nt = grammar->getNTItem(activeNT);
    if (!nt || !nt->root()) {
        ClearOutput();
        AppendOutput("No rule for current nonterminal!\n");
        return;
    }

    auto sm = GetSelMas();
    if (sm.empty()) {
        ClearOutput();
        AppendOutput("No objects selected!\n");
        return;
    }

    const syngt::RETree* cover = FindSelectionCover(nt->root(), sm);
    if (!cover) {
        ClearOutput();
        AppendOutput("Selection does not match any subtree!\n");
        return;
    }
    if (cover == nt->root()) {
        ClearOutput();
        AppendOutput("Minimal regular expression matches whole rule!\n");
        return;
    }

    auto emptyMask = syngt::EmptyMask();
    s_extractCoverStr  = cover->toString(emptyMask, false);
    s_extractActiveNT  = activeNT;
    s_extractCoverNode = cover;

    std::string freeName = GetFreeNonTerminalName();
    strcpy_s(extractNewNTName, sizeof(extractNewNTName), freeName.c_str());

    showExtractRuleDialog = true;
}

void Substitute() {
    if (!grammar || !drawObjects) {
        ClearOutput();
        AppendOutput("No grammar loaded!\n");
        return;
    }

    auto nts = grammar->getNonTerminals();
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) return;

    std::string activeNT = nts[activeNTIndex];
    syngt::NTListItem* nt = grammar->getNTItem(activeNT);
    if (!nt || !nt->root()) {
        ClearOutput();
        AppendOutput("No rule for current nonterminal!\n");
        return;
    }

    auto sm = GetSelMas();
    if (sm.empty()) {
        ClearOutput();
        AppendOutput("No objects selected!\n");
        return;
    }

    // Build a new RE tree with selected NT boxes replaced by their bodies
    auto newRoot = SubstituteInTree(nt->root(), sm);
    if (!newRoot) {
        ClearOutput();
        AppendOutput("Substitution produced empty rule!\n");
        return;
    }
    nt->setRoot(std::move(newRoot)); // also updates m_value via setValueFromRoot

    // Update grammarText
    std::string tempFile = "temp_result.grm";
    grammar->save(tempFile);
    std::string newGrammar = LoadTextFile(tempFile.c_str());
    std::remove(tempFile.c_str());
    if (newGrammar.size() < sizeof(grammarText)) {
        strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
    }

    ClearDiagramLayouts();
    UpdateDiagram();
    LoadRuleToEditor();
    SaveCurrentState();

    ClearOutput();
    AppendOutput("Substitution applied!\n");
}

void ToggleMacro() {
    if (!grammar) { ClearOutput(); AppendOutput("No grammar loaded!\n"); return; }
    auto nts = grammar->getNonTerminals();
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) return;
    std::string ntName = nts[activeNTIndex];
    syngt::NTListItem* item = grammar->getNTItem(ntName);
    if (!item) return;

    item->setMacro(!item->isMacro());

    // Persist via save/reload so grammarText contains AUXILIARYNOTIONS section
    std::string tempFile = "temp_result.grm";
    grammar->save(tempFile);
    std::string newGrammar = LoadTextFile(tempFile.c_str());
    std::remove(tempFile.c_str());
    if (newGrammar.size() < sizeof(grammarText))
        strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());

    SaveCurrentState();
    ClearOutput();
    AppendOutput(item->isMacro() ? "Marked as macro.\n" : "Unmarked as macro.\n");
}

void OpenAllMacros() {
    if (!grammar) { ClearOutput(); AppendOutput("No grammar loaded!\n"); return; }
    auto nts = grammar->getNonTerminals();
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) return;
    grammar->openMacroRefs(nts[activeNTIndex]);
    ClearDiagramLayouts();
    UpdateDiagram();
    ClearOutput();
    AppendOutput("Macro references expanded.\n");
}

void CloseAllDefinitions() {
    if (!grammar) { ClearOutput(); AppendOutput("No grammar loaded!\n"); return; }
    auto nts = grammar->getNonTerminals();
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) return;
    grammar->closeAllRefs(nts[activeNTIndex]);
    ClearDiagramLayouts();
    UpdateDiagram();
    ClearOutput();
    AppendOutput("Definitions collapsed.\n");
}

void NewFile() {
    grammar = std::make_unique<syngt::Grammar>();
    grammar->fillNew();
    grammarText[0] = '\0';
    grammarWidgetVersion++;
    currentFile[0] = '\0';
    activeNTIndex = -1;
    selectionMask.clear();
    drawObjects.reset();
    undoRedo.clearData();
    ClearDiagramLayouts();
    ClearOutput();
    AppendOutput("New grammar created.\n");
    SaveCurrentState();
}

void ImportFromGEdit() {
    std::string filename = OpenGEditFileDialog();
    if (filename.empty()) return;

    try {
        grammar = std::make_unique<syngt::Grammar>();
        grammar->importFromGEdit(filename);

        // Sync grammarText from the imported grammar
        std::string tempFile = "temp_import.grm";
        grammar->save(tempFile);
        std::string imported = LoadTextFile(tempFile.c_str());
        std::remove(tempFile.c_str());

        if (imported.size() >= sizeof(grammarText)) {
            AppendOutput("Imported grammar too large!\n");
            return;
        }
        strcpy_s(grammarText, sizeof(grammarText), imported.c_str());
        currentFile[0] = '\0';

        ClearOutput();
        AppendOutput("Imported from GEdit: ");
        AppendOutput(filename.c_str());
        AppendOutput("\n");

        undoRedo.clearData();
        ParseGrammar();

    } catch (const std::exception& e) {
        ClearOutput();
        AppendOutput("Import Error:\n");
        AppendOutput(e.what());
        AppendOutput("\n");
    }
}

void DiagramAddExtendedPoint() {
    if (!drawObjects) {
        ClearOutput();
        AppendOutput("No diagram loaded!\n");
        return;
    }
    // Requires exactly 1 selected object with an incoming arrow
    int selectedCount = 0;
    for (int i = 0; i < drawObjects->count(); ++i) {
        if ((*drawObjects)[i]->selected()) selectedCount++;
    }
    if (selectedCount != 1) {
        ClearOutput();
        AppendOutput("Select exactly 1 diagram object to add an extended point before it.\n");
        return;
    }
    drawObjects->addExtendedPoint();
    // Note: do NOT call UpdateDiagram() — it would rebuild from grammar and lose the point
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
        ClearDiagramLayouts();
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
        ClearDiagramLayouts();
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
#ifdef _WIN32
    // ===== WINDOWS VERSION (DirectX 11) =====
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

    ImGui::StyleColorsLight();
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;

    // Color scheme
    style.Colors[ImGuiCol_Text] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.93f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.80f, 0.87f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.50f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.55f, 0.95f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.55f, 0.95f, 0.20f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 0.55f, 0.95f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.25f, 0.55f, 0.95f, 0.95f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.55f, 0.95f, 0.80f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.30f, 0.30f, 0.35f, 0.09f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 0.55f, 0.95f, 0.35f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.25f, 0.55f, 0.95f, 0.95f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\consola.ttf", 16.0f);
    
#else
    // ===== LINUX VERSION (OpenGL 3 + GLFW) =====
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    g_Window = glfwCreateWindow(1280, 800, "SynGT - Syntax Grammar Transformation", nullptr, nullptr);
    if (g_Window == nullptr)
        return 1;
    glfwMakeContextCurrent(g_Window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = "syngt.ini";

    ImGui::StyleColorsLight();
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;

    // Color scheme
    style.Colors[ImGuiCol_Text] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.93f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.80f, 0.87f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.50f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.55f, 0.95f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.55f, 0.95f, 0.20f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 0.55f, 0.95f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.25f, 0.55f, 0.95f, 0.95f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.55f, 0.95f, 0.80f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.30f, 0.30f, 0.35f, 0.09f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 0.55f, 0.95f, 0.35f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.25f, 0.55f, 0.95f, 0.95f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.25f, 0.55f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    ImGui_ImplGlfw_InitForOpenGL(g_Window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    io.Fonts->AddFontDefault();
#endif

    // Common initialization
    const char* exampleGrammar = 
        "{ Example: Simple expression grammar }\n"
        "E : T, '+', E ; T.\n"
        "T : '(', E, ')' ; 'id'.\n"
        "EOGram!\n";
    strcpy(grammarText, exampleGrammar);

    // Main loop
    bool done = false;
    while (!done)
    {
#ifdef _WIN32
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

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
#else
        glfwPollEvents();
        if (glfwWindowShouldClose(g_Window))
            break;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
#endif

        // Menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New", "Ctrl+N")) NewFile();
                if (ImGui::MenuItem("Open...", "Ctrl+O")) LoadFile();
                if (ImGui::MenuItem("Save", "Ctrl+S")) SaveFile();
                if (ImGui::MenuItem("Save As...")) SaveFileAs();
                ImGui::Separator();
                if (ImGui::MenuItem("Import from GEdit...")) ImportFromGEdit();
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
                if (ImGui::MenuItem("Eliminate Right Recursion")) EliminateRightRecursion();
                if (ImGui::MenuItem("Regularize (Both)")) EliminateBothRecursions();
                if (ImGui::MenuItem("Minimize (DFA)")) MinimizeGrammar();
                if (ImGui::MenuItem("Analyze Recursion")) AnalyzeRecursion();
                ImGui::Separator();
                if (ImGui::MenuItem("Extract Rule")) ExtractRule();
                if (ImGui::MenuItem("Substitute")) Substitute();
                if (ImGui::MenuItem("Left Factorization")) LeftFactorize();
                if (ImGui::MenuItem("Remove Useless")) RemoveUselessSymbols();
                ImGui::Separator();
                if (ImGui::MenuItem("Toggle Macro")) ToggleMacro();
                if (ImGui::MenuItem("Open All Macros")) OpenAllMacros();
                if (ImGui::MenuItem("Close All Definitions")) CloseAllDefinitions();
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

        // Shortcuts that must not fire while typing in a text field
        if (!io.WantTextInput) {
            // Ctrl+Z - Undo
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
                Undo();
            }
            // Ctrl+Y - Redo
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
                Redo();
            }
            // Ctrl+S - Save
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
                SaveFile();
            }
            // Ctrl+O - Open
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
                LoadFile();
            }
            // Ctrl+N - New
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N, false)) {
                NewFile();
            }
            // F5 - Parse
            if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) {
                ParseGrammar();
            }
            // Ctrl+A - Select all diagram objects
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A, false) && drawObjects) {
                drawObjects->unselectAll();
                for (int i = 0; i < drawObjects->count(); ++i) {
                    drawObjects->changeSelection((*drawObjects)[i]);
                }
            }
            // Delete - remove selected diagram objects
            if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && drawObjects) {
                int selectedCount = 0;
                for (int i = 0; i < drawObjects->count(); ++i) {
                    if ((*drawObjects)[i]->selected()) selectedCount++;
                }
                if (selectedCount > 0) DeleteSelectedObjects();
            }
            // Escape - deselect
            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && drawObjects) {
                drawObjects->unselectAll();
            }
        }

        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - ImGui::GetFrameHeight()));
        ImGui::Begin("SynGT Main", nullptr, 
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

        ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, 0), true);
        
        float leftPanelHeight = ImGui::GetContentRegionAvail().y;
        static float ruleEditorHeight = 100.0f;
        float upperPartHeight = (activeLeftTab == 1) 
            ? (leftPanelHeight - ruleEditorHeight - 8)
            : leftPanelHeight;
        
        ImGui::BeginChild("UpperLeftPanel", ImVec2(0, upperPartHeight), false);
        
        if (ImGui::BeginTabBar("LeftTabs")) {
            if (ImGui::BeginTabItem("Grammar Editor")) {
                activeLeftTab = 0;

                ImGui::Text("Editor");
                ImGui::SameLine();
                if (ImGui::Button("Parse (F5)")) ParseGrammar();
                ImGui::SameLine();
                if (ImGui::Button("Clear")) { grammarText[0] = '\0'; ClearOutput(); }
                
                ImGui::Separator();
                
                ImGui::PushID(grammarWidgetVersion);
                ImGui::InputTextMultiline("##grammar", grammarText, sizeof(grammarText),
                                           ImVec2(-FLT_MIN, -FLT_MIN),
                                           ImGuiInputTextFlags_AllowTabInput);
                ImGui::PopID();
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Syntax Diagram")) {
                activeLeftTab = 1;

                if (grammar && !grammar->getNonTerminals().empty()) {
                    auto nts = grammar->getNonTerminals();
                    
                    ImGui::Text("Select non-terminal:");
                    ImGui::SameLine();
                    
                    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) {
                        activeNTIndex = 0;
                    }
                    
                    ImGui::SetNextItemWidth(-70.0f);
                    if (ImGui::BeginCombo("##nt_select", nts[activeNTIndex].c_str())) {
                        for (int i = 0; i < static_cast<int>(nts.size()); ++i) {
                            bool is_selected = (activeNTIndex == i);

                            std::string label = nts[i];
                            syngt::NTListItem* item = grammar->getNTItem(nts[i]);

                            if (item && item->isMacro()) label += " [M]";
                            if (!item || !item->hasRoot()) {
                                label += " [eps]";
                            }

                            if (ImGui::Selectable(label.c_str(), is_selected)) {
                                activeNTIndex = i;
                                UpdateDiagram();
                                LoadRuleToEditor();
                            }
                            if (is_selected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Delete##nt", ImVec2(-FLT_MIN, 0))) {
                        DeleteNonTerminal(activeNTIndex);
                        UpdateDiagram();
                        LoadRuleToEditor();
                    }

                    ImGui::Separator();
                    
                    ImGui::BeginChild("DiagramCanvas", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
                    
                    if (drawObjects && drawObjects->count() > 0) {
                        // float diagramWidth = static_cast<float>(drawObjects->width());
                        // float diagramHeight = static_cast<float>(drawObjects->height());
                        
                        // int minX = 0, minY = 0, maxX = static_cast<int>(diagramWidth);
                        // int maxY = static_cast<int>(diagramHeight);
                        int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;

                        for (int i = 0; i < drawObjects->count(); ++i) {
                            auto* obj = (*drawObjects)[i];
                            if (obj) {
                                int objX = obj->x();
                                int objY = obj->y();
                                int objEndX = obj->endX();

                                int objType = obj->getType();
                                int topY  = objY;
                                int botY  = objY;
                                if (objType == syngt::graphics::ctDrawObjectTerminal ||
                                    objType == syngt::graphics::ctDrawObjectNonTerminal) {
                                    topY = objY - 20;
                                    botY = objY + 20;
                                } else if (objType == syngt::graphics::ctDrawObjectFirst ||
                                           objType == syngt::graphics::ctDrawObjectLast) {
                                    topY = objY - 10;
                                    botY = objY + 10;
                                }

                                if (objX  < minX) minX = objX;
                                if (topY  < minY) minY = topY;
                                if (objEndX > maxX) maxX = objEndX;
                                if (botY  > maxY) maxY = botY;
                            }
                        }

                        if (minX == INT_MAX) minX = 0;
                        if (minY == INT_MAX) minY = 0;
                        if (maxX == INT_MIN) maxX = minX + 100;
                        if (maxY == INT_MIN) maxY = minY + 100;

                        // Update stable bounds only when not dragging to prevent
                        // offset jumps while objects are being moved
                        if (!s_stableBoundsValid || !isDragging) {
                            s_stableMinX = minX;
                            s_stableMinY = minY;
                            s_stableMaxX = maxX;
                            s_stableMaxY = maxY;
                            s_stableBoundsValid = true;
                        }

                        // Canvas size tracks current bounds (can grow when dragging out)
                        // but offset is anchored to stable bounds (no jumps during drag)
                        int canvasMinX = std::min(minX, s_stableMinX);
                        int canvasMinY = std::min(minY, s_stableMinY);
                        int canvasMaxX = std::max(maxX, s_stableMaxX);
                        int canvasMaxY = std::max(maxY, s_stableMaxY);

                        const float padding = 100.0f;
                        float canvasWidth  = (canvasMaxX - canvasMinX) * diagramScale + padding * 2;
                        float canvasHeight = (canvasMaxY - canvasMinY) * diagramScale + padding * 2;

                        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                        ImVec2 offset = ImVec2(canvasPos.x + padding - s_stableMinX * diagramScale,
                                               canvasPos.y + padding - s_stableMinY * diagramScale);

                        ImGui::Dummy(ImVec2(canvasWidth, canvasHeight));

                        ImGui::SetCursorScreenPos(canvasPos);
                        ImGui::InvisibleButton("##canvas", ImVec2(canvasWidth, canvasHeight));
                        bool isHovered = ImGui::IsItemHovered();
                        
                        ImVec2 mousePos = ImGui::GetMousePos();
                        
                        // Ctrl+scroll to zoom (check whole canvas child window, not just InvisibleButton)
                        bool canvasWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows |
                                                                           ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                        if (canvasWindowHovered && io.KeyCtrl && io.MouseWheel != 0.0f) {
                            float delta = io.MouseWheel * 0.1f;
                            diagramScale = std::max(0.25f, std::min(4.0f, diagramScale + delta));
                        }

                        // Hovering
                        if (isHovered && !isDragging && !isBoxSelecting) {
                            hoveredObject = FindDrawObjectAt(mousePos, offset, diagramScale);
                            if (hoveredObject) {
                                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                            }
                        } else if (!isHovered) {
                            hoveredObject = nullptr;
                        }

                        // Left click
                        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            syngt::graphics::DrawObject* clicked = FindDrawObjectAt(mousePos, offset, diagramScale);

                            if (clicked) {
                                isDragging = true;
                                lastMousePos = mousePos;

                                if (ImGui::GetIO().KeyShift) {
                                    // Shift+Click: select clicked object + all connected (upstream)
                                    drawObjects->selectAllNotSelected(clicked);
                                } else if (ImGui::GetIO().KeyCtrl) {
                                    // Ctrl+Click: toggle selection
                                    drawObjects->changeSelection(clicked);
                                } else {
                                    if (!clicked->selected()) {
                                        // Click unselected: deselect all, select clicked
                                        drawObjects->unselectAll();
                                        drawObjects->changeSelection(clicked);
                                    }
                                    // Click already-selected: keep selection for group drag
                                }
                            } else {
                                isBoxSelecting = true;
                                boxSelectStart = mousePos;

                                if (!ImGui::GetIO().KeyCtrl) {
                                    drawObjects->unselectAll();
                                }
                            }
                        }

                        // Dragging
                        if (isDragging) {
                            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                                ImVec2 currentMousePos = ImGui::GetMousePos();
                                ImVec2 delta = ImVec2(
                                    currentMousePos.x - lastMousePos.x,
                                    currentMousePos.y - lastMousePos.y
                                );

                                if (drawObjects && (delta.x != 0 || delta.y != 0)) {
                                    drawObjects->selectedMove(
                                        static_cast<int>(delta.x / diagramScale),
                                        static_cast<int>(delta.y / diagramScale)
                                    );
                                }

                                lastMousePos = currentMousePos;
                            } else {
                                isDragging = false;
                            }
                        }

                        // Box selecting
                        if (isBoxSelecting) {
                            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                                ImVec2 boxSelectEnd = ImGui::GetMousePos();

                                int left   = static_cast<int>((std::min(boxSelectStart.x, boxSelectEnd.x) - offset.x) / diagramScale);
                                int top    = static_cast<int>((std::min(boxSelectStart.y, boxSelectEnd.y) - offset.y) / diagramScale);
                                int right  = static_cast<int>((std::max(boxSelectStart.x, boxSelectEnd.x) - offset.x) / diagramScale);
                                int bottom = static_cast<int>((std::max(boxSelectStart.y, boxSelectEnd.y) - offset.y) / diagramScale);
                                
                                if (drawObjects) {
                                    for (int i = 0; i < drawObjects->count(); ++i) {
                                        auto* obj = (*drawObjects)[i];
                                        if (!obj) continue;

                                        int t = obj->getType();
                                        if (t != syngt::graphics::ctDrawObjectTerminal &&
                                            t != syngt::graphics::ctDrawObjectNonTerminal) {
                                            continue;
                                        }

                                        // AABB intersection: select if box touches any part of the object
                                        int objLeft   = obj->x();
                                        int objRight  = obj->endX();
                                        int objTop    = obj->y() - 20;
                                        int objBottom = obj->y() + 20;

                                        if (objLeft <= right && objRight >= left &&
                                            objTop  <= bottom && objBottom >= top) {
                                            drawObjects->changeSelection(obj);
                                        }
                                    }
                                }
                                
                                isBoxSelecting = false;
                            }
                        }
                        
                        // Right click
                        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                            contextMenuObject = FindDrawObjectAt(mousePos, offset, diagramScale);
                            ImGui::OpenPopup("DiagramContextMenu");
                        }
                        
                        // Context menu
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
                                if (ImGui::MenuItem("Extract Rule")) {
                                    ExtractRule();
                                }
                                if (ImGui::MenuItem("Substitute")) {
                                    Substitute();
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

                            ImGui::Separator();
                            {
                                int selCnt = 0;
                                if (drawObjects) {
                                    for (int i = 0; i < drawObjects->count(); ++i)
                                        if ((*drawObjects)[i]->selected()) selCnt++;
                                }
                                if (ImGui::MenuItem("Add Extended Point", nullptr, false, selCnt == 1)) {
                                    DiagramAddExtendedPoint();
                                }
                            }

                            ImGui::EndPopup();
                        }

                        // Diagram render
                        ImDrawList* drawList = ImGui::GetWindowDrawList();
                        // Use window screen position (not canvasPos) for clip rect —
                        // canvasPos shifts left when scrolled, causing partial background fill
                        ImVec2 windowScreenPos = ImGui::GetWindowPos();
                        ImVec2 childSize = ImGui::GetWindowSize();
                        ImVec2 clipMin = windowScreenPos;
                        ImVec2 clipMax = ImVec2(windowScreenPos.x + childSize.x, windowScreenPos.y + childSize.y);

                        drawList->PushClipRect(clipMin, clipMax, true);
                        drawList->AddRectFilled(clipMin, clipMax, IM_COL32(248, 248, 248, 255));
                        
                        RenderDiagram(drawList, offset, diagramScale);
                        
                        // Render box selection
                        if (isBoxSelecting && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                            ImVec2 currentMouse = ImGui::GetMousePos();
                            ImVec2 rectMin = ImVec2(
                                std::min(boxSelectStart.x, currentMouse.x),
                                std::min(boxSelectStart.y, currentMouse.y)
                            );
                            ImVec2 rectMax = ImVec2(
                                std::max(boxSelectStart.x, currentMouse.x),
                                std::max(boxSelectStart.y, currentMouse.y)
                            );
                            
                            drawList->AddRectFilled(rectMin, rectMax, IM_COL32(70, 130, 255, 80));
                            drawList->AddRect(rectMin, rectMax, IM_COL32(70, 130, 255, 200), 0.0f, 0, 2.0f);
                        }
                        
                        drawList->PopClipRect();

                        // Zoom indicator (bottom-right corner)
                        {
                            ImVec2 windowPos  = ImGui::GetWindowPos();
                            ImVec2 windowSize = ImGui::GetWindowSize();
                            char zoomBuf[32];
                            snprintf(zoomBuf, sizeof(zoomBuf), "%.0f%%", diagramScale * 100.0f);
                            ImVec2 zoomTextSize = ImGui::CalcTextSize(zoomBuf);
                            ImVec2 zoomPos = ImVec2(
                                windowPos.x + windowSize.x - zoomTextSize.x - 16,
                                windowPos.y + windowSize.y - zoomTextSize.y - 10
                            );
                            ImDrawList* overlayList = ImGui::GetWindowDrawList();
                            overlayList->AddRectFilled(
                                ImVec2(zoomPos.x - 6, zoomPos.y - 4),
                                ImVec2(zoomPos.x + zoomTextSize.x + 6, zoomPos.y + zoomTextSize.y + 4),
                                IM_COL32(240, 240, 245, 200), 3.0f
                            );
                            overlayList->AddText(zoomPos, IM_COL32(60, 60, 60, 255), zoomBuf);
                        }

                        // Selection info
                        int selectedCount = 0;
                        for (int i = 0; i < drawObjects->count(); ++i) {
                            if ((*drawObjects)[i]->selected()) {
                                selectedCount++;
                            }
                        }
                        
                        if (selectedCount > 0) {
                            ImVec2 windowPos = ImGui::GetWindowPos();
                            ImVec2 windowSize = ImGui::GetWindowSize();
                            ImVec2 infoPos = ImVec2(windowPos.x + 10, windowPos.y + windowSize.y - 35);
                            
                            char labelBuf[64];
                            snprintf(labelBuf, sizeof(labelBuf), "Selected: %d", selectedCount);
                            ImVec2 textSize = ImGui::CalcTextSize(labelBuf);
                            ImVec2 buttonSize = ImGui::CalcTextSize("Deselect");
                            
                            float frameHeight = textSize.y + ImGui::GetStyle().FramePadding.y * 2;
                            float totalWidth = textSize.x + buttonSize.x + ImGui::GetStyle().ItemSpacing.x + 20;
                            
                            ImDrawList* infoDrawList = ImGui::GetWindowDrawList();
                            infoDrawList->AddRectFilled(
                                ImVec2(infoPos.x - 8, infoPos.y - 6),
                                ImVec2(infoPos.x + totalWidth, infoPos.y + frameHeight + 4),
                                IM_COL32(240, 240, 245, 220),
                                4.0f
                            );
                            
                            ImGui::SetCursorScreenPos(ImVec2(infoPos.x, infoPos.y));
                            ImGui::Text("%s", labelBuf);
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
        
        if (activeLeftTab == 1) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Button("##left_hsplitter", ImVec2(-1, 8.0f));
            bool leftHsplitterActive = ImGui::IsItemActive();
            bool leftHsplitterHovered = ImGui::IsItemHovered();
            ImGui::PopStyleColor(3);
            
            if (leftHsplitterActive) {
                ruleEditorHeight -= ImGui::GetIO().MouseDelta.y;
                if (ruleEditorHeight < 60.0f) ruleEditorHeight = 60.0f;
                if (ruleEditorHeight > leftPanelHeight - 200.0f) ruleEditorHeight = leftPanelHeight - 200.0f;
            }
            if (leftHsplitterHovered) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            }
            
            ImGui::BeginChild("RuleEditor", ImVec2(0, 0), true);
            ImGui::Text("Rule definition:");
            ImGui::PushItemWidth(-FLT_MIN);
            ImGui::InputText("##rule_edit", ruleText, sizeof(ruleText), ImGuiInputTextFlags_EnterReturnsTrue);
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                BuildRule();
            }
            ImGui::PopItemWidth();
            
            if (ImGui::Button("Build (Apply Changes)", ImVec2(-FLT_MIN, 0))) {
                BuildRule();
            }
            ImGui::EndChild();
        }

        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::Button("##vsplitter", ImVec2(8.0f, -1));
        bool vsplitterActive = ImGui::IsItemActive();
        bool vsplitterHovered = ImGui::IsItemHovered();
        ImGui::PopStyleColor(3);
        
        if (vsplitterActive) {
            leftWidth += ImGui::GetIO().MouseDelta.x;
            if (leftWidth < 200.0f) leftWidth = 200.0f;
            if (leftWidth > io.DisplaySize.x - 200.0f) leftWidth = io.DisplaySize.x - 200.0f;
        }
        if (vsplitterHovered) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
        ImGui::SameLine();

        // Right panel
        ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
        
        // Operatiion panel
        ImGui::BeginChild("Operations", ImVec2(0, rightPanelButtonsHeight), true);
        ImGui::Text("Operations");
        ImGui::Separator();
        
        if (ImGui::Button("Eliminate Left Recursion", ImVec2(-FLT_MIN, 0))) {
            EliminateLeftRecursion();
        }
        if (ImGui::Button("Eliminate Right Recursion", ImVec2(-FLT_MIN, 0))) {
            EliminateRightRecursion();
        }
        if (ImGui::Button("Regularize (Both)", ImVec2(-FLT_MIN, 0))) {
            EliminateBothRecursions();
        }
        if (ImGui::Button("Minimize (DFA)", ImVec2(-FLT_MIN, 0))) {
            MinimizeGrammar();
        }
        if (ImGui::Button("Analyze Recursion", ImVec2(-FLT_MIN, 0))) {
            AnalyzeRecursion();
        }
        if (ImGui::Button("Extract Rule", ImVec2(-FLT_MIN, 0))) {
            ExtractRule();
        }
        if (ImGui::Button("Substitute", ImVec2(-FLT_MIN, 0))) {
            Substitute();
        }
        if (ImGui::Button("Left Factorization", ImVec2(-FLT_MIN, 0))) {
            LeftFactorize();
        }
        if (ImGui::Button("Remove Useless", ImVec2(-FLT_MIN, 0))) {
            RemoveUselessSymbols();
        }
        if (ImGui::Button("Check LL(1)", ImVec2(-FLT_MIN, 0))) {
            CheckLL1();
        }
        ImGui::Separator();
        ImGui::Text("Macros");
        if (ImGui::Button("Toggle Macro", ImVec2(-FLT_MIN, 0))) {
            ToggleMacro();
        }
        if (ImGui::Button("Open All Macros", ImVec2(-FLT_MIN, 0))) {
            OpenAllMacros();
        }
        if (ImGui::Button("Close All Definitions", ImVec2(-FLT_MIN, 0))) {
            CloseAllDefinitions();
        }
        ImGui::Separator();
        ImGui::Text("Semantics");
        if (grammar) {
            auto semItems = grammar->getSemantics();
            if (semItems.empty()) {
                ImGui::TextDisabled("(none)");
            } else {
                for (int si = 0; si < static_cast<int>(semItems.size()); ++si) {
                    ImGui::PushID(si);
                    ImGui::TextUnformatted(semItems[si].c_str());
                    ImGui::SameLine();
                    if (ImGui::SmallButton("x")) {
                        DeleteSemantic(semItems[si]);
                        UpdateDiagram();
                    }
                    ImGui::PopID();
                }
            }
        } else {
            ImGui::TextDisabled("(no grammar)");
        }
        ImGui::EndChild();
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::Button("##hsplitter", ImVec2(-1, 8.0f));
        bool hsplitterActive = ImGui::IsItemActive();
        bool hsplitterHovered = ImGui::IsItemHovered();
        ImGui::PopStyleColor(3);
        
        if (hsplitterActive) {
            rightPanelButtonsHeight += ImGui::GetIO().MouseDelta.y;
            if (rightPanelButtonsHeight < 100.0f) rightPanelButtonsHeight = 100.0f;
            if (rightPanelButtonsHeight > io.DisplaySize.y - 300.0f) rightPanelButtonsHeight = io.DisplaySize.y - 300.0f;
        }
        if (hsplitterHovered) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }
        
        // Output
        ImGui::BeginChild("Output", ImVec2(0, 0), true);
        ImGui::Text("Output:");
        ImGui::Separator();
        ImGui::InputTextMultiline("##output", outputText, sizeof(outputText),
                                   ImVec2(-FLT_MIN, -FLT_MIN),
                                   ImGuiInputTextFlags_ReadOnly);
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

        // Add Reference Dialog
        if (showAddReferenceDialog) {
            ImGui::OpenPopup("Add Non-Terminal Reference");
            showAddReferenceDialog = false;
        }
        
        if (ImGui::BeginPopupModal("Add Non-Terminal Reference", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Select non-terminal to reference:");
            ImGui::Separator();
            
            if (grammar) {
                auto nts = grammar->getNonTerminals();
                
                for (const auto& nt : nts) {
                    if (ImGui::Selectable(nt.c_str())) {
                        AddNonTerminalReferenceToGrammar(nt);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            
            ImGui::Separator();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }

        // Recursion Analysis window
        if (showRecursionWindow) {
            ImGui::OpenPopup("Recursion Analysis");
            if (ImGui::BeginPopupModal("Recursion Analysis", &showRecursionWindow,
                                       ImGuiWindowFlags_AlwaysAutoResize)) {
                bool needRefresh = false;
                if (ImGui::BeginTable("RecTable", 5,
                                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Nonterminal");
                    ImGui::TableSetupColumn("Left Recursion");
                    ImGui::TableSetupColumn("Recursion");
                    ImGui::TableSetupColumn("Right Recursion");
                    ImGui::TableSetupColumn("Actions");
                    ImGui::TableHeadersRow();
                    for (const auto& r : recursionResults) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(r.name.c_str());
                        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(r.leftRecursion.c_str());
                        ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(r.anyRecursion.c_str());
                        ImGui::TableSetColumnIndex(3); ImGui::TextUnformatted(r.rightRecursion.c_str());
                        ImGui::TableSetColumnIndex(4);
                        if (r.leftRecursion == "direct") {
                            if (ImGui::SmallButton(("Elim L##" + r.name).c_str())) {
                                syngt::NTListItem* nt = grammar->getNTItem(r.name);
                                if (nt) {
                                    syngt::LeftElimination::eliminateForNonTerminal(nt, grammar.get());
                                    UpdateGrammarText();
                                    SaveCurrentState();
                                    needRefresh = true;
                                }
                            }
                            ImGui::SameLine();
                        }
                        if (r.rightRecursion == "direct") {
                            if (ImGui::SmallButton(("Elim R##" + r.name).c_str())) {
                                syngt::NTListItem* nt = grammar->getNTItem(r.name);
                                if (nt) {
                                    syngt::RightElimination::eliminateForNonTerminal(nt, grammar.get());
                                    UpdateGrammarText();
                                    SaveCurrentState();
                                    needRefresh = true;
                                }
                            }
                        }
                    }
                    ImGui::EndTable();
                }
                if (needRefresh) {
                    RefreshRecursionResults();
                }
                ImGui::Separator();
                if (ImGui::Button("Refresh")) RefreshRecursionResults();
                ImGui::SameLine();
                if (ImGui::Button("Close")) showRecursionWindow = false;
                ImGui::EndPopup();
            }
        }

        // Extract Rule dialog
        if (showExtractRuleDialog) {
            ImGui::OpenPopup("Extract Rule");
            showExtractRuleDialog = false;
        }

        if (ImGui::BeginPopupModal("Extract Rule", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Extracted expression:");
            ImGui::TextWrapped("%s", s_extractCoverStr.c_str());
            ImGui::Separator();
            ImGui::Text("Enter new nonterminal name:");
            ImGui::InputText("##extract_name", extractNewNTName, sizeof(extractNewNTName));

            if (ImGui::Button("OK") && extractNewNTName[0] != '\0') {
                std::string newName = extractNewNTName;
                if (grammar->findNonTerminal(newName) >= 0) {
                    // keep popup open, show error in output
                    ClearOutput();
                    AppendOutput("Nonterminal '");
                    AppendOutput(newName.c_str());
                    AppendOutput("' already exists!\n");
                } else {
                    // 1. Create new NT and assign extracted rule
                    grammar->addNonTerminal(newName);
                    grammar->setNTRule(newName, s_extractCoverStr + ".");

                    // 2. Replace the selected subtree in the active NT's RE tree
                    //    with a reference to the new NT (by pointer identity)
                    int newNTId = grammar->findNonTerminal(newName);
                    syngt::NTListItem* activeItem = grammar->getNTItem(s_extractActiveNT);
                    bool replaceOk = false;
                    if (activeItem && activeItem->root() && s_extractCoverNode) {
                        replaceOk = ReplaceNodeInTree(activeItem->root(), s_extractCoverNode,
                                                      grammar.get(), newNTId);
                        if (replaceOk) {
                            activeItem->updateValueFromRoot();
                        }
                    }
                    if (!replaceOk) {
                        ClearOutput();
                        AppendOutput("Error: could not replace subtree!\n");
                        s_extractCoverNode = nullptr;
                        ImGui::CloseCurrentPopup();
                    } else {

                    // Update grammarText
                    std::string tempFile = "temp_result.grm";
                    grammar->save(tempFile);
                    std::string newGrammar = LoadTextFile(tempFile.c_str());
                    std::remove(tempFile.c_str());
                    if (newGrammar.size() < sizeof(grammarText)) {
                        strcpy_s(grammarText, sizeof(grammarText), newGrammar.c_str());
                    }

                    // Switch to new NT
                    auto newNts = grammar->getNonTerminals();
                    for (int i = 0; i < static_cast<int>(newNts.size()); ++i) {
                        if (newNts[i] == newName) {
                            activeNTIndex = i;
                            break;
                        }
                    }

                    s_extractCoverNode = nullptr;

                    ClearDiagramLayouts();
                    UpdateDiagram();
                    LoadRuleToEditor();
                    SaveCurrentState();

                    ClearOutput();
                    AppendOutput("Rule extracted to '");
                    AppendOutput(newName.c_str());
                    AppendOutput("'!\n");

                    ImGui::CloseCurrentPopup();
                    } // else replaceOk
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                s_extractCoverNode = nullptr;
                ImGui::CloseCurrentPopup();
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
                ImGui::BulletText("Ctrl+Z - Undo");
                ImGui::BulletText("Ctrl+Y - Redo");
                ImGui::BulletText("Ctrl+N - New grammar");
                ImGui::BulletText("Ctrl+O - Open file");
                ImGui::BulletText("Ctrl+S - Save file");
                ImGui::BulletText("F5    - Parse grammar");
                ImGui::BulletText("Ctrl+A - Select all (diagram)");
                ImGui::BulletText("Delete - Delete selected (diagram)");
                ImGui::BulletText("Escape - Deselect all");
                ImGui::BulletText("Ctrl+Scroll - Zoom diagram");
                ImGui::Separator();
                if (ImGui::Button("Close")) showHelp = false;
                ImGui::EndPopup();
            }
        }

        // ===== RENDERING =====
#ifdef _WIN32
        ImGui::Render();
        const float clear_color[4] = { 1.00f, 1.00f, 1.00f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
#else
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(g_Window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(g_Window);
#endif
    }

    // Cleanup
#ifdef _WIN32
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
#else
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(g_Window);
    glfwTerminate();
#endif

    return 0;
}

void BuildRule() {
    if (!grammar) return;
    
    auto nts = grammar->getNonTerminals();
    if (activeNTIndex < 0 || activeNTIndex >= static_cast<int>(nts.size())) return;
    
    std::string ntName = nts[activeNTIndex];
    std::string newRule = ruleText;
    
    std::string text = grammarText;
    size_t pos = text.find(ntName + " :");
    if (pos == std::string::npos) pos = text.find(ntName + ":");
    if (pos == std::string::npos) return;
    
    size_t colonPos = text.find(":", pos);
    size_t dotPos = text.find(".", colonPos);
    if (colonPos == std::string::npos || dotPos == std::string::npos) return;
    
    if (!newRule.empty() && newRule.back() == '.') {
        newRule.pop_back();
    }
    
    text.replace(colonPos + 1, dotPos - colonPos - 1, " " + newRule);
    
    if (text.size() < sizeof(grammarText)) {
        strcpy_s(grammarText, sizeof(grammarText), text.c_str());
        
        FindAndCreateMissingNonTerminals(newRule);
        
        RebuildGrammarFromText();
        SaveCurrentState();
        RefreshRecursionResults();

        ClearOutput();
        AppendOutput("Rule updated for '");
        AppendOutput(ntName.c_str());
        AppendOutput("'\n");
    }
}
#ifdef _WIN32
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
#endif