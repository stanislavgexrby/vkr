#include <syngt/parser/Parser2.h>
#include <syngt/core/Grammar.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REIteration.h>
#include <stdexcept>
#include <cctype>

namespace syngt {

std::unique_ptr<RETree> Parser2::parse(const std::string& text, Grammar* grammar) {
    CharProducer producer(text);
    return parseFromProducer(&producer, grammar);
}

std::unique_ptr<RETree> Parser2::parseFromProducer(CharProducer* producer, Grammar* grammar) {
    m_producer = producer;
    m_grammar = grammar;
    
    skipSpaces();
    auto result = parseE();
    
    // Ожидаем точку в конце
    skipSpaces();
    if (m_producer->currentChar() == '.') {
        m_producer->next();
    } else {
        throw std::runtime_error("Expected '.' at end of rule");
    }
    
    return result;
}

// E = T [';' T]*  (альтернативы)
std::unique_ptr<RETree> Parser2::parseE() {
    auto left = parseT();
    
    skipSpaces();
    while (m_producer->currentChar() == ';') {
        m_producer->next();
        skipSpaces();
        
        auto right = parseT();
        left = REOr::make(std::move(left), std::move(right));
        
        skipSpaces();
    }
    
    return left;
}

// T = F [',' F]*  (последовательности)
std::unique_ptr<RETree> Parser2::parseT() {
    auto left = parseF();
    
    skipSpaces();
    while (m_producer->currentChar() == ',') {
        m_producer->next();
        skipSpaces();
        
        auto right = parseF();
        left = REAnd::make(std::move(left), std::move(right));
        
        skipSpaces();
    }
    
    return left;
}

// F = U ['*' U]*  (итерации)
// ВАЖНО: В Parser2 используется '*' а не '#' как в Parser!
std::unique_ptr<RETree> Parser2::parseF() {
    auto left = parseU();
    
    skipSpaces();
    while (m_producer->currentChar() == '*') {
        m_producer->next();
        skipSpaces();
        
        auto right = parseU();
        left = REIteration::make(std::move(left), std::move(right));
        
        skipSpaces();
    }
    
    return left;
}

// U = $Semantic | Term | NonTerm | '(' E ')' | '[' E ']'
std::unique_ptr<RETree> Parser2::parseU() {
    skipSpaces();
    char ch = m_producer->currentChar();
    
    // Скобки: ( E )
    if (ch == '(') {
        m_producer->next();
        auto expr = parseE();
        
        skipSpaces();
        if (m_producer->currentChar() != ')') {
            throw std::runtime_error("Expected ')'");
        }
        m_producer->next();
        skipSpaces();
        
        return expr;
    }
    
    // Опциональность: [ E ] → (epsilon ; E)
    if (ch == '[') {
        m_producer->next();
        auto expr = parseE();
        
        skipSpaces();
        if (m_producer->currentChar() != ']') {
            throw std::runtime_error("Expected ']'");
        }
        m_producer->next();
        skipSpaces();
        
        // [E] = (epsilon ; E)
        auto epsilon = std::make_unique<RETerminal>(m_grammar, 0);
        return REOr::make(std::move(epsilon), std::move(expr));
    }
    
    // Терминалы: 'text' или "text"
    if (ch == '\'' || ch == '"') {
        char quote = ch;
        m_producer->next();
        
        std::string name = readName(quote);
        
        if (!m_producer->next()) {
            throw std::runtime_error(std::string("Expected closing ") + quote);
        }
        
        skipSpaces();
        
        int id = m_grammar->addTerminal(name);
        return std::make_unique<RETerminal>(m_grammar, id);
    }
    
    // Epsilon: @ или &
    if (ch == '@' || ch == '&') {
        m_producer->next();
        skipSpaces();
        return std::make_unique<RETerminal>(m_grammar, 0); // Epsilon = ID 0
    }
    
    // Семантика: $identifier
    if (ch == '$') {
        m_producer->next();
        std::string name = "$" + readIdentifier();
        
        int id = m_grammar->findSemantic(name);
        if (id < 0) {
            id = m_grammar->addSemantic(name);
        }
        
        return std::make_unique<RESemantic>(m_grammar, id);
    }
    
    // Нетерминал: identifier
    if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
        std::string name = readIdentifier();
        
        int id = m_grammar->findNonTerminal(name);
        if (id < 0) {
            id = m_grammar->addNonTerminal(name);
        }
        
        return std::make_unique<RENonTerminal>(m_grammar, id, false);
    }
    
    throw std::runtime_error(std::string("Unexpected character: ") + ch);
}

// Вспомогательные функции (копии из Parser.cpp)
void Parser2::skipNotMatter() {
    char ch = m_producer->currentChar();
    
    while (!m_producer->isEnd() && 
           !std::isalnum(static_cast<unsigned char>(ch)) && 
           ch != '_' && ch != '\'' && ch != '"' && 
           ch != '[' && ch != ']' && ch != '(' && ch != ')' && 
           ch != '{' && ch != '}' && ch != '*' && ch != '+' && 
           ch != ',' && ch != ';' && ch != '#' && ch != '.' && 
           ch != '@' && ch != ':' && ch != '$' && ch != '/' && ch != '&') 
    {
        m_producer->next();
        ch = m_producer->currentChar();
    }
}

void Parser2::skipSpaces() {
    skipNotMatter();
    
    // Пропуск комментариев {comment} или //comment
    while (m_producer->currentChar() == '{' || m_producer->currentChar() == '/') {
        if (m_producer->currentChar() == '{') {
            skipToChar('}');
        } else {
            skipToChar('\n');
        }
        m_producer->next();
        skipNotMatter();
    }
}

void Parser2::skipToChar(char ch) {
    while (m_producer->currentChar() != ch && !m_producer->isEnd()) {
        m_producer->next();
    }
}

std::string Parser2::readIdentifier() {
    skipSpaces();
    std::string name;
    
    char ch = m_producer->currentChar();
    while (isLetterOrDigit(ch) && !m_producer->isEnd()) {
        name += ch;
        m_producer->next();
        skipSpaces();
        ch = m_producer->currentChar();
    }
    
    return name;
}

std::string Parser2::readName(char lastChar) {
    std::string name;
    char ch = m_producer->currentChar();
    
    while (ch != lastChar && !m_producer->isEnd()) {
        name += ch;
        m_producer->next();
        ch = m_producer->currentChar();
    }
    
    return name;
}

bool Parser2::isLetterOrDigit(char ch) const {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

}