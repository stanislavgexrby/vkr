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
    
    converter->m_states.reserve(statesCount + 2);
    for (int i = 0; i < statesCount + 2; ++i) {
        converter->m_states.push_back(i);
    }
    
    converter->m_arcs.push_back(std::make_unique<Arc>(
        0, 2, converter->createEpsilon()
    ));
    
    for (int fromState = 0; fromState < statesCount; ++fromState) {
        if (table->getStateName(fromState) == "FinalState") {
            converter->m_arcs.push_back(std::make_unique<Arc>(
                fromState + 2, 1, converter->createEpsilon()
            ));
            break;
        }
    }
    
    for (int fromState = 0; fromState < statesCount; ++fromState) {
        for (int iSymbol = 0; iSymbol < symbolsCount; ++iSymbol) {
            const StatesSet* statesSet = table->getTableElement(fromState, iSymbol);
            
            if (!statesSet || statesSet->empty()) {
                continue;
            }
            
            std::string symbolName = table->getSymbol(iSymbol);
            std::unique_ptr<RETree> tree;
            
            if (symbolName.size() >= 2 && 
                (symbolName[0] == '"' || symbolName[0] == '\'')) {
                std::string content = symbolName.substr(1, symbolName.size() - 2);
                int termId = grammar->addTerminal(content);
                tree = std::make_unique<RETerminal>(grammar, termId);
            } else {
                int ntId = grammar->findNonTerminal(symbolName);
                if (ntId < 0) {
                    ntId = grammar->addNonTerminal(symbolName);
                }
                tree = std::make_unique<RENonTerminal>(grammar, ntId);
            }
            
            for (State toState : statesSet->getStates()) {
                converter->m_arcs.push_back(std::make_unique<Arc>(
                    fromState + 2,
                    toState + 2,
                    tree->copy()
                ));
            }
        }
    }
    
    return converter;
}

void DFAToRegex::mergeParallelArcs() {
    size_t i = 0;
    while (i < m_arcs.size()) {
        Arc* arc1 = m_arcs[i].get();
        
        for (size_t j = m_arcs.size() - 1; j > i; --j) {
            Arc* arc2 = m_arcs[j].get();
            
            if (arc1->fromState == arc2->fromState &&
                arc1->toState == arc2->toState) {
                arc1->tree = createOr(std::move(arc1->tree), std::move(arc2->tree));
                
                m_arcs.erase(m_arcs.begin() + j);
            }
        }
        
        ++i;
    }
}

void DFAToRegex::removeAllStates() {
    while (m_states.size() > 2) {
        mergeParallelArcs();
        
        size_t bestIndex = findBestStateToRemove();
        removeState(bestIndex);
    }
    
    mergeParallelArcs();
}

void DFAToRegex::removeState(size_t stateIndex) {
    State state = m_states[stateIndex];
    
    std::unique_ptr<RETree> loopTree;
    
    for (const auto& arc : m_arcs) {
        if (arc->fromState == state && arc->toState == state) {
            loopTree = arc->tree->copy();
            break;
        }
    }
    
    std::unique_ptr<RETree> loopIteration;
    if (loopTree) {
        loopIteration = createUnaryIteration(std::move(loopTree));
    } else {
        loopIteration = createEpsilon();
    }
    
    std::vector<std::unique_ptr<Arc>> newArcs;
    
    for (const auto& inArc : m_arcs) {
        if (inArc->toState == state && inArc->fromState != state) {
            
            for (const auto& outArc : m_arcs) {
                if (outArc->fromState == state && outArc->toState != state) {
                    
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
    
    m_arcs.erase(
        std::remove_if(m_arcs.begin(), m_arcs.end(),
            [state](const std::unique_ptr<Arc>& arc) {
                return arc->fromState == state || arc->toState == state;
            }),
        m_arcs.end()
    );
    
    for (auto& arc : newArcs) {
        m_arcs.push_back(std::move(arc));
    }
    
    m_states.erase(m_states.begin() + stateIndex);
}

size_t DFAToRegex::findBestStateToRemove() {
    size_t bestIndex = 2;
    int bestCost = INT_MAX;
    
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
                    needRestart = true;
                    break;
                }
                
                if (cost == 0) {
                    return i;
                }
                
                bestCost = cost;
                bestIndex = i;
            }
            
            if (i == 0) break;
        }
    } while (needRestart);
    
    return bestIndex;
}

int DFAToRegex::getStateRemovalCost(State state) {
    int inCount = 0, outCount = 0;
    int inLength = 0, outLength = 0, loopLength = 0;
    Arc* singleInArc = nullptr;
    
    for (const auto& arc : m_arcs) {
        if (arc->fromState == state) {
            if (arc->toState == state) {
                loopLength += arc->tree->getOperationCount();
            } else {
                ++outCount;
                outLength += arc->tree->getOperationCount();
            }
        } else if (arc->toState == state) {
            ++inCount;
            inLength += arc->tree->getOperationCount();
            singleInArc = arc.get();
        }
    }
    
    if (inCount <= 1 && outCount <= 1) {
        if (inCount == 1 && loopLength == 0 && singleInArc) {
            if (tryOptimizeBinaryIteration(singleInArc)) {
                return -1;
            }
        }
        return 0;
    }
    
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
    
    for (const auto& arc : m_arcs) {
        if (arc->fromState == stateFrom) {
            if (arc->toState != stateTo) {
                foundOtherOut = true;
                break;
            }
        }
        
        if (arc->toState == stateFrom && arc->fromState == stateTo) {
            foundBackArc = true;
            backArc = arc.get();
        }
    }
    
    if (foundBackArc && !foundOtherOut && backArc) {
        inArc->tree = createIteration(
            std::move(inArc->tree),
            backArc->tree->copy()
        );
        
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
    
    return std::move(m_arcs.front()->tree);
}

std::unique_ptr<RETree> DFAToRegex::createEpsilon() {
    return std::make_unique<RETerminal>(m_grammar, 0);
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