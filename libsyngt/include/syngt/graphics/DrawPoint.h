#pragma once

namespace syngt {
namespace graphics {

class DrawPoint {
protected:
    int m_x = 0;
    int m_y = 0;

    /**
     * @brief Виртуальный сеттер для x (может быть переопределен)
     */
    virtual void setX(int x);
    
    /**
     * @brief Виртуальный сеттер для y (может быть переопределен)
     */
    virtual void setY(int y);
    
    /**
     * @brief Установить позицию (виртуальный)
     */
    virtual void setPlace(int x, int y);

public:
    DrawPoint() = default;
    virtual ~DrawPoint() = default;
    
    /**
     * @brief Получить длину объекта (чисто виртуальная функция)
     * 
     * Должна быть переопределена в наследниках.
     * Например, для терминала - это ширина текста.
     * 
     * @return Длина объекта в пикселях
     */
    virtual int getLength() const = 0;
    
    /**
     * @brief Получить координату X
     */
    int x() const { return m_x; }
    
    /**
     * @brief Получить координату Y
     */
    int y() const { return m_y; }
    
    /**
     * @brief Получить конечную координату X (x + длина)
     */
    int endX() const { return m_x + getLength(); }
    
    /**
     * @brief Получить конечную координату Y (равна Y)
     */
    int endY() const { return m_y; }
    
    /**
     * @brief Переместить объект на dx, dy
     */
    void move(int dx, int dy);
    
    /**
     * @brief Установить координату X (через виртуальный метод)
     */
    void setXCoord(int x) { setX(x); }
    
    /**
     * @brief Установить координату Y (через виртуальный метод)
     */
    void setYCoord(int y) { setY(y); }
    
    /**
     * @brief Установить позицию (через виртуальный метод)
     */
    void setPosition(int x, int y) { setPlace(x, y); }
};

}
}