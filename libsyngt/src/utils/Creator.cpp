#include <syngt/utils/Creator.h>
#include <syngt/regex/RETree.h>
#include <syngt/graphics/DrawObject.h>
#include <syngt/graphics/Arrow.h>
#include <syngt/graphics/GraphicsConstants.h>
#include <syngt/core/Grammar.h>
#include <syngt/utils/Semantic.h>

namespace syngt {
namespace Creator {

using namespace syngt::graphics;

// (временное решение до рефакторинга DrawObject)
class DrawObjectLastHelper : public DrawObjectLast {
public:
    void setPositionPublic(int x, int y) {
        setPlace(x, y);
    }
};

void createDrawObjects(
    graphics::DrawObjectList* list,
    const RETree* tree
) {
    if (!list || !tree) {
        return;
    }
    
    list->clear();
    
    auto firstDO = std::make_unique<DrawObjectFirst>();
    firstDO->place();
    
    DrawObjectFirst* firstPtr = firstDO.get();
    list->add(std::move(firstDO));
    
    SemanticIDList* semantics = nullptr;
    int height = 0;
    
    DrawObject* prevDO = const_cast<RETree*>(tree)->drawObjectsToRight(
        list, semantics, firstPtr, cwFORWARD, height
    );
    
    auto lastDO = std::make_unique<DrawObjectLast>();
    
    std::unique_ptr<Arrow> inArrow;
    if (semantics == nullptr) {
        inArrow = std::make_unique<Arrow>(cwFORWARD, prevDO);
    } else {
        auto semList = std::unique_ptr<SemanticIDList>(semantics);
        inArrow = std::make_unique<SemanticArrow>(cwFORWARD, prevDO, std::move(semList));
    }
    
    lastDO->setInArrow(std::move(inArrow));
    
    int lastX = prevDO->endX() + lastDO->inArrow()->getLength();
    int lastY = prevDO->y();
    lastDO->setPositionForCreator(lastX, lastY);
    
    DrawObjectLast* lastPtr = lastDO.get();
    list->add(std::move(lastDO));
    
    list->setWidth(lastPtr->endX() + HorizontalSkipFromBorder);
    list->setHeight(height + firstPtr->y() + VerticalSkipFromBorder + NS_Radius);
}

} // namespace Creator
} // namespace syngt