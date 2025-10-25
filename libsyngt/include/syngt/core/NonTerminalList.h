#pragma once
#include <syngt/core/NTListItem.h>
#include <vector>
#include <memory>
#include <string>

namespace syngt {

class Grammar;
class RETree;

class NonTerminalList {
private:
    std::vector<std::string> m_list;
    std::vector<std::unique_ptr<NTListItem>> m_items;
    Grammar* m_grammar = nullptr;
    
public:
    NonTerminalList() = default;
    ~NonTerminalList() = default;
    
    void fillNew();
    int add(const std::string& s);
    void clear();
    
    int find(const std::string& s) const {
        for (size_t i = 0; i < m_list.size(); ++i) {
            if (m_list[i] == s) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
    int getCount() const {
        return static_cast<int>(m_list.size());
    }
    
    std::string getString(int index) const {
        if (index >= 0 && index < static_cast<int>(m_list.size())) {
            return m_list[index];
        }
        return "";
    }
    
    std::vector<std::string> getItems() const {
        return m_list;
    }
    
    void setGrammar(Grammar* grammar);
    
    NTListItem* getItem(int index) const {
        if (index >= 0 && index < static_cast<int>(m_items.size())) {
            return m_items[index].get();
        }
        return nullptr;
    }
    
    NTListItem* getItemByName(const std::string& name) const {
        int index = find(name);
        if (index >= 0) {
            return getItem(index);
        }
        return nullptr;
    }
    
    void setRoot(int index, std::unique_ptr<RETree> root);
    
    void setRootByName(const std::string& name, std::unique_ptr<RETree> root);
};

}