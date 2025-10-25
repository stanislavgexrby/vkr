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
    
    // Ожидаем точку в конце
    skipSpaces();
    if (m_producer->currentChar() == '.') {
        m_producer->next();
    }
    
    return result;
}

// E = T [';' T]*  (альтернативы)
std::unique_ptr<RETree> Parser::parseE() {
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
std::unique_ptr<RETree> Parser::parseT() {
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

// F = U ['#' U]*
std::unique_ptr<RETree> Parser::parseF() {
    auto left = parseU();
    
    skipSpaces();
    while (m_producer->currentChar() == '#') {
        m_producer->next();
        skipSpaces();
        
        auto right = parseU();
        // # оператор - создаем итерацию
        left = REIteration::make(std::move(left), std::move(right));
        
        skipSpaces();
    }
    
    return left;
}

// U = K ['*' | '+' ]*  (итерации)
// * и + это ПОСТФИКСНЫЕ операторы
std::unique_ptr<RETree> Parser::parseU() {
    auto left = parseK();
    
    skipSpaces();
    char ch = m_producer->currentChar();
    
    // Обработка постфиксных * и +
    while (ch == '*' || ch == '+') {
        char op = ch;  // Сохраняем оператор
        m_producer->next();
        skipSpaces();
        
        // Проверяем на *@ или +@
        if (m_producer->currentChar() == '@' || m_producer->currentChar() == '&') {
            m_producer->next();
            skipSpaces();
        }
        
        // Создаем итерацию
        auto epsilon = std::make_unique<RETerminal>(m_grammar, 0);
        
        if (op == '*') {
            // K* → Iteration(epsilon, K)
            left = REIteration::make(std::move(epsilon), std::move(left));
        } else {
            // K+ → Iteration(K, epsilon)
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
    
    // Скобки: ( E )
    if (ch == '(') {
        m_producer->next();
        auto expr = parseE();
        skipSpaces();
        if (m_producer->currentChar() == ')') {
            m_producer->next();
        }
        return expr;
    }
    
    // Квадратные скобки: [ E ] = Or(E, epsilon)
    if (ch == '[') {
        m_producer->next();
        auto expr = parseE();
        skipSpaces();
        if (m_producer->currentChar() == ']') {
            m_producer->next();
        }
        skipSpaces();
        
        // [E] = E ; epsilon (опциональность)
        auto epsilon = std::make_unique<RETerminal>(m_grammar, 0);
        return REOr::make(std::move(expr), std::move(epsilon));
    }
    
    // Терминал в кавычках: 'text' или "text"
    if (ch == '\'' || ch == '"') {
        m_producer->next();
        std::string name = readName(ch);
        m_producer->next();
        
        int id = m_grammar->addTerminal(name);
        return std::make_unique<RETerminal>(m_grammar, id);
    }
    
    // @ или @* или @ID
    if (ch == '@' || ch == '&') {
        m_producer->next();
        skipSpaces();
        
        // Проверяем @* или @+
        char nextCh = m_producer->currentChar();
        if (nextCh == '*' || nextCh == '+') {
            m_producer->next();
            skipSpaces();
            
            // Парсим следующий элемент K (обычно скобки)
            auto inner = parseK();
            
            // @* → Iteration(epsilon, K)
            // @+ → Iteration(K, epsilon) 
            auto epsilon = std::make_unique<RETerminal>(m_grammar, 0);
            
            if (nextCh == '*') {
                return REIteration::make(std::move(epsilon), std::move(inner));
            } else {
                return REIteration::make(std::move(inner), std::move(epsilon));
            }
        }
        
        // Просто @ или &  → epsilon терминал (ID=0)
        return std::make_unique<RETerminal>(m_grammar, 0);
    }
    
    // Семантика: $ID
    if (ch == '$') {
        m_producer->next();
        std::string name = "$" + readIdentifier();
        int id = m_grammar->addSemantic(name);
        return std::make_unique<RESemantic>(m_grammar, id);
    }
    
    // Нетерминал или специальный терминал
    if (isLetterOrDigit(ch)) {
        std::string name = readIdentifier();
        
        // Специальные терминалы
        if (name == "LETTER" || name == "DIGIT" || name == "ID" || 
            name == "chars" || name == "digit" || name == "digits") {
            int id = m_grammar->addTerminal(name);
            return std::make_unique<RETerminal>(m_grammar, id);
        }
        
        // Нетерминал
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