#pragma once
#include <string>

namespace syngt {

/**
 * @brief Производитель символов из строки
 * 
 * Последовательно читает символы из строки.
 * Используется парсером для разбора грамматик.
 */
class CharProducer {
private:
    std::string m_string;
    size_t m_index = 0;
    
public:
    /**
     * @brief Создать производитель из строки
     */
    explicit CharProducer(const std::string& s) 
        : m_string(s)
        , m_index(0) 
    {}
    
    /**
     * @brief Перейти к следующему символу
     * @return true если успешно, false если конец строки
     */
    bool next() {
        if (m_index < m_string.length()) {
            ++m_index;
            return true;
        }
        return false;
    }
    
    /**
     * @brief Получить текущий символ
     * @return Текущий символ или '\0' если конец
     */
    char currentChar() const {
        if (m_index < m_string.length()) {
            return m_string[m_index];
        }
        return '\0';
    }
    
    /**
     * @brief Получить текущую позицию
     */
    size_t index() const {
        return m_index;
    }
    
    /**
     * @brief Проверка на конец строки
     */
    bool isEnd() const {
        return m_index >= m_string.length();
    }
    
    /**
     * @brief Сбросить на начало
     */
    void reset() {
        m_index = 0;
    }
    
    /**
     * @brief Получить всю строку
     */
    const std::string& getString() const {
        return m_string;
    }
};

}