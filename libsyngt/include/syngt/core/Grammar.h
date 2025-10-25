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
class LeftFactorization;

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

    /**
     * @brief Получить имя терминала по ID
     */
    std::string getTerminalName(int id) const {
        return m_terminals->getString(id);
    }
    
    /**
     * @brief Получить имя нетерминала по ID
     */
    std::string getNonTerminalName(int id) const {
        return m_nonTerminals->getString(id);
    }
    
    /**
     * @brief Получить имя семантики по ID
     */
    std::string getSemanticName(int id) const {
        return m_semantics->getString(id);
    }
    
    /**
     * @brief Получить имя макроса по ID
     */
    std::string getMacroName(int id) const {
        return m_macros->getString(id);
    }

    /**
     * @brief Получить элемент нетерминала по имени
     */
    NTListItem* getNTItem(const std::string& name) const {
        return m_nonTerminals->getItemByName(name);
    }

    /**
     * @brief Получить элемент нетерминала по индексу
     */
    NTListItem* getNTItemByIndex(int index) const {
        return m_nonTerminals->getItem(index);
    }

    /**
     * @brief Установить правило для нетерминала (строка)
     */
    void setNTRule(const std::string& name, const std::string& rule);

    /**
     * @brief Установить правило для нетерминала (дерево)
     * Реализация в .cpp (требует полный тип RETree)
     */
    void setNTRoot(const std::string& name, std::unique_ptr<RETree> root);

    /**
     * @brief Проверить, есть ли правило у нетерминала
     */
    bool hasRule(const std::string& name) const;
    
    /**
     * @brief Регуляризация грамматики
     * 
     * Выполняет последовательность трансформаций:
     * 1. Устранение левой рекурсии
     * 2. Левая факторизация (TODO)
     * 3. Удаление бесполезных символов (TODO)
     */
    void regularize();

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