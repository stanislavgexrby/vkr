#include <syngt/analysis/DFAToRegex.h>
#include <syngt/core/Grammar.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REIteration.h>
#include <algorithm>
#include <stdexcept>

namespace syngt {

std::unique_ptr<DFAToRegex> DFAToRegex::fromMinimizationTable(
    Grammar* grammar,
    const MinimizationTable* table
) {
    auto converter = std::make_unique<DFAToRegex>(grammar);
    
    int statesCount = table->getStatesCount();
    int symbolsCount = table->getSymbolsCount();
    
    // Создаем состояния: 0=новый старт, 1=новый финиш, 2+statesCount=состояния автомата
    converter->m_states.reserve(statesCount + 2);
    for (int i = 0; i < statesCount + 2; ++i) {
        converter->m_states.push_back(i);
    }
    
    // Дуга: новый_старт → старый_старт (epsilon)
    converter->m_arcs.push_back(std::make_unique<Arc>(
        0, 2, converter->createEpsilon()
    ));
    
    // Дуга: старый_финиш → новый_финиш (epsilon)
    for (int fromState = 0; fromState < statesCount; ++fromState) {
        if (table->getStateName(fromState) == "FinalState") {
            converter->m_arcs.push_back(std::make_unique<Arc>(
                fromState + 2, 1, converter->createEpsilon()
            ));
            break;
        }
    }
    
    // Создаем дуги из таблицы переходов
    for (int fromState = 0; fromState < statesCount; ++fromState) {
        for (int iSymbol = 0; iSymbol < symbolsCount; ++iSymbol) {
            const StatesSet* statesSet = table->getTableElement(fromState, iSymbol);
            
            if (!statesSet || statesSet->empty()) {
                continue;
            }
            
            std::string symbolName = table->getSymbol(iSymbol);
            std::unique_ptr<RETree> tree;
            
            // Проверяем формат символа
            if (symbolName.size() >= 2 && 
                (symbolName[0] == '"' || symbolName[0] == '\'')) {
                // Терминал в кавычках: извлекаем содержимое
                std::string content = symbolName.substr(1, symbolName.size() - 2);
                int termId = grammar->addTerminal(content);
                tree = std::make_unique<RETerminal>(grammar, termId);
            } else {
                // Нетерминал
                int ntId = grammar->findNonTerminal(symbolName);
                if (ntId < 0) {
                    ntId = grammar->addNonTerminal(symbolName);
                }
                tree = std::make_unique<RENonTerminal>(grammar, ntId);
            }
            
            // Создаем дуги для всех целевых состояний
            for (State toState : statesSet->getStates()) {
                converter->m_arcs.push_back(std::make_unique<Arc>(
                    fromState + 2,
                    toState + 2,
                    tree->copy()  // Копируем дерево для каждой дуги
                ));
            }
        }
    }
    
    return converter;
}

void DFAToRegex::mergeParallelArcs() {
    // Объединяем дуги с одинаковыми from/to через OR
    size_t i = 0;
    while (i < m_arcs.size()) {
        Arc* arc1 = m_arcs[i].get();
        
        // Проверяем все последующие дуги
        for (size_t j = m_arcs.size() - 1; j > i; --j) {
            Arc* arc2 = m_arcs[j].get();
            
            if (arc1->fromState == arc2->fromState &&
                arc1->toState == arc2->toState) {
                // Объединяем через OR
                arc1->tree = createOr(std::move(arc1->tree), std::move(arc2->tree));
                
                // Удаляем arc2
                m_arcs.erase(m_arcs.begin() + j);
            }
        }
        
        ++i;
    }
}

void DFAToRegex::removeAllStates() {
    // Удаляем состояния, пока не останется только Start(0) и Final(1)
    while (m_states.size() > 2) {
        mergeParallelArcs();
        
        size_t bestIndex = findBestStateToRemove();
        removeState(bestIndex);
    }
    
    // Финальное объединение
    mergeParallelArcs();
}

void DFAToRegex::removeState(size_t stateIndex) {
    State state = m_states[stateIndex];
    
    // 1. Найти петлю (S→S) если есть
    std::unique_ptr<RETree> loopTree;
    
    for (const auto& arc : m_arcs) {
        if (arc->fromState == state && arc->toState == state) {
            loopTree = arc->tree->copy();
            break;
        }
    }
    
    // Создаем итерацию петли: (loop)* или epsilon если петли нет
    std::unique_ptr<RETree> loopIteration;
    if (loopTree) {
        loopIteration = createUnaryIteration(std::move(loopTree));
    } else {
        loopIteration = createEpsilon();
    }
    
    // 2. Для каждой пары (X→S, S→Y) создаем дугу X→Y
    std::vector<std::unique_ptr<Arc>> newArcs;
    
    for (const auto& inArc : m_arcs) {
        // Входящая дуга: X→S (не петля)
        if (inArc->toState == state && inArc->fromState != state) {
            
            for (const auto& outArc : m_arcs) {
                // Исходящая дуга: S→Y (не петля)
                if (outArc->fromState == state && outArc->toState != state) {
                    
                    // Создаем новую дугу: X→Y
                    // Выражение: inArc, (loop)*, outArc
                    auto newTree = createAnd(
                        createAnd(
                            inArc->tree->copy(),
                            loopIteration->copy()
                        ),
                        outArc->tree->copy()
                    );
                    
                    newArcs.push_back(std::make_unique<Arc>(
                        inArc->fromState,
                        outArc->toState,
                        std::move(newTree)
                    ));
                }
            }
        }
    }
    
    // 3. Удаляем все дуги, связанные с состоянием S
    m_arcs.erase(
        std::remove_if(m_arcs.begin(), m_arcs.end(),
            [state](const std::unique_ptr<Arc>& arc) {
                return arc->fromState == state || arc->toState == state;
            }),
        m_arcs.end()
    );
    
    // 4. Добавляем новые дуги
    for (auto& arc : newArcs) {
        m_arcs.push_back(std::move(arc));
    }
    
    // 5. Удаляем состояние из списка
    m_states.erase(m_states.begin() + stateIndex);
}

size_t DFAToRegex::findBestStateToRemove() {
    // Пропускаем состояния 0 (Start) и 1 (Final)
    size_t bestIndex = 2;
    int bestCost = INT_MAX;
    
    // Используем метку для restart (как в Pascal с goto)
    bool needRestart;
    
    do {
        needRestart = false;
        bestIndex = m_states.size() - 1;
        bestCost = INT_MAX;
        
        for (size_t i = m_states.size() - 1; i >= 2; --i) {
            State state = m_states[i];
            int cost = getStateRemovalCost(state);
            
            if (cost < bestCost) {
                if (cost < 0) {
                    // Исключение: была оптимизация, нужен перерасчет
                    needRestart = true;
                    break;
                }
                
                if (cost == 0) {
                    // Лучший случай: 1 вход, 1 выход
                    return i;
                }
                
                bestCost = cost;
                bestIndex = i;
            }
            
            if (i == 0) break; // Защита от underflow
        }
    } while (needRestart);
    
    return bestIndex;
}

int DFAToRegex::getStateRemovalCost(State state) {
    int inCount = 0, outCount = 0;
    int inLength = 0, outLength = 0, loopLength = 0;
    Arc* singleInArc = nullptr;
    
    // Подсчитываем входящие/исходящие дуги
    for (const auto& arc : m_arcs) {
        if (arc->fromState == state) {
            if (arc->toState == state) {
                // Петля
                loopLength += arc->tree->getOperationCount();
            } else {
                // Исходящая
                ++outCount;
                outLength += arc->tree->getOperationCount();
            }
        } else if (arc->toState == state) {
            // Входящая
            ++inCount;
            inLength += arc->tree->getOperationCount();
            singleInArc = arc.get();
        }
    }
    
    // Оптимальный случай: 1 вход, 1 выход
    if (inCount <= 1 && outCount <= 1) {
        // Проверяем оптимизацию бинарной итерации
        if (inCount == 1 && loopLength == 0 && singleInArc) {
            if (tryOptimizeBinaryIteration(singleInArc)) {
                return -1; // Сигнал для перерасчета
            }
        }
        return 0;
    }
    
    // Вычисляем стоимость
    int cost = inLength * (outCount - 1) + 
               outLength * (inCount - 1) + 
               inCount * outCount * loopLength;
    
    return cost;
}

bool DFAToRegex::tryOptimizeBinaryIteration(Arc* inArc) {
    State stateFrom = inArc->fromState;
    State stateTo = inArc->toState;
    
    bool foundOtherOut = false;
    bool foundBackArc = false;
    Arc* backArc = nullptr;
    
    // Проверяем условия для бинарной итерации
    for (const auto& arc : m_arcs) {
        if (arc->fromState == stateFrom) {
            if (arc->toState != stateTo) {
                foundOtherOut = true;
                break; // Есть другой выход - нельзя оптимизировать
            }
        }
        
        if (arc->toState == stateFrom && arc->fromState == stateTo) {
            foundBackArc = true;
            backArc = arc.get();
        }
    }
    
    // Применяем оптимизацию если возможно
    if (foundBackArc && !foundOtherOut && backArc) {
        // Создаем итерацию: forward # backward
        inArc->tree = createIteration(
            std::move(inArc->tree),
            backArc->tree->copy()
        );
        
        // Удаляем обратную дугу
        m_arcs.erase(
            std::remove_if(m_arcs.begin(), m_arcs.end(),
                [backArc](const std::unique_ptr<Arc>& arc) {
                    return arc.get() == backArc;
                }),
            m_arcs.end()
        );
        
        return true;
    }
    
    return false;
}

std::unique_ptr<RETree> DFAToRegex::getRegularExpression() {
    if (m_arcs.empty()) {
        return createEpsilon();
    }
    
    // После removeAllStates должна остаться одна дуга Start→Final
    return std::move(m_arcs.front()->tree);
}

// Вспомогательные функции создания деревьев

std::unique_ptr<RETree> DFAToRegex::createEpsilon() {
    return std::make_unique<RETerminal>(m_grammar, 0); // ID=0 это epsilon
}

std::unique_ptr<RETree> DFAToRegex::createUnaryIteration(std::unique_ptr<RETree> tree) {
    auto epsilon = createEpsilon();
    return REIteration::make(std::move(epsilon), std::move(tree));
}

std::unique_ptr<RETree> DFAToRegex::createAnd(
    std::unique_ptr<RETree> left,
    std::unique_ptr<RETree> right
) {
    return REAnd::make(std::move(left), std::move(right));
}

std::unique_ptr<RETree> DFAToRegex::createOr(
    std::unique_ptr<RETree> left,
    std::unique_ptr<RETree> right
) {
    return REOr::make(std::move(left), std::move(right));
}

std::unique_ptr<RETree> DFAToRegex::createIteration(
    std::unique_ptr<RETree> left,
    std::unique_ptr<RETree> right
) {
    return REIteration::make(std::move(left), std::move(right));
}

} // namespace syngt