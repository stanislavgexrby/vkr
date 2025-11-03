#pragma once
#include <syngt/graphics/DrawPoint.h>
#include <syngt/graphics/Arrow.h>
#include <syngt/graphics/GraphicsConstants.h>
#include <memory>
#include <vector>
#include <string>

namespace syngt {

class Grammar;

namespace graphics {

class DrawObjectList;

/**
 * @brief Базовый класс для графических объектов синтаксической диаграммы
 * 
 * Наследует DrawPoint и добавляет:
 * - входящую стрелку (InArrow)
 * - возможность выделения (Selected)
 * - методы размещения (setPlaceToRight/Down)
 */
class DrawObject : public DrawPoint {
protected:
    bool m_selected = false;
    std::unique_ptr<Arrow> m_inArrow;
    
public:
    DrawObject() = default;
    virtual ~DrawObject() = default;
    
    /**
     * @brief Тип объекта (для сериализации)
     */
    virtual int getType() const = 0;
    
    /**
     * @brief Получить длину объекта
     */
    int getLength() const override { return 0; }
    
    /**
     * @brief Нужен ли spike на стрелке
     */
    virtual bool needSpike() const { return true; }
    
    /**
     * @brief Проверка попадания точки в объект
     */
    virtual bool internalPoint(int ax, int ay) const;
    
    /**
     * @brief Разместить объект справа от предыдущего
     */
    void setPlaceToRight();
    
    /**
     * @brief Разместить объект снизу от предыдущего
     */
    void setPlaceToDown(int cy);
    
    /**
     * @brief Установить все параметры сразу
     */
    void setAll(int x, int y, std::unique_ptr<Arrow> inArrow);
    
    /**
     * @brief Скопировать параметры из другого объекта
     */
    void copyAll(const DrawObject* other);
    
    /**
     * @brief Добавить расширенную точку между объектами
     */
    DrawObject* addExtendedPoint(Grammar* grammar);
    
    /**
     * @brief Выделить все связанные невыделенные объекты
     */
    virtual void selectAllNotSelected();
    
    bool selected() const { return m_selected; }
    void setSelected(bool selected) { m_selected = selected; }
    
    Arrow* inArrow() const { return m_inArrow.get(); }
    void setInArrow(std::unique_ptr<Arrow> arrow) { m_inArrow = std::move(arrow); }
    
    DrawPoint* fromDO() const {
        return m_inArrow ? m_inArrow->getFromDO() : nullptr;
    }
};

/**
 * @brief Расширенная точка (маленький кружок на стрелке)
 * 
 * Используется для разветвлений и точек выбора.
 */
class DrawObjectExtendedPoint : public DrawObject {
public:
    DrawObjectExtendedPoint() = default;
    
    int getType() const override { return ctDrawObjectExtendedPoint; }
    bool internalPoint(int ax, int ay) const override;
};

/**
 * @brief Точка с двумя входящими стрелками
 * 
 * Используется для схождения альтернатив.
 */
class DrawObjectPoint : public DrawObjectExtendedPoint {
protected:
    std::unique_ptr<Arrow> m_inArrow2;
    
    void setX(int x) override;
    
public:
    DrawObjectPoint() = default;
    
    int getType() const override { return ctDrawObjectPoint; }
    bool needSpike() const override { return false; }
    
    void selectAllNotSelected() override;
    
    Arrow* secondInArrow() const { return m_inArrow2.get(); }
    void setSecondInArrow(std::unique_ptr<Arrow> arrow) { m_inArrow2 = std::move(arrow); }
};

/**
 * @brief Граничный объект (треугольник)
 * 
 * Базовый класс для First и Last.
 */
class DrawObjectBorder : public DrawObject {
protected:
    struct Point {
        int x, y;
    };
    Point m_points[3];
    bool m_changed = true;
    
    void setX(int x) override;
    void setY(int y) override;
    void setPlace(int x, int y) override;
    
    virtual void placePoints() = 0;
    
public:
    DrawObjectBorder() = default;
    
    int getLength() const override { return NS_Radius; }
    
    const Point* points() const { return m_points; }
    
    void updatePointsIfNeeded() {
        if (m_changed) {
            placePoints();
            m_changed = false;
        }
    }
};

/**
 * @brief Первый объект (начало диаграммы)
 * 
 * Треугольник направленный вправо.
 */
class DrawObjectFirst : public DrawObjectBorder {
protected:
    void placePoints() override;
    
public:
    DrawObjectFirst() = default;
    
    int getType() const override { return ctDrawObjectFirst; }
    
    /**
     * @brief Разместить в начальной позиции
     */
    void place();
};

/**
 * @brief Последний объект (конец диаграммы)
 * 
 * Треугольник направленный влево.
 */
class DrawObjectLast : public DrawObjectBorder {
protected:
    void placePoints() override;
    
public:
    DrawObjectLast() = default;
    
    int getType() const override { return ctDrawObjectLast; }

    void setPositionForCreator(int x, int y) {
        setPlace(x, y);
    }
};

/**
 * @brief Листовой объект с ID и текстом
 * 
 * Базовый для Terminal, NonTerminal, Macro.
 */
class DrawObjectLeaf : public DrawObject {
protected:
    int m_id = 0;
    int m_length = 0;
    Grammar* m_grammar = nullptr;
    
    /**
     * @brief Получить имя элемента из грамматики
     */
    virtual std::string getNameFromGrammar() const = 0;
    
    /**
     * @brief Установить длину по строке (учитывая радиус)
     */
    void setLengthFromString(const std::string& s);
    
public:
    DrawObjectLeaf() = default;
    explicit DrawObjectLeaf(Grammar* grammar, int id);
    
    int getLength() const override { return m_length; }
    
    int id() const { return m_id; }
    std::string name() const { return getNameFromGrammar(); }
    Grammar* grammar() const { return m_grammar; }
};

/**
 * @brief Терминал (в овале)
 */
class DrawObjectTerminal : public DrawObjectLeaf {
protected:
    std::string getNameFromGrammar() const override;
    
public:
    DrawObjectTerminal() = default;
    explicit DrawObjectTerminal(Grammar* grammar, int id);
    
    int getType() const override { return ctDrawObjectTerminal; }
};

/**
 * @brief Нетерминал (в прямоугольнике)
 */
class DrawObjectNonTerminal : public DrawObjectLeaf {
protected:
    std::string getNameFromGrammar() const override;
    
public:
    DrawObjectNonTerminal() = default;
    explicit DrawObjectNonTerminal(Grammar* grammar, int id);
    
    int getType() const override { return ctDrawObjectNonTerminal; }
};

/**
 * @brief Макрос (в пунктирном прямоугольнике)
 */
class DrawObjectMacro : public DrawObjectNonTerminal {
protected:
    std::string getNameFromGrammar() const override;
    
public:
    DrawObjectMacro() = default;
    explicit DrawObjectMacro(Grammar* grammar, int id);
    
    int getType() const override { return ctDrawObjectMacro; }
};

/**
 * @brief Список для хранения ID выделенных объектов
 */
class IntegerList {
private:
    std::vector<int> m_items;
    
public:
    void add(int id) { m_items.push_back(id); }
    void remove(int id);
    void clear() { m_items.clear(); }
    int count() const { return static_cast<int>(m_items.size()); }
    int operator[](int index) const { return m_items[index]; }
    const std::vector<int>& items() const { return m_items; }
};

/**
 * @brief Контейнер графических объектов
 * 
 * Управляет списком DrawObject, выделением, размещением.
 */
class DrawObjectList {
private:
    std::vector<std::unique_ptr<DrawObject>> m_items;
    IntegerList m_selectedList;
    int m_height = 0;
    int m_width = 0;
    Grammar* m_grammar = nullptr;
    
public:
    DrawObjectList() = default;
    explicit DrawObjectList(Grammar* grammar);
    ~DrawObjectList() = default;
    
    void initialize();
    
    void add(std::unique_ptr<DrawObject> obj);
    
    void clearExceptFirst();
    
    void clear();
    
    int count() const { return static_cast<int>(m_items.size()); }
    
    DrawObject* operator[](int index) const {
        if (index >= 0 && index < count()) {
            return m_items[index].get();
        }
        return nullptr;
    }
    
    int indexOf(const DrawObject* obj) const;
    
    DrawObject* findDO(int x, int y) const;
    
    /**
     * @brief Переместить выделенные объекты
     */
    void selectedMove(int dx, int dy);
    
    /**
     * @brief Снять все выделения
     */
    void unselectAll();
    
    /**
     * @brief Изменить выделение в прямоугольнике
     */
    void changeSelectionInRect(int left, int top, int right, int bottom);
    
    /**
     * @brief Изменить выделение объекта
     */
    void changeSelection(DrawObject* target);
    
    /**
     * @brief Выделить все связанные с объектом
     */
    void selectAllNotSelected(DrawObject* target);
    
    /**
     * @brief Добавить расширенную точку к выделенному
     */
    void addExtendedPoint();
    
    int height() const { return m_height; }
    void setHeight(int h) { m_height = h; }
    
    int width() const { return m_width; }
    void setWidth(int w) { m_width = w; }
    
    Grammar* grammar() const { return m_grammar; }
    
    const IntegerList& selectedList() const { return m_selectedList; }
};

} // namespace graphics
} // namespace syngt