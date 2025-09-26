#include <syngt/core/Grammar.h>
#include <syngt/parser/Parser.h>
#include <syngt/core/NTListItem.h>
#include <fstream>
#include <stdexcept>
#include <sstream>

namespace syngt {

Grammar::Grammar() 
    : m_terminals(std::make_unique<TerminalList>())
    , m_semantics(std::make_unique<SemanticList>())
    , m_nonTerminals(std::make_unique<NonTerminalList>())
    , m_macros(std::make_unique<MacroList>())
{
}

void Grammar::fillNew() {
    m_terminals->clear();
    m_semantics->clear();
    m_nonTerminals->clear();
    m_macros->clear();
    
    m_nonTerminals->fillNew();
}

void Grammar::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    fillNew();
    
    Parser parser;
    std::string line;
    
    while (std::getline(file, line)) {
        // Проверяем конец грамматики
        if (line.find("EOGram!") != std::string::npos) {
            break;
        }
        
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '{' || line[0] == '/') {
            continue;
        }
        
        // Ищем двоеточие (разделитель имя : правило)
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        
        // Извлекаем имя нетерминала
        std::string name = line.substr(0, colonPos);
        // Убираем пробелы
        name.erase(0, name.find_first_not_of(" \t"));
        name.erase(name.find_last_not_of(" \t") + 1);
        
        // Извлекаем правило
        std::string rule = line.substr(colonPos + 1);
        
        // Добавляем нетерминал
        int ntId = addNonTerminal(name);
        (void)ntId; // TODO: пока не используем

        // Парсим правило
        try {
            auto tree = parser.parse(rule, this);
            
            // TODO: Сохранить дерево в NTListItem
            // Пока просто парсим
            
        } catch (const std::exception& e) {
            // Игнорируем ошибки парсинга (можно логировать)
        }
    }
    
    file.close();
}

void Grammar::save(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + filename);
    }
    
    // Записываем правила
    for (const auto& nt : m_nonTerminals->getItems()) {
        file << nt << " : /* TODO: правило */ .\n";
    }
    
    file << "EOGram!\n";
    file.close();
}

void Grammar::regularize() {
    // TODO: Реализовать регуляризацию
}

void Grammar::addToDictionary(int dictionaryID, CharProducer* charProducer) {
    (void)dictionaryID;
    (void)charProducer;
}

}