#pragma once

namespace syngt {

class Grammar;

/**
 * @brief Полная регуляризация грамматики
 *
 * Реализует одновременное устранение левой и правой рекурсии
 * по алгоритму TGrammar.regularize() из оригинального Pascal-инструмента.
 *
 * Для каждого нетерминала N применяется формула (теорема Ардена дважды):
 *   1) leftEl:  T = A·R1 | R2
 *   2) rightEl на R1: R1 = RA1·A | RB1
 *   3) rightEl на R2: R2 = RA2·A | RB2
 *   4) newRoot = ((RA2)* , RB2 , (RB1)*) # RA1
 *
 * Нетерминалы обрабатываются в обратном порядке (как в оригинале).
 */
class Regularize {
public:
    /**
     * @brief Регуляризировать всю грамматику (устранить левую и правую рекурсию)
     */
    static void regularize(Grammar* grammar);
};

} // namespace syngt
