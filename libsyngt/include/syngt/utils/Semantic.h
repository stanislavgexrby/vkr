#pragma once
#include <vector>
#include <memory>

namespace syngt {

/**
 * @brief Список ID семантик
 * 
 * Используется для хранения последовательности семантических действий
 * на дугах синтаксической диаграммы.
 * 
 * Соответствует Pascal: unit Semantic → TSemanticIDList
 */
class SemanticIDList {
private:
    std::vector<int> m_items;
    
public:
    SemanticIDList() = default;
    ~SemanticIDList() = default;
    
    /**
     * @brief Создать копию списка
     */
    std::unique_ptr<SemanticIDList> copy() const {
        auto result = std::make_unique<SemanticIDList>();
        result->m_items = m_items;
        return result;
    }
    
    /**
     * @brief Добавить ID семантики
     */
    void add(int id) {
        m_items.push_back(id);
    }
    
    /**
     * @brief Получить длину (для расчета размера стрелки)
     * @return Примерная длина в пикселях
     */
    int getLength() const {
        return static_cast<int>(m_items.size()) * 20;
    }
    
    /**
     * @brief Получить количество элементов
     */
    int count() const {
        return static_cast<int>(m_items.size());
    }
    
    /**
     * @brief Получить ID по индексу
     */
    int getID(int index) const {
        return m_items[index];
    }
    
    /**
     * @brief Получить все ID
     */
    const std::vector<int>& getItems() const {
        return m_items;
    }
    
    /**
     * @brief Очистить список
     */
    void clear() {
        m_items.clear();
    }
    
    /**
     * @brief Проверка на пустоту
     */
    bool isEmpty() const {
        return m_items.empty();
    }
    
    void save();
    void load();
};

}