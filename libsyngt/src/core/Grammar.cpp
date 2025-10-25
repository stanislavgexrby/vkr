#include <syngt/core/Grammar.h>
#include <syngt/parser/Parser.h>
#include <syngt/core/NTListItem.h>
#include <syngt/transform/LeftElimination.h>
#include <syngt/transform/LeftFactorization.h>
#include <syngt/transform/RemoveUseless.h>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace syngt {

Grammar::Grammar() 
    : m_terminals(std::make_unique<TerminalList>())
    , m_semantics(std::make_unique<SemanticList>())
    , m_nonTerminals(std::make_unique<NonTerminalList>())
    , m_macros(std::make_unique<MacroList>())
{
    m_nonTerminals->setGrammar(this);
}

void Grammar::fillNew() {
    m_terminals->clear();
    m_semantics->clear();
    m_nonTerminals->clear();
    m_macros->clear();
    
    // КРИТИЧНО: Добавляем epsilon как первый терминал (ID=0)
    // Пустая строка используется как epsilon в операциях @*, @+, [E] и т.д.
    m_terminals->add("");  // Epsilon терминал
    
    m_nonTerminals->fillNew();
}

void Grammar::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    fillNew();  // Инициализация включает добавление epsilon
    
    Parser parser;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.find("EOGram!") != std::string::npos) {
            break;
        }
        
        if (line.empty() || line[0] == '{' || line[0] == '/') {
            continue;
        }
        
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        
        std::string name = line.substr(0, colonPos);
        name.erase(0, name.find_first_not_of(" \t"));
        name.erase(name.find_last_not_of(" \t") + 1);
        
        std::string rule = line.substr(colonPos + 1);
        
        int ntId = addNonTerminal(name);
        (void)ntId;
        
        try {
            auto tree = parser.parse(rule, this);
            setNTRoot(name, std::move(tree));
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse rule for '" << name << "': " << e.what() << "\n";
            std::cerr << "  Rule was: " << rule << "\n";
        }
    }
    
    file.close();
}

void Grammar::save(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + filename);
    }
    
    auto nts = m_nonTerminals->getItems();
    for (size_t i = 0; i < nts.size(); ++i) {
        NTListItem* item = m_nonTerminals->getItem(static_cast<int>(i));
        if (item && item->hasRoot()) {
            file << nts[i] << " : ";
            
            SelectionMask emptyMask;
            file << item->root()->toString(emptyMask, false);
            
            file << " .\n";
        }
    }
    
    file << "EOGram!\n";
    file.close();
}

void Grammar::addToDictionary(int dictionaryID, CharProducer* charProducer) {
    (void)dictionaryID;
    (void)charProducer;
}

void Grammar::setNTRule(const std::string& name, const std::string& rule) {
    NTListItem* item = getNTItem(name);
    if (item) {
        item->setValue(rule);
    }
}

void Grammar::setNTRoot(const std::string& name, std::unique_ptr<RETree> root) {
    m_nonTerminals->setRootByName(name, std::move(root));
}

bool Grammar::hasRule(const std::string& name) const {
    NTListItem* item = getNTItem(name);
    return item && item->hasRoot();
}

void Grammar::regularize() {
    // 1. Устранение левой рекурсии
    LeftElimination::eliminate(this);
    
    // 2. Левая факторизация
    LeftFactorization::factorizeAll(this);
    
    // 3. Удаление бесполезных символов
    RemoveUseless::remove(this);
}

}