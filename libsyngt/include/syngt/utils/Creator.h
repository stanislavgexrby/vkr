#pragma once
#include <memory>

namespace syngt {

class RETree;
class Grammar;

namespace graphics {
    class DrawObjectList;
}

/**
 * @brief Утилиты для создания графических объектов
 * 
 * Преобразует дерево регулярного выражения (RETree) 
 * в список графических объектов (DrawObjectList) для отображения.
 */
namespace Creator {

/**
 * @brief Константа вертикального отступа между элементами
 */
constexpr int VerticalSpace = 30 + 7;  // NS_Radius + 7

/**
 * @brief Создать графические объекты из дерева RE
 * 
 * Преобразует дерево регулярного выражения в список графических объектов:
 * 1. Добавляет DrawObjectFirst (треугольник начала)
 * 2. Вызывает RETree::drawObjectsToRight() для построения диаграммы
 * 3. Добавляет DrawObjectLast (треугольник конца)
 * 4. Устанавливает размеры списка (width, height)
 * 
 * @param list Список для заполнения (будет очищен)
 * @param tree Дерево регулярного выражения
 * @param grammar Грамматика (для контекста)
 */
void createDrawObjects(
    graphics::DrawObjectList* list,
    const RETree* tree,
    Grammar* grammar
);

} // namespace Creator
} // namespace syngt