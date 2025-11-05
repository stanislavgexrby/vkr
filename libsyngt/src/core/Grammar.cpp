#include <syngt/core/Grammar.h>
#include <syngt/parser/Parser.h>
#include <syngt/parser/Parser2.h>
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
    m_terminals = std::make_unique<TerminalList>();
    m_terminals->add("");
    
    m_semantics = std::make_unique<SemanticList>();
    
    m_nonTerminals = std::make_unique<NonTerminalList>();
    m_nonTerminals->setGrammar(this);
    
    m_macros = std::make_unique<MacroList>();
}

void Grammar::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    fillNew();
    
    Parser parser;
    std::string line;
    std::string currentRule;
    std::string currentName;
    bool readingRule = false;
    
    auto findEndOfRule = [](const std::string& text) -> size_t {
        bool inQuotes = false;
        bool inComment = false;
        char quoteChar = '\0';
        
        for (size_t i = 0; i < text.length(); ++i) {
            char ch = text[i];
            
            if (inComment) {
                if (ch == '}') {
                    inComment = false;
                }
                continue;
            }
            
            if (!inQuotes) {
                if (ch == '{') {
                    inComment = true;
                } else if (ch == '\'' || ch == '"') {
                    inQuotes = true;
                    quoteChar = ch;
                } else if (ch == '.') {
                    return i;
                }
            } else {
                if (ch == quoteChar) {
                    inQuotes = false;
                    quoteChar = '\0';
                }
            }
        }
        
        return std::string::npos;
    };
    
    while (std::getline(file, line)) {
        if (line.find("EOGram!") != std::string::npos) {
            break;
        }
        
        if (line.empty() || line[0] == '{' || line[0] == '/') {
            continue;
        }
        
        size_t colonPos = line.find(':');
        
        if (colonPos != std::string::npos && !readingRule) {
            currentName = line.substr(0, colonPos);
            currentName.erase(0, currentName.find_first_not_of(" \t"));
            currentName.erase(currentName.find_last_not_of(" \t") + 1);
            
            currentRule = line.substr(colonPos + 1);
            
            int ntId = addNonTerminal(currentName);
            (void)ntId;
            
            readingRule = true;
        } else if (readingRule) {
            currentRule += " " + line;
        }
        
        if (readingRule) {
            size_t dotPos = findEndOfRule(currentRule);
            
            if (dotPos != std::string::npos) {
                std::string ruleText = currentRule.substr(0, dotPos);
                ruleText += ".";
                
                try {
                    auto tree = parser.parse(ruleText, this);
                    setNTRoot(currentName, std::move(tree));
                } catch (const std::exception& e) {
                    std::cerr << "Failed to parse rule for '" << currentName << "': " << e.what() << "\n";
                    std::cerr << "  Rule was: " << ruleText << "\n";
                }
                
                currentRule.clear();
                currentName.clear();
                readingRule = false;
            }
        }
    }
    
    if (readingRule && !currentName.empty() && !currentRule.empty()) {
        if (currentRule.back() != '.') {
            currentRule += ".";
        }
        
        try {
            auto tree = parser.parse(currentRule, this);
            setNTRoot(currentName, std::move(tree));
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse rule for '" << currentName << "': " << e.what() << "\n";
            std::cerr << "  Rule was: " << currentRule << "\n";
        }
    }
    
    file.close();
}

void Grammar::importFromGEdit(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    std::string fileContent = buffer.str();
    
    fillNew();
    
    CharProducer producer(fileContent);
    Parser2 parser;
    
    std::string name = readIdentifier(&producer);
    
    while (true) {
        std::string upperName = name;
        for (char& c : upperName) {
            c = std::toupper(static_cast<unsigned char>(c));
        }
        
        if (upperName == "EOGRAM") {
            break;
        }
        
        int ntId = findNonTerminal(name);
        if (ntId < 0) {
            ntId = addNonTerminal(name);
        }
        
        if (!producer.next()) {
            break;
        }
        
        std::string preview;
        size_t savedPos = producer.index();
        for (int i = 0; i < 50 && !producer.isEnd(); ++i) {
            char ch = producer.currentChar();
            if (ch == '\n') preview += "\\n";
            else if (ch == '\t') preview += "\\t";
            else if (ch == '\r') preview += "\\r";
            else preview += ch;
            producer.next();
        }
        
        producer.reset();
        for (size_t i = 0; i < savedPos; ++i) producer.next();
        
        try {
            auto tree = parser.parseFromProducer(&producer, this);
            
            if (tree) {
                SelectionMask mask;
                std::string ruleStr = tree->toString(mask, false);
                setNTRoot(name, std::move(tree));
            }
        } catch (const std::exception& e) {
            std::cerr << "[ImportFromGEdit] ERROR parsing rule: " << e.what() << "\n";
        }
        
        if (!producer.next()) {
            break;
        }
        
        name = readIdentifier(&producer);
    }
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
    LeftElimination::eliminate(this);
    
    LeftFactorization::factorizeAll(this);
    
    RemoveUseless::remove(this);
}

}