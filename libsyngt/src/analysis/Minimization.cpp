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
    // Port of Pascal TMinimizationTable.minimize() algorithm:
    // Repeatedly find pairs of states with identical transitions and merge them.
    // Two states can be merged if, for every symbol, their target state sets
    // are equal (treating the two candidates as interchangeable).
    // The FinalState is never merged with other states.

    // Collect all states that appear in the table
    std::set<State> allStates;
    allStates.insert(StartState);
    allStates.insert(FinalState);
    for (const auto& entry : m_table) {
        allStates.insert(entry.first.first);
        for (State s : entry.second->getStates()) {
            allStates.insert(s);
        }
    }

    // Helper: normalize state (treat s1 == s2 as the same)
    auto normalize = [](State s, State s1, State s2) -> State {
        return (s == s2) ? s1 : s;
    };

    // Helper: check if transition sets of s1 and s2 are equal (treating s1 ≡ s2)
    auto setsEqual = [&](const StatesSet* ss1, const StatesSet* ss2,
                         State s1, State s2) -> bool {
        for (State t : ss1->getStates()) {
            State nt = normalize(t, s1, s2);
            bool found = false;
            for (State t2 : ss2->getStates()) {
                if (normalize(t2, s1, s2) == nt) { found = true; break; }
            }
            if (!found) return false;
        }
        for (State t : ss2->getStates()) {
            State nt = normalize(t, s1, s2);
            bool found = false;
            for (State t1 : ss1->getStates()) {
                if (normalize(t1, s1, s2) == nt) { found = true; break; }
            }
            if (!found) return false;
        }
        return true;
    };

    // Helper: can we merge s2 into s1?
    auto canJoin = [&](State s1, State s2) -> bool {
        if (s1 == FinalState || s2 == FinalState) return false;
        for (const auto& sym : m_symbols) {
            auto it1 = m_table.find({s1, sym});
            auto it2 = m_table.find({s2, sym});
            bool has1 = (it1 != m_table.end() && !it1->second->empty());
            bool has2 = (it2 != m_table.end() && !it2->second->empty());
            if (has1 != has2) return false;
            if (!has1) continue;
            if (!setsEqual(it1->second.get(), it2->second.get(), s1, s2)) return false;
        }
        return true;
    };

    // Helper: merge s2 into s1 — replace all s2 → s1 in the table, remove s2's rows
    auto joinStates = [&](State s1, State s2) {
        // Replace s2 with s1 in all target sets
        for (auto& entry : m_table) {
            if (entry.second->findState(s2)) {
                auto newSS = std::make_unique<StatesSet>();
                for (State t : entry.second->getStates()) {
                    newSS->addState(t == s2 ? s1 : t);
                }
                entry.second = std::move(newSS);
            }
        }
        // Move s2's outgoing transitions to s1 (if s1 doesn't already have them)
        for (const auto& sym : m_symbols) {
            auto k2 = std::make_pair(s2, sym);
            auto it2 = m_table.find(k2);
            if (it2 != m_table.end()) {
                auto k1 = std::make_pair(s1, sym);
                if (m_table.find(k1) == m_table.end()) {
                    auto newSS = std::make_unique<StatesSet>();
                    for (State t : it2->second->getStates()) {
                        newSS->addState(t == s2 ? s1 : t);
                    }
                    m_table[k1] = std::move(newSS);
                }
                m_table.erase(k2);
            }
        }
    };

    // Main loop: restart whenever a merge is performed (Pascal goto startLoop)
    bool changed = true;
    while (changed) {
        changed = false;
        std::vector<State> stateVec(allStates.begin(), allStates.end());
        for (size_t i = 0; i < stateVec.size() && !changed; ++i) {
            for (size_t j = i + 1; j < stateVec.size() && !changed; ++j) {
                if (canJoin(stateVec[i], stateVec[j])) {
                    joinStates(stateVec[i], stateVec[j]);
                    allStates.erase(stateVec[j]);
                    changed = true;
                }
            }
        }
    }
}

void MinimizationTable::determinize() {
    // Not needed: DFAToRegex handles NFA directly via state elimination
}

void MinimizationTable::minimizeByEquivalence() {
    // Implemented above in minimize()
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