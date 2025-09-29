#include <syngt/codegen/ParserGenerator.h>
#include <syngt/core/Grammar.h>
#include <syngt/analysis/ParsingTable.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/RESemantic.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REOr.h>
#include <fstream>
#include <sstream>
#include <set>

namespace syngt {

struct ProductionSymbol {
    bool isTerminal;
    int id;
    std::string name;
};

using Production = std::vector<ProductionSymbol>;

// Извлечь список символов из дерева RE (для генерации продукции)
static Production extractProductionFromTree(const RETree* tree, Grammar* grammar) {
    Production result;
    
    if (!tree) return result;
    
    // Терминал
    if (auto term = dynamic_cast<const RETerminal*>(tree)) {
        ProductionSymbol sym;
        sym.isTerminal = true;
        sym.id = term->id();
        sym.name = grammar->terminals()->getString(term->id());
        result.push_back(sym);
        return result;
    }
    
    // Нетерминал
    if (auto nt = dynamic_cast<const RENonTerminal*>(tree)) {
        ProductionSymbol sym;
        sym.isTerminal = false;
        sym.id = nt->id();
        auto nts = grammar->getNonTerminals();
        if (nt->id() >= 0 && nt->id() < static_cast<int>(nts.size())) {
            sym.name = nts[nt->id()];
        }
        result.push_back(sym);
        return result;
    }
    
    // Семантика (epsilon) - пустая продукция
    if (dynamic_cast<const RESemantic*>(tree)) {
        return result;
    }
    
    // And (последовательность) - объединяем символы
    if (auto andNode = dynamic_cast<const REAnd*>(tree)) {
        auto left = extractProductionFromTree(andNode->left(), grammar);
        auto right = extractProductionFromTree(andNode->right(), grammar);
        result.insert(result.end(), left.begin(), left.end());
        result.insert(result.end(), right.begin(), right.end());
        return result;
    }
    
    // Or не должен быть здесь - альтернативы обрабатываются на уровне выше
    
    return result;
}

std::string ParserGenerator::generate(
    Grammar* grammar,
    const ParsingTable* table,
    Language language,
    const std::string& className
) {
    switch (language) {
        case Language::CPP:
            return generateCPP(grammar, table, className);
        case Language::Python:
            return generatePython(grammar, table, className);
        case Language::Java:
        case Language::CSharp:
            return "// Not implemented yet";
        default:
            return "";
    }
}

void ParserGenerator::saveToFile(
    Grammar* grammar,
    const ParsingTable* table,
    Language language,
    const std::string& filename,
    const std::string& className
) {
    std::string code = generate(grammar, table, language, className);
    
    std::ofstream file(filename);
    if (file.is_open()) {
        file << code;
        file.close();
    }
}

std::string ParserGenerator::generateCPP(
    Grammar* grammar,
    const ParsingTable* table,
    const std::string& className
) {
    std::ostringstream out;
    
    // Заголовок
    out << "// Generated LL(1) Predictive Parser\n";
    out << "// DO NOT EDIT - Generated from grammar\n\n";
    out << "#include <string>\n";
    out << "#include <vector>\n";
    out << "#include <stack>\n";
    out << "#include <map>\n";
    out << "#include <stdexcept>\n";
    out << "#include <iostream>\n\n";
    
    // Структура символа
    out << "// Symbol in the grammar\n";
    out << "struct Symbol {\n";
    out << "    bool isTerminal;\n";
    out << "    int id;\n";
    out << "    \n";
    out << "    bool operator==(const Symbol& other) const {\n";
    out << "        return isTerminal == other.isTerminal && id == other.id;\n";
    out << "    }\n";
    out << "    \n";
    out << "    bool operator<(const Symbol& other) const {\n";
    out << "        if (isTerminal != other.isTerminal) return isTerminal < other.isTerminal;\n";
    out << "        return id < other.id;\n";
    out << "    }\n";
    out << "};\n\n";
    
    // Token types enum
    out << "// Token types (terminals)\n";
    out << "enum TokenType {\n";
    auto terms = grammar->terminals()->getItems();
    for (size_t i = 0; i < terms.size(); ++i) {
        out << "    TOKEN_" << terms[i] << " = " << i;
        if (i < terms.size() - 1) out << ",";
        out << "\n";
    }
    out << "};\n\n";
    
    // Non-terminals enum
    out << "// Non-terminal symbols\n";
    out << "enum NonTerminalType {\n";
    auto nts = grammar->getNonTerminals();
    for (size_t i = 0; i < nts.size(); ++i) {
        out << "    NT_" << nts[i] << " = " << i;
        if (i < nts.size() - 1) out << ",";
        out << "\n";
    }
    out << "};\n\n";
    
    // Token структура
    out << "struct Token {\n";
    out << "    TokenType type;\n";
    out << "    std::string value;\n";
    out << "    \n";
    out << "    Token(TokenType t, const std::string& v = \"\") : type(t), value(v) {}\n";
    out << "};\n\n";
    
    // Production typedef
    out << "using Production = std::vector<Symbol>;\n\n";
    
    // Класс парсера
    out << "class " << className << " {\n";
    out << "private:\n";
    
    // Генерация таблицы разбора
    out << generateTableCPP(grammar, table);
    
    // Вспомогательные методы
    out << "    std::string symbolToString(const Symbol& sym) {\n";
    out << "        if (sym.isTerminal) {\n";
    out << "            switch(sym.id) {\n";
    for (size_t i = 0; i < terms.size(); ++i) {
        out << "                case " << i << ": return \"" << terms[i] << "\";\n";
    }
    out << "                default: return \"UNKNOWN_TERMINAL\";\n";
    out << "            }\n";
    out << "        } else {\n";
    out << "            switch(sym.id) {\n";
    for (size_t i = 0; i < nts.size(); ++i) {
        out << "                case " << i << ": return \"" << nts[i] << "\";\n";
    }
    out << "                default: return \"UNKNOWN_NONTERMINAL\";\n";
    out << "            }\n";
    out << "        }\n";
    out << "    }\n\n";
    
    // Public методы
    out << "public:\n";
    out << generateParseMethodCPP(grammar);
    
    out << "};\n";
    
    return out.str();
}

std::string ParserGenerator::generateTableCPP(
    Grammar* grammar,
    const ParsingTable* table
) {
    std::ostringstream out;
    
    auto nts = grammar->getNonTerminals();
    auto terms = grammar->terminals()->getItems();
    
    out << "    // Parsing table M[NonTerminal, Terminal] -> Production\n";
    out << "    std::map<std::pair<int, int>, Production> parsingTable;\n\n";
    
    out << "    void initParsingTable() {\n";
    
    // Для каждого нетерминала
    for (size_t i = 0; i < nts.size(); ++i) {
        const std::string& ntName = nts[i];
        
        // Для каждого терминала
        for (size_t t = 0; t < terms.size(); ++t) {
            const RETree* rule = table->getRule(ntName, static_cast<int>(t));
            if (rule) {
                Production prod = extractProductionFromTree(rule, grammar);
                
                out << "        parsingTable[{" << i << ", " << t << "}] = {";
                for (size_t s = 0; s < prod.size(); ++s) {
                    out << "{" << (prod[s].isTerminal ? "true" : "false") 
                        << ", " << prod[s].id << "}";
                    if (s < prod.size() - 1) out << ", ";
                }
                out << "};\n";
            }
        }
        
        // EOF
        const RETree* eofRule = table->getRule(ntName, -1);
        if (eofRule) {
            Production prod = extractProductionFromTree(eofRule, grammar);
            out << "        parsingTable[{" << i << ", -1}] = {";
            for (size_t s = 0; s < prod.size(); ++s) {
                out << "{" << (prod[s].isTerminal ? "true" : "false") 
                    << ", " << prod[s].id << "}";
                if (s < prod.size() - 1) out << ", ";
            }
            out << "};\n";
        }
    }
    
    out << "    }\n\n";
    
    return out.str();
}

std::string ParserGenerator::generateParseMethodCPP(Grammar* grammar) {
    std::ostringstream out;
    
    auto nts = grammar->getNonTerminals();
    
    int startSymbol = 0;
    for (size_t i = 0; i < nts.size(); ++i) {
        NTListItem* nt = grammar->getNTItem(nts[i]);
        if (nt && nt->hasRoot()) {
            startSymbol = static_cast<int>(i);
            break;
        }
    }
    
    out << "    " << "bool parse(const std::vector<Token>& tokens) {\n";
    out << "        initParsingTable();\n\n";
    
    out << "        std::stack<Symbol> parseStack;\n";
    out << "        size_t tokenPos = 0;\n\n";
    
    out << "        // Push start symbol: " << nts[startSymbol] << "\n";
    out << "        parseStack.push({false, " << startSymbol << "}); \n\n";
    
    out << "        while (!parseStack.empty()) {\n";
    out << "            Symbol top = parseStack.top();\n";
    out << "            parseStack.pop();\n\n";
    
    out << "            if (top.isTerminal) {\n";
    out << "                // Match terminal\n";
    out << "                if (tokenPos >= tokens.size()) {\n";
    out << "                    std::cerr << \"Parse error: unexpected end of input, expected \"\n";
    out << "                              << symbolToString(top) << std::endl;\n";
    out << "                    return false;\n";
    out << "                }\n\n";
    
    out << "                if (tokens[tokenPos].type != top.id) {\n";
    out << "                    std::cerr << \"Parse error: expected \" << symbolToString(top)\n";
    out << "                              << \", got token \" << tokens[tokenPos].value << std::endl;\n";
    out << "                    return false;\n";
    out << "                }\n\n";
    
    out << "                tokenPos++; // Consume token\n";
    out << "            } else {\n";
    out << "                // Non-terminal - look up in parsing table\n";
    out << "                int lookahead = (tokenPos < tokens.size()) ?\n";
    out << "                    tokens[tokenPos].type : -1; // -1 for EOF\n\n";
    
    out << "                auto key = std::make_pair(top.id, lookahead);\n";
    out << "                if (parsingTable.find(key) == parsingTable.end()) {\n";
    out << "                    std::cerr << \"Parse error: no production for \"\n";
    out << "                              << symbolToString(top) << \" with lookahead \";\n";
    out << "                    if (lookahead == -1) {\n";
    out << "                        std::cerr << \"EOF\";\n";
    out << "                    } else if (tokenPos < tokens.size()) {\n";
    out << "                        std::cerr << tokens[tokenPos].value;\n";
    out << "                    }\n";
    out << "                    std::cerr << std::endl;\n";
    out << "                    return false;\n";
    out << "                }\n\n";
    
    out << "                // Apply production (push RHS in reverse order)\n";
    out << "                const Production& prod = parsingTable[key];\n";
    out << "                for (int i = prod.size() - 1; i >= 0; i--) {\n";
    out << "                    parseStack.push(prod[i]);\n";
    out << "                }\n";
    out << "            }\n";
    out << "        }\n\n";
    
    out << "        // Check if all tokens consumed\n";
    out << "        if (tokenPos < tokens.size()) {\n";
    out << "            std::cerr << \"Parse error: extra tokens after parsing\" << std::endl;\n";
    out << "            return false;\n";
    out << "        }\n\n";
    
    out << "        return true;\n";
    out << "    }\n";
    
    return out.str();
}

std::string ParserGenerator::generatePython(
    Grammar* grammar,
    const ParsingTable* table,
    const std::string& className
) {
    std::ostringstream out;
    
    out << "# Generated LL(1) Predictive Parser\n";
    out << "# DO NOT EDIT - Generated from grammar\n\n";
    out << "from typing import List, Tuple, Optional\n";
    out << "from dataclasses import dataclass\n\n";
    
    // Symbol class
    out << "@dataclass\n";
    out << "class Symbol:\n";
    out << "    is_terminal: bool\n";
    out << "    id: int\n\n";
    
    // Token class
    out << "@dataclass\n";
    out << "class Token:\n";
    out << "    type: int\n";
    out << "    value: str = \"\"\n\n";
    
    // Constants
    auto terms = grammar->terminals()->getItems();
    auto nts = grammar->getNonTerminals();
    
    out << "# Terminal types\n";
    for (size_t i = 0; i < terms.size(); ++i) {
        out << "TOKEN_" << terms[i] << " = " << i << "\n";
    }
    out << "EOF_TOKEN = -1\n\n";
    
    out << "# Non-terminal types\n";
    for (size_t i = 0; i < nts.size(); ++i) {
        out << "NT_" << nts[i] << " = " << i << "\n";
    }
    out << "\n";
    
    // Parser class
    out << "class " << className << ":\n";
    out << "    def __init__(self):\n";
    out << generateTablePython(grammar, table);
    out << "\n";
    out << generateParseMethodPython(grammar);
    
    return out.str();
}

std::string ParserGenerator::generateTablePython(
    Grammar* grammar,
    const ParsingTable* table
) {
    std::ostringstream out;
    
    auto nts = grammar->getNonTerminals();
    auto terms = grammar->terminals()->getItems();
    
    out << "        self.table = {}\n";
    
    for (size_t i = 0; i < nts.size(); ++i) {
        const std::string& ntName = nts[i];
        
        for (size_t t = 0; t < terms.size(); ++t) {
            const RETree* rule = table->getRule(ntName, static_cast<int>(t));
            if (rule) {
                Production prod = extractProductionFromTree(rule, grammar);
                out << "        self.table[(" << i << ", " << t << ")] = [";
                for (size_t s = 0; s < prod.size(); ++s) {
                    out << "Symbol(" << (prod[s].isTerminal ? "True" : "False") 
                        << ", " << prod[s].id << ")";
                    if (s < prod.size() - 1) out << ", ";
                }
                out << "]\n";
            }
        }
        
        const RETree* eofRule = table->getRule(ntName, -1);
        if (eofRule) {
            Production prod = extractProductionFromTree(eofRule, grammar);
            out << "        self.table[(" << i << ", -1)] = [";
            for (size_t s = 0; s < prod.size(); ++s) {
                out << "Symbol(" << (prod[s].isTerminal ? "True" : "False") 
                    << ", " << prod[s].id << ")";
                if (s < prod.size() - 1) out << ", ";
            }
            out << "]\n";
        }
    }
    
    return out.str();
}

std::string ParserGenerator::generateParseMethodPython(Grammar* grammar) {
    std::ostringstream out;
    
    auto nts = grammar->getNonTerminals();
    
    int startSymbol = 0;
    for (size_t i = 0; i < nts.size(); ++i) {
        NTListItem* nt = grammar->getNTItem(nts[i]);
        if (nt && nt->hasRoot()) {
            startSymbol = static_cast<int>(i);
            break;
        }
    }
    
    out << "    def parse(self, tokens: List[Token]) -> bool:\n";
    out << "        # Start symbol: " << nts[startSymbol] << "\n";
    out << "        stack = [Symbol(False, " << startSymbol << ")]  \n";
    out << "        pos = 0\n\n";
    
    out << "        while stack:\n";
    out << "            top = stack.pop()\n\n";
    
    out << "            if top.is_terminal:\n";
    out << "                # Match terminal\n";
    out << "                if pos >= len(tokens):\n";
    out << "                    print('Parse error: unexpected end of input')\n";
    out << "                    return False\n\n";
    
    out << "                if tokens[pos].type != top.id:\n";
    out << "                    print(f'Parse error: expected {top.id}, got {tokens[pos].type}')\n";
    out << "                    return False\n\n";
    
    out << "                pos += 1\n";
    out << "            else:\n";
    out << "                # Non-terminal\n";
    out << "                lookahead = tokens[pos].type if pos < len(tokens) else -1\n";
    out << "                key = (top.id, lookahead)\n\n";
    
    out << "                if key not in self.table:\n";
    out << "                    print(f'Parse error: no production for NT={top.id}, lookahead={lookahead}')\n";
    out << "                    return False\n\n";
    
    out << "                # Apply production (push in reverse)\n";
    out << "                production = self.table[key]\n";
    out << "                for symbol in reversed(production):\n";
    out << "                    stack.append(symbol)\n\n";
    
    out << "        return pos >= len(tokens)\n";
    
    return out.str();
}

}