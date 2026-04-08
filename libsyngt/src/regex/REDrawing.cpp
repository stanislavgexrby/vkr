#include <syngt/regex/RETree.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/REMacro.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REIteration.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/graphics/DrawObject.h>
#include <syngt/graphics/Arrow.h>
#include <syngt/graphics/Ward.h>
#include <syngt/graphics/GraphicsConstants.h>
#include <syngt/utils/Semantic.h>
#include <syngt/core/Grammar.h>
#include <functional>
#include <vector>

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

// RESemantic::drawObjectsToRight
// Accumulates semantic ID into the shared list; does not create a DrawObject.
// The next real element (terminal/non-terminal) will consume the list
// and attach it as a SemanticArrow.
DrawObject* RESemantic::drawObjectsToRight(
    DrawObjectList* list,
    SemanticIDList*& semantics,
    DrawObject* fromDO,
    int ward,
    int& height
) const {
    (void)list;
    (void)ward;
    height = 0;
    if (semantics == nullptr) {
        semantics = new SemanticIDList();
    }
    semantics->add(m_id);
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
    // Flatten the (left-associative) Or tree into a list of alternatives,
    // then draw all from a single fork point — no extra fork per Or node.
    std::vector<const RETree*> alternatives;
    std::function<void(const RETree*)> flatten = [&](const RETree* node) {
        if (const auto* orNode = dynamic_cast<const REOr*>(node)) {
            flatten(orNode->firstOperand());
            flatten(orNode->secondOperand());
        } else {
            alternatives.push_back(node);
        }
    };
    flatten(this);

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

    // Draw each alternative, tracking Y, heights, ends, and pending semantics
    std::vector<int> branchYs;
    std::vector<int> branchHeights;
    std::vector<DrawObject*> branchEnds;
    std::vector<SemanticIDList*> branchSems;

    int currentY = leftY;
    for (size_t i = 0; i < alternatives.size(); i++) {
        branchYs.push_back(currentY);

        DrawObject* branchFrom;
        if (i == 0) {
            branchFrom = leftPtr;
        } else {
            auto downPoint = std::make_unique<DrawObjectPoint>();
            downPoint->setInArrow(std::make_unique<Arrow>(cwNONE, leftPtr));
            downPoint->setPosition(leftX, currentY);
            branchFrom = downPoint.get();
            list->add(std::move(downPoint));
        }

        int h = 0;
        SemanticIDList* sem = nullptr;
        DrawObject* branchEnd = alternatives[i]->drawObjectsToRight(list, sem, branchFrom, ward, h);
        branchEnds.push_back(branchEnd);
        branchHeights.push_back(h);
        branchSems.push_back(sem);

        if (i + 1 < alternatives.size()) {
            currentY += h + VerticalSpace;
        }
    }

    // Compute right edge across all branches, accounting for semantic arrow lengths
    int maxX = 0;
    for (size_t i = 0; i < branchEnds.size(); i++) {
        int arrLen = MinArrowLength;
        if (branchSems[i]) arrLen += branchSems[i]->getLength();
        if (i == 0 && ward == cwBACKWARD && branchEnds[i]->needSpike()) arrLen += SpikeLength;
        int ex = branchEnds[i]->endX() + arrLen;
        if (ex > maxX) maxX = ex;
    }

    // Build join points bottom-to-top so each higher join gets secondInArrow
    // from the one below (the merge arrow that goes upward).
    int curWardUp = (ward == cwFORWARD) ? cwFORWARD : cwNONE;
    DrawObjectPoint* lowerJoin = nullptr;
    DrawObjectPoint* topJoin = nullptr;

    for (int i = (int)alternatives.size() - 1; i >= 0; i--) {
        auto joinPoint = std::make_unique<DrawObjectPoint>();
        joinPoint->setPosition(maxX, branchYs[i]);

        int curWardBranch = (i == 0 && ward == cwBACKWARD && branchEnds[i]->needSpike()) ?
            cwBACKWARD : cwNONE;
        if (branchSems[i] == nullptr) {
            joinPoint->setInArrow(std::make_unique<Arrow>(curWardBranch, branchEnds[i]));
        } else {
            auto arrow = std::make_unique<SemanticArrow>(
                curWardBranch, branchEnds[i],
                std::unique_ptr<SemanticIDList>(branchSems[i])
            );
            joinPoint->setInArrow(std::move(arrow));
        }

        if (lowerJoin != nullptr) {
            joinPoint->setSecondInArrow(std::make_unique<Arrow>(curWardUp, lowerJoin));
        }

        DrawObjectPoint* joinPtr = joinPoint.get();
        list->add(std::move(joinPoint));
        lowerJoin = joinPtr;
        if (i == 0) topJoin = joinPtr;
    }

    // Height = distance from top row to bottom of last branch
    height = (branchYs.back() - leftY) + std::max(branchHeights.back(), ElementHalfHeight);

    return topJoin;
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
    
    // Pascal logic: @* (epsilon first → firstLast==leftPtr): cwFORWARD only when ward=cwBACKWARD
    //               @+ (body first → firstLast!=leftPtr): cwNONE always
    int curWardUp;
    if (firstLast == leftPtr) {
        curWardUp = (ward == cwBACKWARD) ? cwFORWARD : cwNONE;
    } else {
        curWardUp = cwNONE;
    }
    rightPoint->setSecondInArrow(std::make_unique<Arrow>(curWardUp, secondEndPtr));
    
    DrawObjectPoint* rightPtr = rightPoint.get();
    list->add(std::move(rightPoint));

    // Pascal DrawObjectsToDown alignment: SecondLastDO.x := result.x when smaller
    if (secondEndPtr->x() < rightPtr->x()) {
        secondEndPtr->setPosition(rightPtr->x(), downY);
    }

    int cy = std::max(height1, ElementHalfHeight) + VerticalSpace;
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

    NTListItem* ntItem = getListItem();
    std::unique_ptr<DrawObjectLeaf> leafObj;
    if (ntItem && ntItem->isMacro()) {
        leafObj = std::make_unique<DrawObjectMacro>(m_grammar, m_id);
    } else {
        leafObj = std::make_unique<DrawObjectNonTerminal>(m_grammar, m_id);
    }
    leafObj->setInArrow(makeArrowForwardAlways(fromDO, ward, semantics));
    leafObj->setPlaceToRight();

    DrawObject* result = leafObj.get();
    list->add(std::move(leafObj));
    m_drawObj = list->count() - 1;

    return result;
}

}