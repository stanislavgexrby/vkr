#include <syngt/regex/RETree.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/REMacro.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REIteration.h>
#include <syngt/graphics/DrawObject.h>
#include <syngt/graphics/Arrow.h>
#include <syngt/graphics/Ward.h>
#include <syngt/graphics/GraphicsConstants.h>
#include <syngt/utils/Semantic.h>
#include <syngt/core/Grammar.h>

namespace syngt {

using namespace graphics;

static std::unique_ptr<Arrow> makeArrowForwardAlways(
    DrawObject* fromDO,
    int ward,
    SemanticIDList*& semantics
) {
    int curWard;
    if (fromDO->needSpike() || ward == cwFORWARD) {
        curWard = ward;
    } else {
        curWard = cwNONE;
    }
    
    if (semantics == nullptr) {
        return std::make_unique<Arrow>(curWard, fromDO);
    } else {
        auto arrow = std::make_unique<SemanticArrow>(
            curWard, fromDO, std::unique_ptr<SemanticIDList>(semantics)
        );
        semantics = nullptr;
        return arrow;
    }
}

// RETree::drawObjectsToRight
DrawObject* RETree::drawObjectsToRight(
    DrawObjectList* list,
    SemanticIDList*& semantics,
    DrawObject* fromDO,
    int ward,
    int& height
) const {
    (void)list;
    (void)ward;
    semantics = nullptr;
    height = 0;
    return fromDO;
}

// В RETerminal::drawObjectsToRight
DrawObject* RETerminal::drawObjectsToRight(
    DrawObjectList* list,
    SemanticIDList*& semantics,
    DrawObject* fromDO,
    int ward,
    int& height
) const {
    if (m_id != 0) {
        height = ElementHalfHeight;
    } else {
        height = 0;
    }
    
    if (m_id == 0) {
        semantics = nullptr;
        return fromDO;
    }
    
    auto terminal = std::make_unique<DrawObjectTerminal>(m_grammar, m_id);
    terminal->setInArrow(makeArrowForwardAlways(fromDO, ward, semantics));
    terminal->setPlaceToRight();
    
    DrawObject* result = terminal.get();
    list->add(std::move(terminal));
    m_drawObj = list->count() - 1;

    return result;
}

// REAnd::drawObjectsToRight
DrawObject* REAnd::drawObjectsToRight(
    DrawObjectList* list,
    SemanticIDList*& semantics,
    DrawObject* fromDO,
    int ward,
    int& height
) const {
    int height1 = 0;
    DrawObject* result;
    
    if (ward == cwFORWARD) {
        result = m_first->drawObjectsToRight(list, semantics, fromDO, ward, height1);
        result = m_second->drawObjectsToRight(list, semantics, result, ward, height);
    } else {
        result = m_second->drawObjectsToRight(list, semantics, fromDO, ward, height);
        result = m_first->drawObjectsToRight(list, semantics, result, ward, height1);
    }
    
    if (height < height1) {
        height = height1;
    }
    
    return result;
}

// REOr::drawObjectsToRight
DrawObject* REOr::drawObjectsToRight(
    DrawObjectList* list,
    SemanticIDList*& semantics,
    DrawObject* fromDO,
    int ward,
    int& height
) const {
    // Pascal binary recursive approach: two branches, top and bottom,
    // joined at a single joinPoint via inArrow (top) and secondInArrow (bottom).

    int curWard = ((ward == cwBACKWARD) && fromDO->needSpike()) ?
        cwBACKWARD : cwNONE;

    auto leftPoint = std::make_unique<DrawObjectPoint>();
    if (semantics == nullptr) {
        leftPoint->setInArrow(std::make_unique<Arrow>(curWard, fromDO));
    } else {
        auto arrow = std::make_unique<SemanticArrow>(
            curWard, fromDO, std::unique_ptr<SemanticIDList>(semantics)
        );
        semantics = nullptr;
        leftPoint->setInArrow(std::move(arrow));
    }
    leftPoint->setPlaceToRight();

    DrawObjectPoint* leftPtr = leftPoint.get();
    const int leftX = leftPtr->x();
    const int leftY = leftPtr->y();
    list->add(std::move(leftPoint));

    // Draw first (top) branch
    int height1 = 0;
    SemanticIDList* sem1 = nullptr;
    DrawObject* firstEnd = m_first->drawObjectsToRight(list, sem1, leftPtr, ward, height1);

    // Join point receives first branch
    auto joinPoint = std::make_unique<DrawObjectPoint>();
    int curWard1 = (ward == cwBACKWARD && firstEnd->needSpike()) ? cwBACKWARD : cwNONE;
    if (sem1 == nullptr) {
        joinPoint->setInArrow(std::make_unique<Arrow>(curWard1, firstEnd));
    } else {
        auto arrow = std::make_unique<SemanticArrow>(
            curWard1, firstEnd, std::unique_ptr<SemanticIDList>(sem1)
        );
        joinPoint->setInArrow(std::move(arrow));
    }
    joinPoint->setPlaceToRight();
    DrawObjectPoint* joinPtr = joinPoint.get();
    list->add(std::move(joinPoint));

    // Down point below left fork
    int downY = leftY + height1 + VerticalSpace;
    auto downPoint = std::make_unique<DrawObjectPoint>();
    downPoint->setInArrow(std::make_unique<Arrow>(cwNONE, leftPtr));
    downPoint->setPosition(leftX, downY);
    DrawObjectPoint* downPtr = downPoint.get();
    list->add(std::move(downPoint));

    // Draw second (bottom) branch
    int height2 = 0;
    SemanticIDList* sem2 = nullptr;
    DrawObject* secondEnd = m_second->drawObjectsToRight(list, sem2, downPtr, ward, height2);

    // Second end point receives second branch
    auto secondPoint = std::make_unique<DrawObjectPoint>();
    if (sem2 == nullptr) {
        secondPoint->setInArrow(std::make_unique<Arrow>(cwNONE, secondEnd));
    } else {
        auto arrow = std::make_unique<SemanticArrow>(
            cwNONE, secondEnd, std::unique_ptr<SemanticIDList>(sem2)
        );
        secondPoint->setInArrow(std::move(arrow));
    }
    DrawObjectPoint* secondPtr = secondPoint.get();
    list->add(std::move(secondPoint));

    // Align both endpoints to maxX
    int maxX = std::max(firstEnd->endX(), secondEnd->endX()) + MinArrowLength;
    joinPtr->setPosition(maxX, leftY);
    secondPtr->setPosition(maxX, downY);

    // Connect joinPoint to secondPoint via secondInArrow (arrow going "up" to merge)
    int curWardUp = (ward == cwFORWARD) ? cwFORWARD : cwNONE;
    joinPtr->setSecondInArrow(std::make_unique<Arrow>(curWardUp, secondPtr));

    height = height1 + VerticalSpace + std::max(height2, ElementHalfHeight);

    return joinPtr;
}

// REIteration::drawObjectsToRight
DrawObject* REIteration::drawObjectsToRight(
    DrawObjectList* list,
    SemanticIDList*& semantics,
    DrawObject* fromDO,
    int ward,
    int& height
) const {
    int curWard = (ward == cwBACKWARD && fromDO->needSpike()) ?
        cwBACKWARD : cwNONE;
    
    auto leftPoint = std::make_unique<DrawObjectPoint>();
    if (semantics == nullptr) {
        leftPoint->setInArrow(std::make_unique<Arrow>(curWard, fromDO));
    } else {
        auto arrow = std::make_unique<SemanticArrow>(
            curWard, fromDO, std::unique_ptr<SemanticIDList>(semantics)
        );
        semantics = nullptr;
        leftPoint->setInArrow(std::move(arrow));
    }
    leftPoint->setPlaceToRight();
    
    DrawObjectPoint* leftPtr = leftPoint.get();
    const int leftX = leftPtr->x();
    const int leftY = leftPtr->y();
    list->add(std::move(leftPoint));
    
    int height1 = 0;
    SemanticIDList* sem1 = nullptr;
    DrawObject* firstLast = m_first->drawObjectsToRight(
        list, sem1, leftPtr, ward, height1
    );
    
    int downY = leftY + VerticalSpace + std::max(height1, ElementHalfHeight);
    
    int curWardDown = (ward == cwFORWARD) ? cwBACKWARD : cwNONE;
    auto downPoint = std::make_unique<DrawObjectPoint>();
    downPoint->setInArrow(std::make_unique<Arrow>(curWardDown, leftPtr));
    downPoint->setPosition(leftX, downY);
    
    DrawObjectPoint* downPtr = downPoint.get();
    list->add(std::move(downPoint));
    
    int oppositeWard = -ward;
    int height2 = 0;
    SemanticIDList* sem2 = nullptr;
    DrawObject* secondLast = m_second->drawObjectsToRight(
        list, sem2, downPtr, oppositeWard, height2
    );
    
    auto secondEndPoint = std::make_unique<DrawObjectPoint>();
    secondEndPoint->setPosition(secondLast->endX() + MinArrowLength, downY);
    if (sem2 == nullptr) {
        secondEndPoint->setInArrow(std::make_unique<Arrow>(cwNONE, secondLast));
    } else {
        auto arrow = std::make_unique<SemanticArrow>(
            cwNONE, secondLast, std::unique_ptr<SemanticIDList>(sem2)
        );
        secondEndPoint->setInArrow(std::move(arrow));
    }
    
    DrawObjectPoint* secondEndPtr = secondEndPoint.get();
    list->add(std::move(secondEndPoint));
    
    int maxX = (firstLast->endX() > secondEndPtr->x()) ?
        firstLast->endX() : secondEndPtr->x();
    
    auto rightPoint = std::make_unique<DrawObjectPoint>();
    rightPoint->setPosition(maxX, leftY);
    
    int curWard1 = (ward == cwBACKWARD && firstLast->needSpike()) ? 
        cwBACKWARD : cwNONE;
    if (sem1 == nullptr) {
        rightPoint->setInArrow(std::make_unique<Arrow>(curWard1, firstLast));
    } else {
        auto arrow = std::make_unique<SemanticArrow>(
            curWard1, firstLast, std::unique_ptr<SemanticIDList>(sem1)
        );
        rightPoint->setInArrow(std::move(arrow));
    }
    
    int curWardUp = (ward == cwFORWARD) ? cwFORWARD : cwNONE;
    rightPoint->setSecondInArrow(std::make_unique<Arrow>(curWardUp, secondEndPtr));
    
    DrawObjectPoint* rightPtr = rightPoint.get();
    list->add(std::move(rightPoint));
    
    int cy = height1 + VerticalSpace;
    if (height2 > 0) {
        cy += height2;
    } else {
        cy += ElementHalfHeight;
    }
    height = (height1 > cy) ? height1 : cy;
    
    return rightPtr;
}

// RENonTerminal::drawObjectsToRight
DrawObject* RENonTerminal::drawObjectsToRight(
    DrawObjectList* list,
    SemanticIDList*& semantics,
    DrawObject* fromDO,
    int ward,
    int& height
) const {
    if (m_isOpen) {
        RETree* root = getRoot();
        if (root) {
            return root->drawObjectsToRight(list, semantics, fromDO, ward, height);
        }
    }
    
    height = ElementHalfHeight;

    auto nonTerminal = std::make_unique<DrawObjectNonTerminal>(m_grammar, m_id);
    nonTerminal->setInArrow(makeArrowForwardAlways(fromDO, ward, semantics));
    nonTerminal->setPlaceToRight();
    
    DrawObject* result = nonTerminal.get();
    list->add(std::move(nonTerminal));
    m_drawObj = list->count() - 1;

    return result;
}

}