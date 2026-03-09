#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace syngt {

class TerminalList {
private:
    std::vector<std::string> m_items;
    
public:
    TerminalList() = default;
    ~TerminalList() = default;
    
    int add(const std::string& s);
    
    int find(const std::string& s) const;
    
    std::string getString(int index) const;

    // Returns the actual stored string without any display substitution.
    // For the epsilon terminal (stored as "") this returns "" rather than "@".
    std::string getRawString(int index) const {
        if (index < 0 || index >= static_cast<int>(m_items.size())) {
            return "";
        }
        return m_items[index];
    }
    
    int getCount() const { return static_cast<int>(m_items.size()); }
    
    const std::vector<std::string>& getItems() const { return m_items; }

    void clear() { m_items.clear(); }
};

}