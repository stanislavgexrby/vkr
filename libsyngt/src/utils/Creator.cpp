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

// Вспомогательный класс для доступа к protected методам
// (временное решение до рефакторинга DrawObject)
class DrawObjectLastHelper : public DrawObjectLast {
public:
    void setPositionPublic(int x, int y) {
        setPlace(x, y);
    }
};

void createDrawObjects(
    graphics::DrawObjectList* list,
    const RETree* tree,
    Grammar*
) {
    if (!list || !tree) {
        return;
    }
    
    // Очистить список
    list->clear();
    
    // Создать первый объект (начало диаграммы)
    auto firstDO = std::make_unique<DrawObjectFirst>();
    firstDO->place();  // Разместить в начальной позиции
    
    // Сохраняем указатель на первый объект до перемещения
    DrawObjectFirst* firstPtr = firstDO.get();
    list->add(std::move(firstDO));
    
    // Семантики (начальное значение nullptr)
    SemanticIDList* semantics = nullptr;
    int height = 0;
    
    // Построить диаграмму вправо
    // TODO: Реализовать RETree::drawObjectsToRight()
    // Пока просто установим prevDO = firstDO
    DrawObject* prevDO = firstPtr;
    
    // Создать последний объект (конец диаграммы)
    auto lastDO = std::make_unique<DrawObjectLastHelper>();
    
    // Создать входящую стрелку
    std::unique_ptr<Arrow> inArrow;
    if (semantics == nullptr) {
        inArrow = std::make_unique<Arrow>(cwFORWARD, prevDO);
    } else {
        // SemanticArrow с семантиками
        auto semList = std::make_unique<SemanticIDList>(*semantics);
        inArrow = std::make_unique<SemanticArrow>(cwFORWARD, prevDO, std::move(semList));
    }
    
    // Установить входящую стрелку
    lastDO->setInArrow(std::move(inArrow));
    
    // Установить позицию последнего объекта через helper
    int lastX = prevDO->endX() + lastDO->inArrow()->getLength();
    int lastY = prevDO->y();
    lastDO->setPositionPublic(lastX, lastY);
    
    // Сохраняем указатель перед перемещением
    DrawObjectLast* lastPtr = lastDO.get();
    list->add(std::move(lastDO));
    
    // Установить размеры списка
    list->setWidth(lastPtr->endX() + HorizontalSkipFromBorder);
    list->setHeight(height + firstPtr->y() + VerticalSkipFromBorder + NS_Radius);
}

} // namespace Creator
} // namespace syngt