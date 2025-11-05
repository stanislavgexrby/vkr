#pragma once
#include <syngt/regex/RETree.h>
#include <syngt/parser/CharProducer.h>
#include <memory>
#include <string>

namespace syngt {

class Grammar;

void skipSpaces(CharProducer* producer);
std::string readIdentifier(CharProducer* producer);
void skipToChar(CharProducer* producer, char ch);
bool isLetterOrDigit(char ch);

/**
 * @brief Упрощенный парсер грамматик (для импорта из других форматов)
 */
class Parser2 {
private:
    CharProducer* m_producer = nullptr;
    Grammar* m_grammar = nullptr;
    
    std::unique_ptr<RETree> parseE();
    std::unique_ptr<RETree> parseT();
    std::unique_ptr<RETree> parseF();
    std::unique_ptr<RETree> parseU();
    
    std::string readName(char lastChar);
    
public:
    Parser2() = default;
    ~Parser2() = default;
    
    std::unique_ptr<RETree> parse(const std::string& text, Grammar* grammar);
    std::unique_ptr<RETree> parseFromProducer(CharProducer* producer, Grammar* grammar);
};

}