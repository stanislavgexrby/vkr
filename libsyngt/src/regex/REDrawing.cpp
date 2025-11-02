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

// REOr::drawObjectsToRight  
DrawObject* REOr::drawObjectsToRight(
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
    
    // Первая ветка идет вправо
    int height1 = 0;
    SemanticIDList* sem1 = nullptr;
    DrawObject* firstLast = m_first->drawObjectsToRight(
        list, sem1, leftPtr, ward, height1
    );
    
    // Создаем правую точку схождения
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
    
    // Вторая ветка идет вниз от левой точки
    int cy = height1 + VerticalSpace;
    
    // Создаем точку для второй ветки (вниз от левой точки)
    auto downPoint = std::make_unique<DrawObjectPoint>();
    downPoint->setInArrow(std::make_unique<Arrow>(cwNONE, leftPtr));
    downPoint->setPosition(leftPtr->x(), leftPtr->y() + cy);
    
    DrawObjectPoint* downPtr = downPoint.get();
    list->add(std::move(downPoint));
    
    // Вторая ветка от точки вниз идет вправо
    int height2 = 0;
    SemanticIDList* sem2 = nullptr;
    DrawObject* secondLast = m_second->drawObjectsToRight(
        list, sem2, downPtr, ward, height2
    );
    
    // Подключаем конец второй ветки к правой точке (вторая входящая стрелка)
    int curWard2 = (ward == cwFORWARD) ? cwFORWARD : cwNONE;
    if (sem2 == nullptr) {
        rightPtr->setSecondInArrow(std::make_unique<Arrow>(curWard2, secondLast));
    } else {
        auto arrow = std::make_unique<SemanticArrow>(
            curWard2, secondLast, std::unique_ptr<SemanticIDList>(sem2)
        );
        rightPtr->setSecondInArrow(std::move(arrow));
    }
    
    // Выравниваем X координаты
    if (secondLast->endX() > rightPtr->x()) {
        rightPtr->setXCoord(secondLast->endX());
    }
    
    // Вычисляем высоту
    if (height2 > 0) {
        cy += height2;
    } else {
        cy += NS_Radius;
    }
    
    height = (height1 > cy) ? height1 : cy;
    
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