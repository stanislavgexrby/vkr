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

    static void skipNotMatter(CharProducer* producer) {
    char ch = producer->currentChar();
    
    while (!producer->isEnd() && 
           !std::isalnum(static_cast<unsigned char>(ch)) && 
           ch != '_' && ch != '\'' && ch != '"' && 
           ch != '[' && ch != ']' && ch != '(' && ch != ')' && 
           ch != '{' && ch != '}' && ch != '*' && ch != '+' && 
           ch != ',' && ch != ';' && ch != '#' && ch != '.' && 
           ch != '@' && ch != ':' && ch != '$' && ch != '/' && ch != '&') 
    {
        producer->next();
        ch = producer->currentChar();
    }
}

void skipToChar(CharProducer* producer, char ch) {
    while (producer->currentChar() != ch && !producer->isEnd()) {
        producer->next();
    }
}

void skipSpaces(CharProducer* producer) {
    skipNotMatter(producer);
    
    while (producer->currentChar() == '{' || producer->currentChar() == '/') {
        if (producer->currentChar() == '{') {
            skipToChar(producer, '}');
        } else {
            skipToChar(producer, '\n');
        }
        producer->next();
        skipNotMatter(producer);
    }
}

bool isLetterOrDigit(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

std::string readIdentifier(CharProducer* producer) {
    skipSpaces(producer);
    std::string name;
    
    char ch = producer->currentChar();
    while (isLetterOrDigit(ch) && !producer->isEnd()) {
        name += ch;
        producer->next();
        skipSpaces(producer);
        ch = producer->currentChar();
    }
    
    return name;
}

std::unique_ptr<RETree> Parser2::parse(const std::string& text, Grammar* grammar) {
    CharProducer producer(text);
    return parseFromProducer(&producer, grammar);
}

std::unique_ptr<RETree> Parser2::parseFromProducer(CharProducer* producer, Grammar* grammar) {
    m_producer = producer;
    m_grammar = grammar;
    
    skipSpaces(m_producer);
    
    auto result = parseE();
    
    skipSpaces(m_producer);
    
    if (m_producer->currentChar() == '.') {
        m_producer->next();
    } else {
        throw std::runtime_error("Expected '.' at end of rule");
    }
    
    return result;
}

// E = T [';' T]*
std::unique_ptr<RETree> Parser2::parseE() {
    auto left = parseT();
    
    skipSpaces(m_producer);
    while (m_producer->currentChar() == ';') {
        m_producer->next();
        skipSpaces(m_producer);
        
        auto right = parseT();
        left = REOr::make(std::move(left), std::move(right));
        
        skipSpaces(m_producer);
    }
    
    return left;
}

// T = F [',' F]*
std::unique_ptr<RETree> Parser2::parseT() {
    auto left = parseF();
    
    skipSpaces(m_producer);
    while (m_producer->currentChar() == ',') {
        m_producer->next();
        skipSpaces(m_producer);
        
        auto right = parseF();
        left = REAnd::make(std::move(left), std::move(right));
        
        skipSpaces(m_producer);
    }
    
    return left;
}

// F = U ['*' U]*
std::unique_ptr<RETree> Parser2::parseF() {
    auto left = parseU();
    
    skipSpaces(m_producer);
    while (m_producer->currentChar() == '*') {
        m_producer->next();
        skipSpaces(m_producer);
        
        auto right = parseU();
        left = REIteration::make(std::move(left), std::move(right));
        
        skipSpaces(m_producer);
    }
    
    return left;
}

// U = $Semantic | Term | NonTerm | '(' E ')' | '[' E ']'
std::unique_ptr<RETree> Parser2::parseU() {
    skipSpaces(m_producer);
    char ch = m_producer->currentChar();
    
    if (ch == '(') {
        m_producer->next();
        auto expr = parseE();
        
        skipSpaces(m_producer);
        if (m_producer->currentChar() != ')') {
            throw std::runtime_error("Expected ')'");
        }
        m_producer->next();
        skipSpaces(m_producer);
        
        return expr;
    }
    
    if (ch == '[') {
        m_producer->next();
        auto expr = parseE();
        
        skipSpaces(m_producer);
        if (m_producer->currentChar() != ']') {
            throw std::runtime_error("Expected ']'");
        }
        m_producer->next();
        skipSpaces(m_producer);
        
        auto epsilon = std::make_unique<RETerminal>(m_grammar, 0);
        return REOr::make(std::move(epsilon), std::move(expr));
    }
    
    if (ch == '\'' || ch == '"') {
        char quote = ch;
        m_producer->next();
        
        std::string name = readName(quote);
        
        if (!m_producer->next()) {
            throw std::runtime_error(std::string("Expected closing ") + quote);
        }
        
        skipSpaces(m_producer);
        
        int id = m_grammar->addTerminal(name);
        return std::make_unique<RETerminal>(m_grammar, id);
    }
    
    if (ch == '@' || ch == '&') {
        m_producer->next();
        skipSpaces(m_producer);
        return std::make_unique<RETerminal>(m_grammar, 0);
    }
    
    if (ch == '$') {
        m_producer->next();
        std::string name = "$" + readIdentifier(m_producer);
        
        int id = m_grammar->findSemantic(name);
        if (id < 0) {
            id = m_grammar->addSemantic(name);
        }
        
        return std::make_unique<RESemantic>(m_grammar, id);
    }
    
    if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
        std::string name = readIdentifier(m_producer);
        
        int id = m_grammar->findNonTerminal(name);
        if (id < 0) {
            id = m_grammar->addNonTerminal(name);
        }
        
        return std::make_unique<RENonTerminal>(m_grammar, id, false);
    }
    
    throw std::runtime_error(std::string("Unexpected character: ") + ch);
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

} // namespace syngt