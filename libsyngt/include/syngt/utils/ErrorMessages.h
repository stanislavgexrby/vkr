#pragma once
#include <string>
#include <stdexcept>

/**
 * @brief Сообщения об ошибках и функции обработки ошибок
 */

namespace syngt {

namespace ErrorMessages {
    inline std::string cantFindName(const std::string& name) {
        return "Can't find Identifier>" + name + "< in Dictionaries";
    }
    
    inline std::string cantFindChar(char ch) {
        return std::string("Waiting for char '") + ch + "'";
    }
    
    inline std::string cantAddSemantic(const std::string& name) {
        return "Can't add Context Symbol>" + name + "<: because name already exists";
    }
    
    inline std::string cantAddMacro(const std::string& name) {
        return "Can't add Auxiliary Notions>" + name + "<: because name already exists";
    }
    
    inline std::string cantAddNonTerminal(const std::string& name) {
        return "Can't add NonTerminal>" + name + "<: because name already exists";
    }
    
    inline std::string cantOpenAllMacro(const std::string& name) {
        return "Can't open all Auxiliary Notions because >" + name + "< has recursion definition";
    }
}

/**
 * @brief Выбросить исключение с ошибкой
 */
inline void throwError(const std::string& msg) {
    throw std::runtime_error(msg);
}

/**
 * @brief Вывести предупреждение
 */
inline void logWarning(const std::string& msg) {
    // В консольной версии просто игнорируем
    // В GUI можно будет показывать диалог
    (void)msg;
}

} // namespace syngt