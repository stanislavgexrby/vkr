#include <syngt/graphics/Arrow.h>
#include <syngt/graphics/DrawPoint.h>
#include <iostream>

namespace syngt {
namespace graphics {

Arrow::Arrow() 
    : m_length(MinArrowLength)
    , m_fromDO(nullptr)
    , m_ward(cwNONE)
{
}

Arrow::Arrow(int ward, DrawPoint* fromDO)
    : m_length(MinArrowLength)
    , m_fromDO(fromDO)
    , m_ward(ward)
{
    if (ward != cwNONE) {
        m_length += SpikeLength;
    }
}

int Arrow::getLength() const {
    return m_length;
}

std::unique_ptr<Arrow> Arrow::copy() const {
    auto result = std::make_unique<Arrow>();
    result->m_length = m_length;
    result->m_fromDO = m_fromDO;
    result->m_ward = m_ward;
    return result;
}

DrawPoint* Arrow::save() {
    std::cout << ctArrow << "\n";
    std::cout << m_ward << "\n";
    return m_fromDO;
}

SemanticArrow::SemanticArrow(int ward, DrawPoint* fromDO, std::unique_ptr<SemanticIDList> semantics)
    : Arrow()
    , m_semantics(std::move(semantics))
{
    m_ward = ward;
    m_fromDO = fromDO;
    m_length = MinArrowLength;
    
    if (m_semantics) {
        m_length += m_semantics->getLength();
    }
    
    if (ward != cwNONE) {
        m_length += SpikeLength;
    }
}

std::unique_ptr<Arrow> SemanticArrow::copy() const {
    std::unique_ptr<SemanticIDList> semanticsCopy;
    if (m_semantics) {
        semanticsCopy = m_semantics->copy();
    }
    
    return std::make_unique<SemanticArrow>(m_ward, m_fromDO, std::move(semanticsCopy));
}

DrawPoint* SemanticArrow::save() {
    std::cout << ctSemanticArrow << "\n";
    std::cout << m_ward << "\n";
    
    if (m_semantics) {
        m_semantics->save();
    }
    
    return m_fromDO;
}

} // namespace graphics
} // namespace syngt