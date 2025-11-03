#pragma once
#include <syngt/core/Types.h>
#include <memory>
#include <string>

namespace syngt {
    class SemanticIDList;
}

namespace syngt {
namespace graphics {
    class DrawObjectList;
    class DrawObject;
}

/**
 * @brief Базовый абстрактный класс для дерева регулярных выражений
 * 
 * Представляет узел в синтаксическом дереве грамматики.
 * Может быть: Terminal, NonTerminal, Semantic, Or, And, Iteration и т.д.
 */
class RETree {
protected:
    int m_drawObj = -1;
    
public:
    RETree() = default;
    virtual ~RETree() = default;
    
    /**
     * @brief Создать копию узла
     */
    virtual std::unique_ptr<RETree> copy() const = 0;
    
    /**
     * @brief Сохранить в поток
     */
    virtual void save() = 0;
    
    /**
     * @brief Преобразовать в строку
     * @param mask Маска выделения
     * @param reverse Обратный порядок (для right elimination)
     */
    virtual std::string toString(const SelectionMask& mask, bool reverse) const = 0;
    
    /**
     * @brief Заменить все пустые узлы
     */
    virtual void substituteAllEmpty() = 0;
    
    /**
     * @brief Снять все отметки
     */
    virtual void unmarkAll() = 0;
    
    /**
     * @brief Попытаться установить пустую отметку
     */
    virtual void tryToSetEmptyMark() = 0;
    
    /**
     * @brief Левый потомок (для бинарных операций)
     */
    virtual RETree* left() const { return nullptr; }
    
    /**
     * @brief Правый потомок (для бинарных операций)
     */
    virtual RETree* right() const { return nullptr; }
    
    /**
     * @brief Все ли макросы были раскрыты
     */
    virtual bool allMacroWasOpened() const { return true; }
    
    /**
     * @brief Все ли определения были закрыты
     */
    virtual bool allDefinitionWasClosed() const { return true; }

    virtual int getOperationCount() const = 0;
    
    int drawObj() const { return m_drawObj; }
    void setDrawObj(int index) { m_drawObj = index; }

    virtual syngt::graphics::DrawObject* drawObjectsToRight(
        syngt::graphics::DrawObjectList* list,
        syngt::SemanticIDList*& semantics,
        syngt::graphics::DrawObject* fromDO,
        int ward,
        int& height
    ) const;

};

}