#pragma once
#include <syngt/regex/RELeaf.h>

namespace syngt {

class NTListItem;

/**
 * @brief Базовый класс для макросов
 * 
 * Макросы - это символы, которые могут быть раскрыты (opened) или закрыты.
 * Включает нетерминалы и специальные макросы.
 * 
 * Соответствует Pascal: TRE_Macro
 */
class REMacro : public RELeaf {
protected:
    bool m_isOpen = false;
    
    /**
     * @brief Получить корень дерева (если макрос раскрыт)
     */
    virtual RETree* getRoot() const = 0;
    
    /**
     * @brief Получить элемент списка (NTListItem или макрос)
     */
    virtual NTListItem* getListItem() const = 0;
    
public:
    REMacro() = default;
    virtual ~REMacro() = default;
    
    bool isOpen() const { return m_isOpen; }
    void setOpen(bool open) { m_isOpen = open; }
};

}