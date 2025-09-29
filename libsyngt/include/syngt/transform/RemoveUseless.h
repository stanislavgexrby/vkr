#pragma once
#include <vector>
#include <string>
#include <set>

namespace syngt {

class Grammar;
class NTListItem;

/**
 * @brief Удаление бесполезных символов из грамматики
 * 
 * Удаляет:
 * 1. Непродуктивные символы - из которых нельзя вывести терминальную строку
 * 2. Недостижимые символы - до которых нельзя дойти из стартового
 */
class RemoveUseless {
public:
    /**
     * @brief Удалить все бесполезные символы
     */
    static void remove(Grammar* grammar);
};

}