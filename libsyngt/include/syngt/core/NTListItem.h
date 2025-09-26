#pragma once
#include <syngt/core/Types.h>
#include <syngt/regex/RETree.h>
#include <memory>
#include <string>

namespace syngt {

// Forward declarations
namespace graphics {
    class DrawObjectList;
}

/**
 * @brief Элемент нетерминала
 * 
 * Содержит:
 * - Имя нетерминала
 * - Текстовое правило (value)
 * - Дерево RE (root)
 * - Графическое представление (draw)
 * 
 * Соответствует Pascal: TNTListItem
 */
class NTListItem {
private:
    std::string m_name;                              // Имя нетерминала
    std::string m_value;                             // Текстовое правило
    std::unique_ptr<RETree> m_root;                  // Дерево RE
    // std::unique_ptr<graphics::DrawObjectList> m_draw;  // Графика (добавим позже)
    int m_mark = cmNotMarked;                        // Маркер
    
    void setValueFromRoot();
    void setStringFromRoot();
    void setRootFromValue();
    
public:
    NTListItem() = default;
    explicit NTListItem(const std::string& name) : m_name(name) {}
    ~NTListItem() = default;
    
    /**
     * @brief Установить значение и распарсить в дерево
     */
    void setValue(const std::string& value);
    
    /**
     * @brief Создать копию дерева RE
     */
    std::unique_ptr<RETree> copyRETree() const {
        if (m_root) {
            return m_root->copy();
        }
        return nullptr;
    }
    
    const std::string& name() const { return m_name; }
    const std::string& value() const { return m_value; }
    RETree* root() const { return m_root.get(); }
    int mark() const { return m_mark; }
    
    void setName(const std::string& name) { m_name = name; }
    void setRoot(std::unique_ptr<RETree> root) { m_root = std::move(root); }
    void setMark(int mark) { m_mark = mark; }
    
    // Размеры (пока заглушки, будут работать когда добавим DrawObjectList)
    int height() const { return 0; /* m_draw->height() */ }
    int width() const { return 0; /* m_draw->width() */ }
};

}