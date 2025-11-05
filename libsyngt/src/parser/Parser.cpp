#include <syngt/parser/Parser.h>
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

std::unique_ptr<RETree> Parser::parse(const std::string& text, Grammar* grammar) {
    CharProducer producer(text);
    return parseFromProducer(&producer, grammar);
}

std::unique_ptr<RETree> Parser::parseFromProducer(CharProducer* producer, Grammar* grammar) {
    m_producer = producer;
    m_grammar = grammar;
    
    skipSpaces();
    auto result = parseE();
    
    skipSpaces();
    if (m_producer->currentChar() == '.') {
        m_producer->next();
    }
    
    return result;
}

// E = T [';' T]*
std::unique_ptr<RETree> Parser::parseE() {
    auto left = parseT();
    
    skipSpaces();
    int altCount = 0;
    while (m_producer->currentChar() == ';') {
        altCount++;
        m_producer->next();
        skipSpaces();
        
        auto right = parseT();
        left = REOr::make(std::move(left), std::move(right));
        
        skipSpaces();
    }
    
    return left;
}

// T = F [',' F]*
std::unique_ptr<RETree> Parser::parseT() {
    auto left = parseF();
    
    skipSpaces();
    int seqCount = 0;
    while (m_producer->currentChar() == ',') {
        seqCount++;
        m_producer->next();
        skipSpaces();
        
        auto right = parseF();
        left = REAnd::make(std::move(left), std::move(right));
        
        skipSpaces();
    }
    
    return left;
}

// F = U ['#' U]*
std::unique_ptr<RETree> Parser::parseF() {
    auto left = parseU();
    
    skipSpaces();
    while (m_producer->currentChar() == '#') {
        m_producer->next();
        skipSpaces();
        
        auto right = parseU();
        left = REIteration::make(std::move(left), std::move(right));
        
        skipSpaces();
    }
    
    return left;
}

// U = K ['*' | '+' ]*
std::unique_ptr<RETree> Parser::parseU() {
    auto left = parseK();
    
    skipSpaces();
    char ch = m_producer->currentChar();
    
    while (ch == '*' || ch == '+') {
        char op = ch;
        m_producer->next();
        skipSpaces();
        
        if (m_producer->currentChar() == '@' || m_producer->currentChar() == '&') {
            m_producer->next();
            skipSpaces();
        }
        
        auto epsilon = std::make_unique<RETerminal>(m_grammar, 0);
        
        if (op == '*') {
            // K* -> Iteration(epsilon, K)
            left = REIteration::make(std::move(epsilon), std::move(left));
        } else {
            // K+ -> Iteration(K, epsilon)
            left = REIteration::make(std::move(left), std::move(epsilon));
        }
        
        skipSpaces();
        ch = m_producer->currentChar();
    }
    
    return left;
}

// K = Term | NonTerm | Semantic | Macro | '(' E ')' | '[' E ']' | '@'
std::unique_ptr<RETree> Parser::parseK() {
    skipSpaces();
    char ch = m_producer->currentChar();
    
    if (ch == '(') {
        m_producer->next();
        auto expr = parseE();
        skipSpaces();
        if (m_producer->currentChar() == ')') {
            m_producer->next();
        }
        return expr;
    }
    
    if (ch == '[') {
        m_producer->next();
        auto expr = parseE();
        skipSpaces();
        if (m_producer->currentChar() == ']') {
            m_producer->next();
        }
        skipSpaces();
        
        auto epsilon = std::make_unique<RETerminal>(m_grammar, 0);
        return REOr::make(std::move(expr), std::move(epsilon));
    }
    
    if (ch == '\'' || ch == '"') {
        m_producer->next();
        std::string name = readName(ch);
        m_producer->next();
        
        int id = m_grammar->addTerminal(name);
        return std::make_unique<RETerminal>(m_grammar, id);
    }
    
    if (ch == '@' || ch == '&') {
        m_producer->next();
        skipSpaces();
        
        char nextCh = m_producer->currentChar();
        if (nextCh == '*' || nextCh == '+') {
            m_producer->next();
            skipSpaces();
            
            auto inner = parseK();
            
            // @* -> Iteration(epsilon, K)
            // @+ -> Iteration(K, epsilon) 
            auto epsilon = std::make_unique<RETerminal>(m_grammar, 0);
            
            if (nextCh == '*') {
                return REIteration::make(std::move(epsilon), std::move(inner));
            } else {
                return REIteration::make(std::move(inner), std::move(epsilon));
            }
        }
        
        return std::make_unique<RETerminal>(m_grammar, 0);
    }
    
    if (ch == '$') {
        m_producer->next();
        std::string name = "$" + readIdentifier();
        int id = m_grammar->addSemantic(name);
        return std::make_unique<RESemantic>(m_grammar, id);
    }
    
    if (isLetterOrDigit(ch)) {
        std::string name = readIdentifier();
        
        if (name == "LETTER" || name == "DIGIT" || name == "ID" || 
            name == "chars" || name == "digit" || name == "digits") {
            int id = m_grammar->addTerminal(name);
            return std::make_unique<RETerminal>(m_grammar, id);
        }
        
        int id = m_grammar->addNonTerminal(name);
        return std::make_unique<RENonTerminal>(m_grammar, id, false);
    }
    
    throw std::runtime_error("Unexpected character in parser: " + std::string(1, ch));
}

void Parser::skipNotMatter() {
    char ch = m_producer->currentChar();
    
    while (ch != '\0' && 
           !isLetterOrDigit(ch) &&
           ch != '\'' && ch != '"' && 
           ch != '[' && ch != ']' && 
           ch != '(' && ch != ')' &&
           ch != '{' && ch != '}' && 
           ch != '*' && ch != '+' && 
           ch != ',' && ch != ';' && 
           ch != '#' && ch != '.' && 
           ch != '@' && ch != ':' && 
           ch != '$' && ch != '/' && ch != '&') 
    {
        m_producer->next();
        ch = m_producer->currentChar();
    }
}

void Parser::skipSpaces() {
    skipNotMatter();
    
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

void Parser::skipToChar(char ch) {
    while (m_producer->currentChar() != ch && !m_producer->isEnd()) {
        m_producer->next();
    }
}

std::string Parser::readIdentifier() {
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

std::string Parser::readName(char lastChar) {
    std::string name;
    char ch = m_producer->currentChar();
    
    while (ch != lastChar && !m_producer->isEnd()) {
        name += ch;
        m_producer->next();
        ch = m_producer->currentChar();
    }
    
    return name;
}

bool Parser::isLetterOrDigit(char ch) const {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

}