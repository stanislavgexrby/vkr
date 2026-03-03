#pragma once

namespace syngt {

class Grammar;
class NTListItem;

/**
 * @brief Устранение правой рекурсии в грамматике
 *
 * Трансформирует правила вида:
 *   A → α A | β
 * В правила (теорема Ардена):
 *   A → β α*
 */
class RightElimination {
public:
    /**
     * @brief Устранить правую рекурсию для одного нетерминала
     */
    static void eliminateForNonTerminal(NTListItem* nt, Grammar* grammar);

    /**
     * @brief Устранить правую рекурсию для всей грамматики
     */
    static void eliminate(Grammar* grammar);

    /**
     * @brief Проверка на прямую правую рекурсию (A → α A)
     */
    static bool hasDirectRightRecursion(NTListItem* nt, Grammar* grammar);
};

}
