#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace syngt {

class NTListItem;

class NonTerminalList {
private:
    std::vector<std::string> m_items;
    
public:
    NonTerminalList() = default;
    ~NonTerminalList() = default;
    
    int add(const std::string& s);

    int find(const std::string& s) const;

    std::string getString(int index) const;

    int getCount() const { return static_cast<int>(m_items.size()); }

    const std::vector<std::string>& getItems() const { return m_items; }

    void fillNew();

    void clear() { m_items.clear(); }
};

}