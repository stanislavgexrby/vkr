#include <syngt/analysis/ParsingTable.h>
#include <syngt/core/Grammar.h>
#include <syngt/core/NTListItem.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/regex/REOr.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REIteration.h>
#include <syngt/transform/FirstFollow.h>
#include <iostream>
#include <iomanip>
#include <functional>

namespace syngt {

static bool isAlternativeNullable(
    const RETree* tree,
    const std::map<std::string, bool>& nullable
) {
    if (!tree) return true;
    
    if (dynamic_cast<const RESemantic*>(tree)) return true;
    if (dynamic_cast<const RETerminal*>(tree)) return false;
    if (dynamic_cast<const REIteration*>(tree)) return true;
    
    if (auto nt = dynamic_cast<const RENonTerminal*>(tree)) {
        if (nt->grammar()) {
            auto nts = nt->grammar()->getNonTerminals();
            int id = nt->id();
            if (id >= 0 && id < static_cast<int>(nts.size())) {
                auto it = nullable.find(nts[id]);
                return it != nullable.end() && it->second;
            }
        }
        return false;
    }
    
    if (auto orNode = dynamic_cast<const REOr*>(tree)) {
        return isAlternativeNullable(orNode->left(), nullable) ||
               isAlternativeNullable(orNode->right(), nullable);
    }
    
    if (auto andNode = dynamic_cast<const REAnd*>(tree)) {
        return isAlternativeNullable(andNode->left(), nullable) &&
               isAlternativeNullable(andNode->right(), nullable);
    }
    
    return false;
}

static std::set<int> computeFirstForAlternative(
    const RETree* tree,
    const std::map<std::string, std::set<int>>& firstSets,
    const std::map<std::string, bool>& nullable
) {
    std::set<int> result;
    if (!tree) return result;
    
    if (auto term = dynamic_cast<const RETerminal*>(tree)) {
        result.insert(term->id());
        return result;
    }
    
    if (dynamic_cast<const RESemantic*>(tree)) {
        return result;  // Epsilon
    }
    
    if (auto nt = dynamic_cast<const RENonTerminal*>(tree)) {
        if (nt->grammar()) {
            auto nts = nt->grammar()->getNonTerminals();
            int id = nt->id();
            if (id >= 0 && id < static_cast<int>(nts.size())) {
                auto it = firstSets.find(nts[id]);
                if (it != firstSets.end()) {
                    result = it->second;
                }
            }
        }
        return result;
    }
    
    if (auto orNode = dynamic_cast<const REOr*>(tree)) {
        auto left = computeFirstForAlternative(orNode->left(), firstSets, nullable);
        auto right = computeFirstForAlternative(orNode->right(), firstSets, nullable);
        result.insert(left.begin(), left.end());
        result.insert(right.begin(), right.end());
        return result;
    }
    
    if (auto andNode = dynamic_cast<const REAnd*>(tree)) {
        auto left = computeFirstForAlternative(andNode->left(), firstSets, nullable);
        result.insert(left.begin(), left.end());
        
        if (isAlternativeNullable(andNode->left(), nullable)) {
            auto right = computeFirstForAlternative(andNode->right(), firstSets, nullable);
            result.insert(right.begin(), right.end());
        }
        return result;
    }
    
    if (auto iterNode = dynamic_cast<const REIteration*>(tree)) {
        return computeFirstForAlternative(iterNode->left(), firstSets, nullable);
    }
    
    return result;
}

std::unique_ptr<ParsingTable> ParsingTable::build(Grammar* grammar) {
    if (!grammar) return nullptr;
    
    auto table = std::unique_ptr<ParsingTable>(new ParsingTable());
    table->m_grammar = grammar;
    
    auto firstSets = FirstFollow::computeFirst(grammar);
    auto followSets = FirstFollow::computeFollow(grammar, firstSets);
    
    std::map<std::string, bool> nullable;
    auto nts = grammar->getNonTerminals();
    
    for (const auto& nt : nts) {
        nullable[nt] = false;
    }
    
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& ntName : nts) {
            if (nullable[ntName]) continue;
            
            NTListItem* nt = grammar->getNTItem(ntName);
            if (!nt || !nt->hasRoot()) continue;
            
            if (isAlternativeNullable(nt->root(), nullable)) {
                nullable[ntName] = true;
                changed = true;
            }
        }
    }
    
    for (const auto& ntName : nts) {
        NTListItem* nt = grammar->getNTItem(ntName);
        if (!nt || !nt->hasRoot()) continue;
        
        std::vector<const RETree*> alternatives;
        std::function<void(const RETree*)> collectAlts = [&](const RETree* tree) {
            if (!tree) return;
            if (auto orNode = dynamic_cast<const REOr*>(tree)) {
                collectAlts(orNode->left());
                collectAlts(orNode->right());
            } else {
                alternatives.push_back(tree);
            }
        };
        collectAlts(nt->root());
        
        for (const auto* alt : alternatives) {
            table->processAlternative(ntName, alt, firstSets, followSets, nullable);
        }
    }
    
    return table;
}

void ParsingTable::processAlternative(
    const std::string& ntName,
    const RETree* alternative,
    const std::map<std::string, std::set<int>>& firstSets,
    const std::map<std::string, std::set<int>>& followSets,
    const std::map<std::string, bool>& nullable
) {
    auto firstAlpha = computeFirstForAlternative(alternative, firstSets, nullable);
    
    for (int termId : firstAlpha) {
        addRule(ntName, termId, alternative);
    }
    
    if (isAlternativeNullable(alternative, nullable)) {
        auto followIt = followSets.find(ntName);
        if (followIt != followSets.end()) {
            for (int termId : followIt->second) {
                addRule(ntName, termId, alternative);
            }
        }
    }
}

void ParsingTable::addRule(
    const std::string& nonTerminal,
    int terminal,
    const RETree* rule
) {
    TableKey key = {nonTerminal, terminal};
    
    if (m_table.count(key) > 0 && m_table[key] != rule) {
        std::string conflict = "Conflict at M[" + nonTerminal + ", ";
        if (terminal == -1) {
            conflict += "$";
        } else if (m_grammar) {
            conflict += m_grammar->terminals()->getString(terminal);
        }
        conflict += "]";
        m_conflicts.push_back(conflict);
    }
    
    m_table[key] = rule;
}

const RETree* ParsingTable::getRule(const std::string& nonTerminal, int terminal) const {
    TableKey key = {nonTerminal, terminal};
    auto it = m_table.find(key);
    return it != m_table.end() ? it->second : nullptr;
}

bool ParsingTable::hasRule(const std::string& nonTerminal, int terminal) const {
    TableKey key = {nonTerminal, terminal};
    return m_table.count(key) > 0;
}

void ParsingTable::print(Grammar* grammar) const {
    if (!grammar) return;
    
    auto nts = grammar->getNonTerminals();
    int termCount = grammar->terminals()->getCount();
    
    std::cout << "\n=== LL(1) Parsing Table ===\n\n";
    
    std::cout << std::setw(12) << " ";
    for (int t = 0; t < termCount; ++t) {
        std::cout << std::setw(15) << grammar->terminals()->getString(t);
    }
    std::cout << std::setw(15) << "$" << "\n";
    std::cout << std::string(12 + (termCount + 1) * 15, '-') << "\n";
    
    for (const auto& nt : nts) {
        std::cout << std::setw(12) << nt;
        
        for (int t = 0; t < termCount; ++t) {
            const RETree* rule = getRule(nt, t);
            if (rule) {
                SelectionMask mask;
                std::string ruleStr = rule->toString(mask, false);
                if (ruleStr.length() > 14) {
                    ruleStr = ruleStr.substr(0, 11) + "...";
                }
                std::cout << std::setw(15) << ruleStr;
            } else {
                std::cout << std::setw(15) << "-";
            }
        }
        
        // EOF ($)
        const RETree* rule = getRule(nt, -1);
        if (rule) {
            SelectionMask mask;
            std::string ruleStr = rule->toString(mask, false);
            if (ruleStr.length() > 14) {
                ruleStr = ruleStr.substr(0, 11) + "...";
            }
            std::cout << std::setw(15) << ruleStr;
        } else {
            std::cout << std::setw(15) << "-";
        }
        
        std::cout << "\n";
    }
    
    if (hasConflicts()) {
        std::cout << "\n=== CONFLICTS ===\n";
        for (const auto& conflict : m_conflicts) {
            std::cout << "  " << conflict << "\n";
        }
    }
}

std::string ParsingTable::exportForCodegen(Grammar* grammar) const {
    if (!grammar) return "";
    
    std::string result;
    result += "// LL(1) Parsing Table\n";
    result += "// Generated from grammar\n\n";
    
    auto nts = grammar->getNonTerminals();
    int termCount = grammar->terminals()->getCount();
    
    result += "const ParsingTable table = {\n";
    
    for (const auto& nt : nts) {
        for (int t = -1; t < termCount; ++t) {
            const RETree* rule = getRule(nt, t);
            if (rule) {
                result += "  {\"" + nt + "\", ";
                if (t == -1) {
                    result += "EOF";
                } else {
                    result += "\"" + grammar->terminals()->getString(t) + "\"";
                }
                
                SelectionMask mask;
                result += ", \"" + rule->toString(mask, false) + "\"},\n";
            }
        }
    }
    
    result += "};\n";
    
    return result;
}

}