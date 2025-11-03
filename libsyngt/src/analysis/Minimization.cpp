#include <syngt/analysis/Minimization.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace syngt {

State MinimizationTable::createState() {
    State newState = m_nextState++;
    
    std::ostringstream oss;
    oss << "State" << newState;
    m_stateNames[newState] = oss.str();
    
    return newState;
}

void MinimizationTable::linkStates(State from, State to, const std::string& symbol) {
    findOrAddSymbol(symbol);
    
    auto key = std::make_pair(from, symbol);
    auto it = m_table.find(key);
    
    if (it == m_table.end()) {
        m_table[key] = std::make_unique<StatesSet>();
    }
    
    m_table[key]->addState(to);
}

int MinimizationTable::findOrAddSymbol(const std::string& symbol) {
    auto it = std::find(m_symbols.begin(), m_symbols.end(), symbol);
    
    if (it != m_symbols.end()) {
        return static_cast<int>(std::distance(m_symbols.begin(), it));
    }
    
    m_symbols.push_back(symbol);
    return static_cast<int>(m_symbols.size() - 1);
}

const StatesSet* MinimizationTable::getTableElement(State state, int symbolIndex) const {
    if (symbolIndex < 0 || symbolIndex >= static_cast<int>(m_symbols.size())) {
        return nullptr;
    }
    
    auto key = std::make_pair(state, m_symbols[symbolIndex]);
    auto it = m_table.find(key);
    
    if (it != m_table.end()) {
        return it->second.get();
    }
    
    return nullptr;
}

const std::string& MinimizationTable::getSymbol(int index) const {
    static const std::string empty;
    
    if (index >= 0 && index < static_cast<int>(m_symbols.size())) {
        return m_symbols[index];
    }
    
    return empty;
}

const std::string& MinimizationTable::getStateName(State state) const {
    auto it = m_stateNames.find(state);
    
    if (it != m_stateNames.end()) {
        return it->second;
    }
    
    if (state == StartState) {
        static const std::string start = "StartState";
        return start;
    } else if (state == FinalState) {
        static const std::string final = "FinalState";
        return final;
    }
    
    static const std::string unknown = "Unknown";
    return unknown;
}

void MinimizationTable::setStateName(State state, const std::string& name) {
    m_stateNames[state] = name;
}

void MinimizationTable::minimize() {
    // 1. Детерминизация (если нужно)
    // determinize();
    
    // 2. Минимизация
    // minimizeByEquivalence();
    
    // TODO: Полная реализация алгоритма Хопкрофта
    // Пока оставляем автомат как есть
}

void MinimizationTable::determinize() {
    // TODO: Алгоритм детерминизации NFA → DFA
    // Используется подмножественная конструкция
}

void MinimizationTable::minimizeByEquivalence() {
    // TODO: Алгоритм минимизации через классы эквивалентности
    // Разбиваем состояния на классы неразличимых состояний
}

void MinimizationTable::writeToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return;
    }
    
    file << toString();
    file.close();
}

std::string MinimizationTable::toString() const {
    std::ostringstream oss;
    
    oss << std::setw(10) << "";
    for (const auto& symbol : m_symbols) {
        std::string displaySymbol = symbol;
        if (symbol.empty()) {
            displaySymbol = "ε"; // epsilon
        } else if (symbol.size() > 0 && symbol[0] == '\'') {
            displaySymbol = symbol;
        } else {
            displaySymbol = "'" + symbol + "'";
        }
        oss << std::setw(12) << displaySymbol;
    }
    oss << "\n";
    
    for (State state = 0; state < m_nextState; ++state) {
        std::string stateName = getStateName(state);
        
        if (state == FinalState) {
            stateName = "X";
        }
        
        oss << std::setw(10) << stateName;
        
        for (size_t symIdx = 0; symIdx < m_symbols.size(); ++symIdx) {
            const StatesSet* statesSet = getTableElement(state, static_cast<int>(symIdx));
            
            if (statesSet && !statesSet->empty()) {
                std::ostringstream cell;
                bool first = true;
                
                for (State s : statesSet->getStates()) {
                    if (!first) cell << " ";
                    
                    if (s == FinalState) {
                        cell << "X";
                    } else {
                        cell << s;
                    }
                    
                    first = false;
                }
                
                oss << std::setw(12) << cell.str();
            } else {
                oss << std::setw(12) << "";
            }
        }
        
        oss << "\n";
    }
    
    return oss.str();
}

} // namespace syngt