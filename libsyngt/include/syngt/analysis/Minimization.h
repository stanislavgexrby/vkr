#pragma once
#include <vector>
#include <string>
#include <map>
#include <set>
#include <memory>

namespace syngt {

class Grammar;

using State = int;

constexpr State StartState = 0;
constexpr State FinalState = 1;

constexpr const char* EmptySymbol = "";

class StatesSet {
private:
    std::set<State> m_states;
    
public:
    StatesSet() = default;
    
    void addState(State state) {
        m_states.insert(state);
    }
    
    bool findState(State state) const {
        return m_states.find(state) != m_states.end();
    }
    
    const std::set<State>& getStates() const {
        return m_states;
    }
    
    size_t count() const {
        return m_states.size();
    }
    
    bool empty() const {
        return m_states.empty();
    }
};

struct MinRecord {
    State start;   // Начальное состояние
    State finish;  // Конечное состояние
    
    MinRecord() : start(StartState), finish(FinalState) {}
    MinRecord(State s, State f) : start(s), finish(f) {}
};

class MinimizationTable {
private:
    // Таблица переходов: [состояние][символ] → набор состояний
    std::map<std::pair<State, std::string>, std::unique_ptr<StatesSet>> m_table;
    
    // Список символов алфавита
    std::vector<std::string> m_symbols;
    
    // Имена состояний (StartState, FinalState, State2, State3, ...)
    std::map<State, std::string> m_stateNames;
    
    // Счетчик состояний
    State m_nextState = 2; // 0=Start, 1=Final, 2+ = промежуточные
    
    int findOrAddSymbol(const std::string& symbol);
    
    void determinize();
    
    void minimizeByEquivalence();
    
public:
    MinimizationTable() = default;
    ~MinimizationTable() = default;
    
    State createState();
    
    void linkStates(State from, State to, const std::string& symbol);
    
    const StatesSet* getTableElement(State state, int symbolIndex) const;
    
    const std::string& getSymbol(int index) const;
    
    const std::string& getStateName(State state) const;
    
    void setStateName(State state, const std::string& name);
    
    int getStatesCount() const {
        return static_cast<int>(m_nextState);
    }
    
    int getSymbolsCount() const {
        return static_cast<int>(m_symbols.size());
    }
    
    void minimize();
    
    void writeToFile(const std::string& filename) const;
    
    std::string toString() const;
};

} // namespace syngt