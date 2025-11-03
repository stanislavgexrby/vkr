#pragma once
#include <syngt/regex/RETree.h>
#include <syngt/parser/CharProducer.h>
#include <memory>
#include <string>

namespace syngt {

class Grammar;

/**
 * @brief Парсер грамматик в формате SynGT
 * 
 * Грамматика парсера (рекурсивный descent):
 * RE_FromString = E '.'
 * E = T [';' T]*          (альтернативы)
 * T = F [',' F]*          (последовательности)
 * F = U ['#' U]*          (специальные операторы)
 * U = K ['*' ; '+' ]*     (итерации)
 * K = Term | NonTerm | Semantic | Macro | '(' E ')' | '[' E ']'
 */
class Parser {
private:
    CharProducer* m_producer = nullptr;
    Grammar* m_grammar = nullptr;
    
    std::unique_ptr<RETree> parseE();    // Expression (альтернативы)
    std::unique_ptr<RETree> parseT();    // Term (последовательности)
    std::unique_ptr<RETree> parseF();    // Factor (с #)
    std::unique_ptr<RETree> parseU();    // Unary (с * +)
    std::unique_ptr<RETree> parseK();    // Constant (терминалы, нетерминалы, скобки)
    
    void skipSpaces();
    void skipNotMatter();
    void skipToChar(char ch);
    std::string readIdentifier();
    std::string readName(char lastChar);
    bool isLetterOrDigit(char ch) const;
    
public:
    Parser() = default;
    ~Parser() = default;
    
    /**
     * @brief Распарсить регулярное выражение из строки
     * @param text Текст правила (например: "begin , statement , end.")
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