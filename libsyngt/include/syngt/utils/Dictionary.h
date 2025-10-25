#pragma once
#include <string>
#include <vector>
#include <syngt/core/Types.h>

/**
 * @brief Словарь типов грамматики
 * 
 * Используется при импорте грамматик из других форматов.
 * Сопоставляет названия разделов (TERMINALS, NONTERMINALS и т.д.)
 * с соответствующими списками в Grammar.
 */

namespace syngt {

/**
 * @brief Имена типов в импортируемых грамматиках
 * 
 * В Pascal был TStringList с индексами:
 * 0: TERMINALS
 * 1: FORWARDPASSSEMANTICS
 * 2: NONTERMINALS
 * 3: AUXILIARYNOTIONS (макросы)
 * 4+: BACKWARDPASSSEMANTICS и т.д. → все мапятся в cgSemanticList
 */
class Dictionary {
private:
    static const std::vector<std::string> s_names;
    
public:
    /**
     * @brief Найти тип словаря по имени
     * 
     * @param name Имя раздела (например "TERMINALS", "NONTERMINALS")
     * @return Индекс типа (cgTerminalList=0, cgSemanticList=1, cgNonTerminalList=2, cgMacroList=3)
     *         или cgSemanticList для неизвестных типов
     */
    static int findDictionaryType(const std::string& name);
    
    /**
     * @brief Получить имя типа по индексу
     */
    static const std::string& getDictionaryName(int type);
};

// Статическая инициализация словаря
inline const std::vector<std::string> Dictionary::s_names = {
    "TERMINALS",              // 0 = cgTerminalList
    "FORWARDPASSSEMANTICS",   // 1 = cgSemanticList (первый вариант)
    "NONTERMINALS",           // 2 = cgNonTerminalList
    "AUXILIARYNOTIONS",       // 3 = cgMacroList
    "BACKWARDPASSSEMANTICS",  // 4+ = все мапятся в cgSemanticList
    "FORWARDPASSRESOLVERS",
    "BACKWARDPASSRESOLVERS"
};

inline int Dictionary::findDictionaryType(const std::string& name) {
    std::string upperName = name;
    for (char& c : upperName) {
        c = std::toupper(static_cast<unsigned char>(c));
    }
    
    for (size_t i = 0; i < s_names.size(); ++i) {
        if (s_names[i] == upperName) {
            // Индексы 0-3 мапятся напрямую, индексы 4+ → cgSemanticList
            if (i < 4) {
                return static_cast<int>(i);
            } else {
                return cgSemanticList;
            }
        }
    }
    
    // По умолчанию - семантики
    return cgSemanticList;
}

inline const std::string& Dictionary::getDictionaryName(int type) {
    if (type >= 0 && type < static_cast<int>(s_names.size())) {
        return s_names[type];
    }
    static const std::string unknown = "UNKNOWN";
    return unknown;
}

} // namespace syngt