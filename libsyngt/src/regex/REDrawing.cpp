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

// В RETerminal::drawObjectsToRight
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
    
    if (m_id == 0) {
        semantics = nullptr;
        return fromDO;
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

    // ===== Левая точка разветвления =====
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
    const int baseX = leftPtr->x();
    const int baseY = leftPtr->y();
    list->add(std::move(leftPoint));

    // ===== Рисуем каждую альтернативную ветвь =====
    std::vector<DrawObject*> branchEnds;
    const int verticalSpacing = 60;

    for (size_t i = 0; i < alternatives.size(); ++i) {
        DrawObjectPoint* branchStart = nullptr;
        int branchY = baseY + static_cast<int>(i) * verticalSpacing;

        if (i == 0) {
            // Первая альтернатива идет напрямую от левой точки
            branchStart = leftPtr;
        } else {
            // Остальные альтернативы - создаем новые точки на своем уровне Y
            auto branchJoin = std::make_unique<DrawObjectPoint>();
            branchJoin->setPosition(baseX, branchY);
            branchJoin->setInArrow(std::make_unique<Arrow>(cwNONE, leftPtr));
            branchStart = branchJoin.get();
            list->add(std::move(branchJoin));
        }

        int branchHeight = 0;
        SemanticIDList* sem = nullptr;

        // Рисуем содержимое ветви
        DrawObject* branchEnd = alternatives[i]->drawObjectsToRight(
            list, sem, branchStart, ward, branchHeight
        );

        branchEnds.push_back(branchEnd);
    }

    // ===== Находим максимальный X среди всех ветвей =====
    int maxEndX = baseX;
    for (auto* branchEnd : branchEnds) {
        if (branchEnd->endX() > maxEndX)
            maxEndX = branchEnd->endX();
    }

    // ===== Выравниваем все ветви по максимальному X =====
    for (auto* branchEnd : branchEnds) {
        if (branchEnd->endX() < maxEndX) {
            int shift = maxEndX - branchEnd->endX();
            branchEnd->move(shift, 0);
        }
    }

    // ===== Создаем правую точку соединения =====
    DrawObject* finalPoint = nullptr;
    
    if (branchEnds.size() == 1) {
        // Одна альтернатива
        finalPoint = branchEnds[0];
        height = NS_Radius;
    } else {
        // 2+ альтернативы - создаем каскад точек снизу вверх
        // Начинаем с последней ветви и идем к первой
        DrawObjectPoint* currentPoint = nullptr;
        
        // Создаем точки соединения снизу вверх
        for (int i = static_cast<int>(branchEnds.size()) - 1; i >= 0; --i) {
            if (i == static_cast<int>(branchEnds.size()) - 1) {
                // Самая нижняя ветвь - просто запоминаем её конец
                currentPoint = dynamic_cast<DrawObjectPoint*>(branchEnds[i]);
                if (!currentPoint) {
                    // Если это не точка, создаем промежуточную точку
                    auto tempPoint = std::make_unique<DrawObjectPoint>();
                    tempPoint->setPosition(maxEndX + SpaceLength, branchEnds[i]->y());
                    tempPoint->setInArrow(std::make_unique<Arrow>(cwNONE, branchEnds[i]));
                    currentPoint = tempPoint.get();
                    list->add(std::move(tempPoint));
                }
            } else {
                // Создаем точку соединения на уровне текущей ветви
                auto joinPoint = std::make_unique<DrawObjectPoint>();
                joinPoint->setPosition(maxEndX + SpaceLength, branchEnds[i]->y());
                joinPoint->setInArrow(std::make_unique<Arrow>(cwNONE, branchEnds[i]));
                joinPoint->setSecondInArrow(std::make_unique<Arrow>(cwNONE, currentPoint));
                
                currentPoint = joinPoint.get();
                list->add(std::move(joinPoint));
            }
        }
        
        finalPoint = currentPoint;
        height = static_cast<int>(alternatives.size() - 1) * verticalSpacing + NS_Radius;
    }

    return finalPoint;
}

DrawObject* REIteration::drawObjectsToRight(
    DrawObjectList* list,
    SemanticIDList*& semantics,
    DrawObject* fromDO,
    int ward,
    int& height
) const {
    // ===== Левая точка с отступом =====
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
    
    // ===== Создаем промежуточную точку ПОСЛЕ leftPoint для первого операнда =====
    // Это создает отступ между leftPoint и первым элементом
    auto startPoint = std::make_unique<DrawObjectPoint>();
    startPoint->setPosition(leftX + MinArrowLength, leftY);
    startPoint->setInArrow(std::make_unique<Arrow>(cwNONE, leftPtr));
    
    DrawObjectPoint* startPtr = startPoint.get();
    list->add(std::move(startPoint));
    
    // ===== Первый операнд от startPoint =====
    int height1 = 0;
    SemanticIDList* sem1 = nullptr;
    DrawObject* firstLast = m_first->drawObjectsToRight(
        list, sem1, startPtr, ward, height1
    );
    
    // ===== Y для нижней ветки =====
    int downY = leftY + VerticalSpace + std::max(height1, NS_Radius);
    
    // ===== Точка вниз =====
    auto downPoint = std::make_unique<DrawObjectPoint>();
    downPoint->setInArrow(std::make_unique<Arrow>(cwNONE, leftPtr));
    downPoint->setPosition(leftX, downY);
    
    DrawObjectPoint* downPtr = downPoint.get();
    list->add(std::move(downPoint));
    
    // ===== Промежуточная точка для второго операнда =====
    auto startPoint2 = std::make_unique<DrawObjectPoint>();
    startPoint2->setPosition(leftX + MinArrowLength, downY);
    startPoint2->setInArrow(std::make_unique<Arrow>(cwNONE, downPtr));
    
    DrawObjectPoint* startPtr2 = startPoint2.get();
    list->add(std::move(startPoint2));
    
    // ===== Второй операнд =====
    int oppositeWard = -ward;
    int height2 = 0;
    SemanticIDList* sem2 = nullptr;
    DrawObject* secondLast = m_second->drawObjectsToRight(
        list, sem2, startPtr2, oppositeWard, height2
    );
    
    // ===== Находим максимальный X =====
    int maxX = (firstLast->endX() > secondLast->endX()) ? 
               firstLast->endX() : secondLast->endX();
    
    // ===== Выравниваем =====
    if (firstLast->endX() < maxX) {
        int shift = maxX - firstLast->endX();
        firstLast->move(shift, 0);
    }
    if (secondLast->endX() < maxX) {
        int shift = maxX - secondLast->endX();
        secondLast->move(shift, 0);
    }
    
    // ===== Промежуточная точка перед rightPoint =====
    // Создаем точку на maxX для схождения веток
    auto preRightPoint = std::make_unique<DrawObjectPoint>();
    preRightPoint->setPosition(maxX + MinArrowLength, leftY);
    
    // Первая стрелка от верхней ветки
    int curWard1 = (ward == cwBACKWARD && firstLast->needSpike()) ? 
        cwBACKWARD : cwNONE;
    if (sem1 == nullptr) {
        preRightPoint->setInArrow(std::make_unique<Arrow>(curWard1, firstLast));
    } else {
        auto arrow = std::make_unique<SemanticArrow>(
            curWard1, firstLast, std::unique_ptr<SemanticIDList>(sem1)
        );
        preRightPoint->setInArrow(std::move(arrow));
    }
    
    // Вторая стрелка от нижней ветки
    if (sem2 == nullptr) {
        preRightPoint->setSecondInArrow(std::make_unique<Arrow>(cwNONE, secondLast));
    } else {
        auto arrow = std::make_unique<SemanticArrow>(
            cwNONE, secondLast, std::unique_ptr<SemanticIDList>(sem2)
        );
        preRightPoint->setSecondInArrow(std::move(arrow));
    }
    
    DrawObjectPoint* preRightPtr = preRightPoint.get();
    list->add(std::move(preRightPoint));
    
    // ===== Правая точка с отступом =====
    auto rightPoint = std::make_unique<DrawObjectPoint>();
    rightPoint->setPosition(maxX + MinArrowLength * 2, leftY);
    rightPoint->setInArrow(std::make_unique<Arrow>(cwNONE, preRightPtr));
    
    DrawObjectPoint* rightPtr = rightPoint.get();
    list->add(std::move(rightPoint));
    
    // ===== Высота =====
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