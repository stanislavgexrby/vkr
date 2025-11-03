#include <syngt/graphics/DrawObject.h>
#include <syngt/core/Grammar.h>
#include <cmath>
#include <algorithm>

namespace syngt {
namespace graphics {

bool DrawObject::internalPoint(int ax, int ay) const {
    return (ax >= m_x) && (ax <= endX()) && 
           (std::abs(ay - m_y) <= NS_Radius);
}

void DrawObject::setPlaceToRight() {
    if (fromDO()) {
        m_x = fromDO()->endX() + m_inArrow->getLength();
        m_y = fromDO()->y();
    }
}

void DrawObject::setPlaceToDown(int cy) {
    if (fromDO()) {
        m_x = fromDO()->x();
        m_y = fromDO()->y() + cy;
    }
}

void DrawObject::setAll(int x, int y, std::unique_ptr<Arrow> inArrow) {
    m_x = x;
    m_y = y;
    m_inArrow = std::move(inArrow);
}

void DrawObject::copyAll(const DrawObject* other) {
    if (!other) return;
    
    m_x = other->m_x;
    m_y = other->m_y;
    if (other->m_inArrow) {
        m_inArrow = other->m_inArrow->copy();
    }
}

DrawObject* DrawObject::addExtendedPoint(Grammar* grammar) {
    (void)grammar;
    if (!m_inArrow) return nullptr;
    
    auto extPoint = new DrawObjectExtendedPoint();
    
    int curWard;
    if (m_inArrow->ward() == cwFORWARD) {
        m_inArrow->setWard(cwNONE);
        curWard = cwFORWARD;
    } else {
        curWard = cwNONE;
    }
    
    DrawPoint* fromDP = m_inArrow->getFromDO();
    extPoint->setInArrow(std::move(m_inArrow));
    extPoint->setPosition((fromDP->endX() + m_x) / 2, m_y);
    
    m_inArrow = std::make_unique<Arrow>(curWard, extPoint);
    
    return extPoint;
}

void DrawObject::selectAllNotSelected() {
    m_selected = true;
    if (m_inArrow) {
        DrawPoint* fromDP = m_inArrow->getFromDO();
        if (auto fromDO = dynamic_cast<DrawObject*>(fromDP)) {
            if (!fromDO->selected()) {
                fromDO->selectAllNotSelected();
            }
        }
    }
}

bool DrawObjectExtendedPoint::internalPoint(int ax, int ay) const {
    return (std::abs(m_x - ax) <= 2) && (std::abs(m_y - ay) <= 2);
}

void DrawObjectPoint::setX(int x) {
    DrawObject::setX(x);
    if (m_inArrow2) {
        m_inArrow2->getFromDO()->setXCoord(x);
    }
}

void DrawObjectPoint::selectAllNotSelected() {
    DrawObjectExtendedPoint::selectAllNotSelected();
    
    if (m_inArrow2) {
        DrawPoint* fromDP = m_inArrow2->getFromDO();
        if (auto fromDO = dynamic_cast<DrawObject*>(fromDP)) {
            if (!fromDO->selected()) {
                fromDO->selectAllNotSelected();
            }
        }
    }
}

void DrawObjectBorder::setX(int x) {
    DrawObject::setX(x);
    m_changed = true;
}

void DrawObjectBorder::setY(int y) {
    DrawObject::setY(y);
    m_changed = true;
}

void DrawObjectBorder::setPlace(int x, int y) {
    DrawObject::setPlace(x, y);
    m_changed = true;
}

void DrawObjectFirst::placePoints() {
    m_points[0] = {m_x, m_y - NS_Radius};
    m_points[1] = {m_x, m_y + NS_Radius};
    m_points[2] = {endX(), m_y};
}

void DrawObjectFirst::place() {
    setPosition(HorizontalSkipFromBorder,
                VerticalSkipFromBorder + VerticalSkipForName + NS_Radius);
}

void DrawObjectLast::placePoints() {
    m_points[0] = {endX(), m_y - NS_Radius};
    m_points[1] = {endX(), m_y + NS_Radius};
    m_points[2] = {m_x, m_y};
}

DrawObjectLeaf::DrawObjectLeaf(Grammar* grammar, int id) 
    : m_id(id)
    , m_grammar(grammar)
{
}

void DrawObjectLeaf::setLengthFromString(const std::string& s) {
    const int charWidth = 10;      
    const int padding = 20;         
    const int minTotalWidth = 50;  
    
    int textWidth = static_cast<int>(s.length()) * charWidth;
    int totalWidth = (NS_Radius * 2) + padding + textWidth;
    
    if (totalWidth < minTotalWidth) {
        totalWidth = minTotalWidth;
    }
    
    m_length = totalWidth;
}

DrawObjectTerminal::DrawObjectTerminal(Grammar* grammar, int id)
    : DrawObjectLeaf(grammar, id)
{
    std::string name = getNameFromGrammar();
    if (!name.empty()) {
        setLengthFromString(name);
    } else {
        m_length = NS_NullTerminal * 2;
    }
}

std::string DrawObjectTerminal::getNameFromGrammar() const {
    if (!m_grammar) return "";
    auto terminals = m_grammar->getTerminals();
    if (m_id >= 0 && m_id < static_cast<int>(terminals.size())) {
        return terminals[m_id];
    }
    return "";
}

DrawObjectNonTerminal::DrawObjectNonTerminal(Grammar* grammar, int id)
    : DrawObjectLeaf(grammar, id)
{
    std::string name = getNameFromGrammar();
    if (!name.empty()) {
        setLengthFromString(name);
    } else {
        m_length = NS_NullTerminal * 2;
    }
}

std::string DrawObjectNonTerminal::getNameFromGrammar() const {
    if (!m_grammar) return "";
    auto nts = m_grammar->getNonTerminals();
    if (m_id >= 0 && m_id < static_cast<int>(nts.size())) {
        return nts[m_id];
    }
    return "";
}

DrawObjectMacro::DrawObjectMacro(Grammar* grammar, int id)
    : DrawObjectNonTerminal(grammar, id)
{
    // Длина устанавливается в родительском конструкторе DrawObjectNonTerminal
}

std::string DrawObjectMacro::getNameFromGrammar() const {
    if (!m_grammar) return "";
    auto macros = m_grammar->getMacros();
    if (m_id >= 0 && m_id < static_cast<int>(macros.size())) {
        return macros[m_id];
    }
    return "";
}

void IntegerList::remove(int id) {
    auto it = std::find(m_items.begin(), m_items.end(), id);
    if (it != m_items.end()) {
        m_items.erase(it);
    }
}

DrawObjectList::DrawObjectList(Grammar* grammar)
    : m_grammar(grammar)
{
}

void DrawObjectList::initialize() {
    clear();
    
    auto first = std::make_unique<DrawObjectFirst>();
    first->place();
    
    auto last = std::make_unique<DrawObjectLast>();
    last->setInArrow(std::make_unique<Arrow>(cwFORWARD, first.get()));
    
    add(std::move(first));
    add(std::move(last));
}

void DrawObjectList::add(std::unique_ptr<DrawObject> obj) {
    m_items.push_back(std::move(obj));
}

void DrawObjectList::clearExceptFirst() {
    for (int i = count() - 1; i >= 1; --i) {
        m_items.erase(m_items.begin() + i);
    }
}

void DrawObjectList::clear() {
    m_items.clear();
    m_selectedList.clear();
}

int DrawObjectList::indexOf(const DrawObject* obj) const {
    for (int i = 0; i < count(); ++i) {
        if (m_items[i].get() == obj) {
            return i;
        }
    }
    return -1;
}

DrawObject* DrawObjectList::findDO(int x, int y) const {
    for (int i = 0; i < count(); ++i) {
        if (m_items[i]->internalPoint(x, y)) {
            return m_items[i].get();
        }
    }
    return nullptr;
}

void DrawObjectList::selectedMove(int dx, int dy) {
    for (int id : m_selectedList.items()) {
        if (id >= 0 && id < count()) {
            m_items[id]->move(dx, dy);
        }
    }
}

void DrawObjectList::unselectAll() {
    for (auto& obj : m_items) {
        obj->setSelected(false);
    }
    m_selectedList.clear();
}

static bool inInterval(int a1, int a2, int a) {
    if (a1 > a2) {
        std::swap(a1, a2);
    }
    return (a >= a1) && (a <= a2);
}

void DrawObjectList::changeSelectionInRect(int left, int top, int right, int bottom) {
    for (int i = 0; i < count(); ++i) {
        DrawObject* obj = m_items[i].get();
        if (inInterval(top, bottom, obj->y()) && 
            inInterval(left, right, obj->x())) {
            
            if (obj->selected()) {
                m_selectedList.remove(i);
                obj->setSelected(false);
            } else {
                m_selectedList.add(i);
                obj->setSelected(true);
            }
        }
    }
}

void DrawObjectList::changeSelection(DrawObject* target) {
    int i = indexOf(target);
    if (i < 0) return;
    
    if (target->selected()) {
        m_selectedList.remove(i);
        target->setSelected(false);
    } else {
        m_selectedList.add(i);
        target->setSelected(true);
    }
}

void DrawObjectList::selectAllNotSelected(DrawObject* target) {
    m_selectedList.clear();
    target->selectAllNotSelected();
    
    for (int i = 0; i < count(); ++i) {
        if (m_items[i]->selected()) {
            m_selectedList.add(i);
        }
    }
}

void DrawObjectList::addExtendedPoint() {
    if (m_selectedList.count() != 1) return;
    
    DrawObject* selected = m_items[m_selectedList[0]].get();
    DrawObject* extPoint = selected->addExtendedPoint(m_grammar);
    
    if (extPoint) {
        add(std::unique_ptr<DrawObject>(extPoint));
    }
}

} // namespace graphics
} // namespace syngt