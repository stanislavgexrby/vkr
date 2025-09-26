#pragma once
#include <syngt/core/TerminalList.h>
#include <syngt/core/NonTerminalList.h>
#include <syngt/core/SemanticList.h>
#include <syngt/core/MacroList.h>
#include <memory>
#include <string>
#include <vector>

namespace syngt {

class CharProducer;
class NTListItem;

/**
 * @brief Главный класс грамматики
 * 
 * Объединяет все списки: терминалы, нетерминалы, семантики, макросы.
 * Управляет загрузкой, сохранением и трансформациями грамматики.
 * 
 * Соответствует Pascal: TGrammar
 */
class Grammar {
private:
    std::unique_ptr<TerminalList> m_terminals;
    std::unique_ptr<SemanticList> m_semantics;
    std::unique_ptr<NonTerminalList> m_nonTerminals;
    std::unique_ptr<MacroList> m_macros;
    
    void addToDictionary(int dictionaryID, CharProducer* charProducer);
    
public:
    Grammar();
    ~Grammar() = default;
    
    /**
     * @brief Инициализировать новую грамматику
     * Создает стартовый нетерминал 'S'
     */
    void fillNew();
    
    /**
     * @brief Загрузить грамматику из файла
     * @param filename Путь к файлу .grm
     */
    void load(const std::string& filename);
    
    /**
     * @brief Сохранить грамматику в файл
     * @param filename Путь к файлу .grm
     */
    void save(const std::string& filename);
    
    /**
     * @brief Выполнить регуляризацию грамматики
     * Удаляет левую и правую рекурсию
     */
    void regularize();
    
    int addTerminal(const std::string& s) {
        return m_terminals->add(s);
    }
    
    int addSemantic(const std::string& s) {
        return m_semantics->add(s);
    }
    
    int addNonTerminal(const std::string& s) {
        return m_nonTerminals->add(s);
    }
    
    int addMacro(const std::string& s) {
        return m_macros->add(s);
    }
    
    int findTerminal(const std::string& s) const {
        return m_terminals->find(s);
    }
    
    int findNonTerminal(const std::string& s) const {
        return m_nonTerminals->find(s);
    }
    
    int findSemantic(const std::string& s) const {
        return m_semantics->find(s);
    }
    
    int findMacro(const std::string& s) const {
        return m_macros->find(s);
    }
    
    std::vector<std::string> getTerminals() const {
        return m_terminals->getItems();
    }
    
    std::vector<std::string> getNonTerminals() const {
        return m_nonTerminals->getItems();
    }
    
    std::vector<std::string> getSemantics() const {
        return m_semantics->getItems();
    }
    
    std::vector<std::string> getMacros() const {
        return m_macros->getItems();
    }
    
    // NTListItem* getNonTerminal(const std::string& name);
    
    TerminalList* terminals() { return m_terminals.get(); }
    const TerminalList* terminals() const { return m_terminals.get(); }
    
    NonTerminalList* nonTerminals() { return m_nonTerminals.get(); }
    const NonTerminalList* nonTerminals() const { return m_nonTerminals.get(); }
    
    SemanticList* semantics() { return m_semantics.get(); }
    const SemanticList* semantics() const { return m_semantics.get(); }
    
    MacroList* macros() { return m_macros.get(); }
    const MacroList* macros() const { return m_macros.get(); }
};

}