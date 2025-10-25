#pragma once
#include <syngt/graphics/Ward.h>
#include <syngt/utils/Semantic.h>
#include <memory>

namespace syngt {
namespace graphics {

constexpr int SpaceLength = 5;
constexpr int MinArrowLength = 2 * SpaceLength;
constexpr int SpikeLength = 7;
constexpr int SpikeWidth = 3;
constexpr int TextHeight = 15;

enum ArrowType {
    ctArrow = 0,
    ctSemanticArrow = 1
};

class DrawPoint;

/**
 * @brief Стрелка между графическими объектами
 * 
 * Представляет связь между двумя DrawPoint объектами.
 * Хранит длину, направление и указатель на источник.
 */
class Arrow {
protected:
    int m_length;
    DrawPoint* m_fromDO;  // Не владеет
    int m_ward;
    
public:
    Arrow();
    virtual ~Arrow() = default;
    
    /**
     * @brief Конструктор со всеми параметрами
     */
    Arrow(int ward, DrawPoint* fromDO);
    
    /**
     * @brief Получить длину стрелки
     */
    virtual int getLength() const;
    
    /**
     * @brief Получить источник стрелки
     */
    DrawPoint* getFromDO() const { return m_fromDO; }
    
    /**
     * @brief Установить источник стрелки
     */
    void setFromDO(DrawPoint* fromDO) { m_fromDO = fromDO; }
    
    /**
     * @brief Получить направление
     */
    int ward() const { return m_ward; }
    
    /**
     * @brief Установить направление
     */
    void setWard(int ward) { m_ward = ward; }
    
    /**
     * @brief Создать копию стрелки
     */
    virtual std::unique_ptr<Arrow> copy() const;
    
    /**
     * @brief Сохранить стрелку (возвращает источник)
     */
    virtual DrawPoint* save();
};

/**
 * @brief Стрелка с семантическими действиями
 * 
 * Расширяет Arrow добавлением списка семантических ID.
 * Длина стрелки увеличивается на длину семантик.
 */
class SemanticArrow : public Arrow {
private:
    std::unique_ptr<SemanticIDList> m_semantics;
    
public:
    SemanticArrow() = default;
    
    /**
     * @brief Конструктор с семантиками
     */
    SemanticArrow(int ward, DrawPoint* fromDO, std::unique_ptr<SemanticIDList> semantics);
    
    ~SemanticArrow() override = default;
    
    /**
     * @brief Получить семантики
     */
    SemanticIDList* getSemantics() const { return m_semantics.get(); }
    
    /**
     * @brief Создать копию с семантиками
     */
    std::unique_ptr<Arrow> copy() const override;
    
    /**
     * @brief Сохранить стрелку с семантиками
     */
    DrawPoint* save() override;
};

} // namespace graphics
} // namespace syngt