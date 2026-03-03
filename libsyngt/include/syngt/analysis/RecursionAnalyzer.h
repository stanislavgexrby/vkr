#pragma once

#include <string>
#include <vector>

namespace syngt {

class Grammar;

struct RecursionResult {
    std::string name;
    std::string leftRecursion;   // "direct", "indirect", or ""
    std::string anyRecursion;    // "direct", "indirect", or ""
    std::string rightRecursion;  // "direct", "indirect", or ""
};

/**
 * @brief Анализатор рекурсий в грамматике
 *
 * Порт алгоритма TAnalyzeForm из оригинального Pascal-инструмента:
 * для каждого нетерминала определяет тип рекурсии (прямая/косвенная)
 * и её направление (левая/центральная/правая).
 */
class RecursionAnalyzer {
public:
    static std::vector<RecursionResult> analyze(const Grammar* grammar);
};

} // namespace syngt
