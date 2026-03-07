#pragma once
#include <memory>

namespace syngt {

class Grammar;
class NTListItem;
class RETree;

/**
 * @brief Устранение левой рекурсии в грамматике
 *
 * Реализует алгоритм из Pascal (TransGrammar.leftEl / TransRE_*.leftEl):
 * Для каждого NT вычисляет TTransformation{R1, R2, E} и строит
 *   A = R2 * R1*    (если E=false или R2=nil)
 *   A = (R2 * R1*) | ε   (если E=true и R2 != nil)
 *
 * где R1 — часть после рекурсивного вхождения,
 *     R2 — нерекурсивная часть,
 *     E  — может ли выражение порождать пустое слово.
 */
class LeftElimination {
public:
    static void eliminateForNonTerminal(NTListItem* nt, Grammar* grammar);
    static void eliminate(Grammar* grammar);
    static bool hasDirectLeftRecursion(NTListItem* nt);

private:
    // Используется в hasDirectLeftRecursion для быстрой проверки
    static bool isLeftRecursive(const RETree* node, const NTListItem* nt);
};

} // namespace syngt
