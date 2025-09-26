#pragma once
#include <string>
#include <vector>

namespace syngt {

/**
 * @brief Список семантических действий
 * 
 * Семантики - это действия, выполняемые при разборе (например, вычисления).
 */
class SemanticList {
private:
    std::vector<std::string> m_items;
    
public:
    SemanticList() = default;
    ~SemanticList() = default;
    
    int add(const std::string& s);
    int find(const std::string& s) const;
    std::string getString(int index) const;
    int getCount() const { return static_cast<int>(m_items.size()); }
    const std::vector<std::string>& getItems() const { return m_items; }
    void clear() { m_items.clear(); }
};

}