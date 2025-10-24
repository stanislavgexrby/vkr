#include <syngt/graphics/DrawPoint.h>

namespace syngt {
namespace graphics {

void DrawPoint::setX(int x) {
    m_x = x;
}

void DrawPoint::setY(int y) {
    m_y = y;
}

void DrawPoint::setPlace(int x, int y) {
    m_x = x;
    m_y = y;
}

void DrawPoint::move(int dx, int dy) {
    m_x += dx;
    m_y += dy;
}

}
}