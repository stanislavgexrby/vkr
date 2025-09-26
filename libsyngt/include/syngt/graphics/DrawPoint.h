#pragma once

namespace syngt {
namespace graphics {

class DrawPoint {
protected:
    int m_x = 0;
    int m_y = 0;
    
    virtual void setX(int x) { m_x = x; }
    virtual void setY(int y) { m_y = y; }
    
public:
    DrawPoint() = default;
    virtual ~DrawPoint() = default;

    virtual int getLength() const = 0;

    void setPlace(int x, int y) {
        m_x = x;
        m_y = y;
    }
    
    int x() const { return m_x; }
    int y() const { return m_y; }

    virtual int endX() const {
        return m_x + getLength();
    }

    virtual int endY() const {
        return m_y;
    }
};

}
}