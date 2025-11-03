#pragma once
#include <syngt/regex/RETree.h>
#include <syngt/parser/CharProducer.h>
#include <memory>
#include <string>

namespace syngt {

class Grammar;

/**
 * @brief Упрощенный парсер грамматик (для импорта из других форматов)
 * 
 * Отличия от Parser:
 * - Использует '*' для итерации вместо '#'
 * - Не поддерживает макросы
 * - Используется для ImportFromGEdit()
 * 
 * Грамматика парсера:
 * RE_FromString = E '.'
 * E = T [';' T]*          (альтернативы)
 * T = F [',' F]*          (последовательности)
 * F = U ['*' U]*          (итерации)
 * U = $Semantic | Term | NonTerm | '(' E ')' | '[' E ']'
 * Term = ''' char* ''' | '"' char* '"'
 */
class Parser2 {
private:
    CharProducer* m_producer = nullptr;
    Grammar* m_grammar = nullptr;
    
    std::unique_ptr<RETree> parseE();    // Expression (альтернативы)
    std::unique_ptr<RETree> parseT();    // Term (последовательности)
    std::unique_ptr<RETree> parseF();    // Factor (итерации с *)
    std::unique_ptr<RETree> parseU();    // Unary (терминалы, нетерминалы)
    
    void skipSpaces();
    void skipNotMatter();
    void skipToChar(char ch);
    std::string readIdentifier();
    std::string readName(char lastChar);
    bool isLetterOrDigit(char ch) const;
    
public:
    Parser2() = default;
    ~Parser2() = default;
    
    /**
     * @brief Распарсить регулярное выражение из строки
     * @param text Текст правила
     * @param grammar Грамматика для контекста
     * @return Дерево RE
     */
    std::unique_ptr<RETree> parse(const std::string& text, Grammar* grammar);
    
    /**
     * @brief Распарсить из CharProducer
     */
    std::unique_ptr<RETree> parseFromProducer(CharProducer* producer, Grammar* grammar);
};

}