#pragma once
#include <string>
#include <memory>

namespace syngt {

class Grammar;
class ParsingTable;

/**
 * @brief Генератор кода предсказывающего парсера
 * 
 * Генерирует код парсера на основе LL(1) таблицы разбора
 */
class ParserGenerator {
public:
    enum class Language {
        CPP,        // C++
        Python,     // Python
        Java,       // Java
        CSharp      // C#
    };
    
    /**
     * @brief Генерировать парсер для грамматики
     * @param grammar Грамматика
     * @param table Таблица разбора
     * @param language Целевой язык
     * @param className Имя класса парсера
     * @return Сгенерированный код
     */
    static std::string generate(
        Grammar* grammar,
        const ParsingTable* table,
        Language language,
        const std::string& className = "Parser"
    );
    
    /**
     * @brief Сохранить сгенерированный парсер в файл
     */
    static void saveToFile(
        Grammar* grammar,
        const ParsingTable* table,
        Language language,
        const std::string& filename,
        const std::string& className = "Parser"
    );
    
private:
    /**
     * @brief Генерация для C++
     */
    static std::string generateCPP(
        Grammar* grammar,
        const ParsingTable* table,
        const std::string& className
    );
    
    /**
     * @brief Генерация для Python
     */
    static std::string generatePython(
        Grammar* grammar,
        const ParsingTable* table,
        const std::string& className
    );
    
    /**
     * @brief Генерация таблицы разбора для C++
     */
    static std::string generateTableCPP(
        Grammar* grammar,
        const ParsingTable* table
    );
    
    /**
     * @brief Генерация таблицы разбора для Python
     */
    static std::string generateTablePython(
        Grammar* grammar,
        const ParsingTable* table
    );
    
    /**
     * @brief Генерация метода parse для C++
     */
    static std::string generateParseMethodCPP(Grammar* grammar);
    
    /**
     * @brief Генерация метода parse для Python
     */
    static std::string generateParseMethodPython(Grammar* grammar);
};

}