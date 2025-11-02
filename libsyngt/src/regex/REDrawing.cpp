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

#include <functional>

namespace syngt {

using namespace graphics;

// Вспомогательная функция для создания стрелки
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

// RETree::drawObjectsToRight - базовая реализация
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

// RETerminal::drawObjectsToRight
DrawObject* RETerminal::drawObjectsToRight(
    DrawObjectList* list,
    SemanticIDList*& semantics,
    DrawObject* fromDO,
    int ward,
    int& height
) const {
    if (m_id != 0) {
        height = NS_Radius;
    } else {
        height = 0;
    }
    
    auto terminal = std::make_unique<DrawObjectTerminal>(m_grammar, m_id);
    terminal->setInArrow(makeArrowForwardAlways(fromDO, ward, semantics));
    terminal->setPlaceToRight();
    
    DrawObject* result = terminal.get();
    list->add(std::move(terminal));
    
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

DrawObject* REOr::drawObjectsToRight(
    DrawObjectList* list,
    SemanticIDList*& semantics,
    DrawObject* fromDO,
    int ward,
    int& height
) const {
    // ===== Flatten nested ORs =====
    std::function<void(const RETree*, std::vector<const RETree*>&)> flatten;
    flatten = [&](const RETree* node, std::vector<const RETree*>& out) {
        if (!node) return;
        if (auto orNode = dynamic_cast<const REOr*>(node)) {
            flatten(orNode->m_first.get(), out);
            flatten(orNode->m_second.get(), out);
        } else {
            out.push_back(node);
        }
    };

    std::vector<const RETree*> alternatives;
    flatten(this, alternatives);

    // ===== Common left point =====
    int curWard = ((ward == cwBACKWARD) && fromDO->needSpike()) ? cwBACKWARD : cwNONE;

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
    const int baseX = leftPtr->x();
    const int baseY = leftPtr->y();
    list->add(std::move(leftPoint));

    // ===== Draw each alternative branch =====
    std::vector<DrawObject*> branchEnds;
    int maxEndX = baseX;
    int currentY = baseY;
    const int verticalSpacing = 60;

    for (size_t i = 0; i < alternatives.size(); ++i) {
        DrawObjectPoint* branchStart = nullptr;

        if (i == 0) {
            branchStart = leftPtr;
        } else {
            auto branchJoin = std::make_unique<DrawObjectPoint>();
            branchJoin->setPosition(baseX, currentY);
            branchJoin->setInArrow(std::make_unique<Arrow>(cwNONE, leftPtr));
            branchStart = branchJoin.get();
            list->add(std::move(branchJoin));
        }

        int branchHeight = 0;
        SemanticIDList* sem = nullptr;

        DrawObject* branchEnd = alternatives[i]->drawObjectsToRight(
            list, sem, branchStart, ward, branchHeight
        );

        branchEnds.push_back(branchEnd);

        if (branchEnd->endX() > maxEndX)
            maxEndX = branchEnd->endX();

        currentY += std::max(branchHeight, NS_Radius) + verticalSpacing;
    }

    // ===== Create single right junction point =====
    auto rightPoint = std::make_unique<DrawObjectPoint>();
    rightPoint->setPosition(maxEndX + 40, baseY);
    DrawObjectPoint* rightPtr = rightPoint.get();
    list->add(std::move(rightPoint));

    // ===== Connect all branches to the single right point =====
    for (auto* branchEnd : branchEnds) {
        // todo 
        // rightPtr->addInArrow(std::make_unique<Arrow>(cwNONE, branchEnd));
    }

    // ===== Height and return =====
    height = (currentY - baseY) - verticalSpacing; // subtract last spacing
    return rightPtr;
}

// REIteration::drawObjectsToRight
DrawObject* REIteration::drawObjectsToRight(
    DrawObjectList* list,
    SemanticIDList*& semantics,
    DrawObject* fromDO,
    int ward,
    int& height
) const {
    // Создаем левую точку разветвления
    int curWard = (ward == cwBACKWARD && fromDO->needSpike()) ? cwBACKWARD : cwNONE;
    
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
    list->add(std::move(leftPoint));
    
    // Первый операнд (повторяемая часть A) идет вправо
    int height1 = 0;
    SemanticIDList* sem1 = nullptr;
    DrawObject* firstLast = m_first->drawObjectsToRight(
        list, sem1, leftPtr, ward, height1
    );
    
    // Создаем правую точку после первого операнда
    int curWard1 = (ward == cwBACKWARD && firstLast->needSpike()) ? cwBACKWARD : cwNONE;
    auto rightPoint = std::make_unique<DrawObjectPoint>();
    if (sem1 == nullptr) {
        rightPoint->setInArrow(std::make_unique<Arrow>(curWard1, firstLast));
    } else {
        auto arrow = std::make_unique<SemanticArrow>(
            curWard1, firstLast, std::unique_ptr<SemanticIDList>(sem1)
        );
        rightPoint->setInArrow(std::move(arrow));
    }
    rightPoint->setPlaceToRight();
    
    DrawObjectPoint* rightPtr = rightPoint.get();
    list->add(std::move(rightPoint));
    
    // Второй операнд (B) идет от правой точки вправо, затем назад к левой точке
    int height2 = 0;
    SemanticIDList* sem2 = nullptr;
    DrawObject* secondLast = m_second->drawObjectsToRight(
        list, sem2, rightPtr, cwBACKWARD, height2
    );
    
    // Создаем обратную стрелку от конца второго операнда к левой точке (петля!)
    auto backArrow = std::make_unique<Arrow>(cwBACKWARD, secondLast);
    leftPtr->setSecondInArrow(std::move(backArrow));
    
    // Вычисляем высоту
    int cy = height1 + VerticalSpace;
    if (height2 > 0) {
        cy += height2;
    } else {
        cy += NS_Radius;
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
    // Если нетерминал "открыт", рекурсивно обрабатываем его корень
    if (m_isOpen) {
        RETree* root = getRoot();
        if (root) {
            return root->drawObjectsToRight(list, semantics, fromDO, ward, height);
        }
    }
    
    // Иначе создаем графический объект нетерминала
    height = NS_Radius;
    
    auto nonTerminal = std::make_unique<DrawObjectNonTerminal>(m_grammar, m_id);
    nonTerminal->setInArrow(makeArrowForwardAlways(fromDO, ward, semantics));
    nonTerminal->setPlaceToRight();
    
    DrawObject* result = nonTerminal.get();
    list->add(std::move(nonTerminal));
    
    return result;
}

} // namespace syngt