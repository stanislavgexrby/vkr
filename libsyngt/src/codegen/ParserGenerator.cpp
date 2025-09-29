#include <syngt/codegen/ParserGenerator.h>
#include <syngt/core/Grammar.h>
#include <syngt/analysis/ParsingTable.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/RENonTerminal.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REOr.h>
#include <fstream>
#include <sstream>

namespace syngt {

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
            // TODO: реализовать позже
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
    
    out << "// Generated LL(1) Parser\n";
    out << "// DO NOT EDIT - Generated from grammar\n\n";
    out << "#include <string>\n";
    out << "#include <vector>\n";
    out << "#include <stack>\n";
    out << "#include <stdexcept>\n";
    out << "#include <iostream>\n\n";
    
    // Перечисления для символов
    out << "// Token types\n";
    out << "enum class TokenType {\n";
    
    // Терминалы
    auto terms = grammar->terminals()->getItems();
    for (size_t i = 0; i < terms.size(); ++i) {
        out << "    " << terms[i];
        if (i < terms.size() - 1) out << ",";
        out << "\n";
    }
    out << "    END_OF_INPUT\n";
    out << "};\n\n";
    
    // Нетерминалы
    out << "// Non-terminal symbols\n";
    out << "enum class NonTerminal {\n";
    auto nts = grammar->getNonTerminals();
    for (size_t i = 0; i < nts.size(); ++i) {
        out << "    " << nts[i];
        if (i < nts.size() - 1) out << ",";
        out << "\n";
    }
    out << "};\n\n";
    
    // Структура токена
    out << "struct Token {\n";
    out << "    TokenType type;\n";
    out << "    std::string value;\n";
    out << "};\n\n";
    
    // Класс парсера
    out << "class " << className << " {\n";
    out << "private:\n";
    
    // Таблица разбора
    out << generateTableCPP(grammar, table);
    
    // Вспомогательные методы
    out << "    // Convert symbol to string for error messages\n";
    out << "    std::string tokenTypeToString(TokenType t) {\n";
    out << "        switch(t) {\n";
    for (size_t i = 0; i < terms.size(); ++i) {
        out << "            case TokenType::" << terms[i] << ": return \"" << terms[i] << "\";\n";
    }
    out << "            case TokenType::END_OF_INPUT: return \"$\";\n";
    out << "            default: return \"UNKNOWN\";\n";
    out << "        }\n";
    out << "    }\n\n";
    
    out << "    std::string nonTerminalToString(NonTerminal nt) {\n";
    out << "        switch(nt) {\n";
    for (const auto& nt : nts) {
        out << "            case NonTerminal::" << nt << ": return \"" << nt << "\";\n";
    }
    out << "            default: return \"UNKNOWN\";\n";
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
    
    out << "    // Parsing table: M[NonTerminal][TokenType] -> Production\n";
    out << "    // -1 means error, other numbers are production indices\n";
    out << "    int getProduction(NonTerminal nt, TokenType term) {\n";
    out << "        // This is a simplified version - production rules encoded as indices\n";
    out << "        // In full implementation, would return actual production to apply\n";
    
    auto nts = grammar->getNonTerminals();
    auto terms = grammar->terminals()->getItems();
    
    // Генерируем switch по нетерминалам
    out << "        switch(nt) {\n";
    
    for (const auto& ntName : nts) {
        out << "            case NonTerminal::" << ntName << ":\n";
        out << "                switch(term) {\n";
        
        // Для каждого терминала
        for (size_t t = 0; t < terms.size(); ++t) {
            const RETree* rule = table->getRule(ntName, static_cast<int>(t));
            if (rule) {
                out << "                    case TokenType::" << terms[t] << ": return " << t << ";\n";
            }
        }
        
        // EOF
        const RETree* eofRule = table->getRule(ntName, -1);
        if (eofRule) {
            out << "                    case TokenType::END_OF_INPUT: return " << terms.size() << ";\n";
        }
        
        out << "                    default: return -1;\n";
        out << "                }\n";
    }
    
    out << "            default: return -1;\n";
    out << "        }\n";
    out << "    }\n\n";
    
    return out.str();
}

std::string ParserGenerator::generateParseMethodCPP(Grammar* grammar) {
    std::ostringstream out;
    
    auto nts = grammar->getNonTerminals();
    
    out << "    bool parse(const std::vector<Token>& tokens) {\n";
    out << "        std::stack<int> stateStack;  // Simplified: using symbol indices\n";
    out << "        size_t tokenIndex = 0;\n\n";
    
    out << "        // Push start symbol\n";
    out << "        stateStack.push(0);  // " << nts[0] << "\n\n";
    
    out << "        while (!stateStack.empty()) {\n";
    out << "            int top = stateStack.top();\n";
    out << "            stateStack.pop();\n\n";
    
    out << "            TokenType currentToken = (tokenIndex < tokens.size()) ?\n";
    out << "                tokens[tokenIndex].type : TokenType::END_OF_INPUT;\n\n";
    
    out << "            // Check if top is non-terminal\n";
    out << "            if (top < " << nts.size() << ") {\n";
    out << "                NonTerminal nt = static_cast<NonTerminal>(top);\n";
    out << "                int production = getProduction(nt, currentToken);\n\n";
    
    out << "                if (production == -1) {\n";
    out << "                    std::cerr << \"Parse error at token: \"\n";
    out << "                              << tokenTypeToString(currentToken) << std::endl;\n";
    out << "                    return false;\n";
    out << "                }\n\n";
    
    out << "                // Apply production (push right-hand side in reverse)\n";
    out << "                // Simplified implementation\n";
    out << "            } else {\n";
    out << "                // Top is terminal - match with current token\n";
    out << "                if (tokenIndex >= tokens.size()) {\n";
    out << "                    std::cerr << \"Unexpected end of input\" << std::endl;\n";
    out << "                    return false;\n";
    out << "                }\n\n";
    
    out << "                // Match terminal\n";
    out << "                tokenIndex++;\n";
    out << "            }\n";
    out << "        }\n\n";
    
    out << "        return tokenIndex >= tokens.size();\n";
    out << "    }\n";
    
    return out.str();
}

std::string ParserGenerator::generatePython(
    Grammar* grammar,
    const ParsingTable* table,
    const std::string& className
) {
    std::ostringstream out;
    
    out << "# Generated LL(1) Parser\n";
    out << "# DO NOT EDIT - Generated from grammar\n\n";
    out << "from enum import Enum\n";
    out << "from typing import List, Tuple\n\n";
    
    // Token types
    out << "class TokenType(Enum):\n";
    auto terms = grammar->terminals()->getItems();
    for (size_t i = 0; i < terms.size(); ++i) {
        out << "    " << terms[i] << " = " << i << "\n";
    }
    out << "    END_OF_INPUT = " << terms.size() << "\n\n";
    
    // Non-terminals
    out << "class NonTerminal(Enum):\n";
    auto nts = grammar->getNonTerminals();
    for (size_t i = 0; i < nts.size(); ++i) {
        out << "    " << nts[i] << " = " << i << "\n";
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
    
    out << "        # Parsing table\n";
    out << "        self.table = {\n";
    
    for (const auto& ntName : nts) {
        out << "            NonTerminal." << ntName << ": {\n";
        
        for (size_t t = 0; t < terms.size(); ++t) {
            const RETree* rule = table->getRule(ntName, static_cast<int>(t));
            if (rule) {
                out << "                TokenType." << terms[t] << ": " << t << ",\n";
            }
        }
        
        const RETree* eofRule = table->getRule(ntName, -1);
        if (eofRule) {
            out << "                TokenType.END_OF_INPUT: " << terms.size() << ",\n";
        }
        
        out << "            },\n";
    }
    
    out << "        }\n";
    
    return out.str();
}

std::string ParserGenerator::generateParseMethodPython(Grammar* grammar) {
    std::ostringstream out;
    
    auto nts = grammar->getNonTerminals();
    
    out << "    def parse(self, tokens: List[Tuple[TokenType, str]]) -> bool:\n";
    out << "        stack = [NonTerminal." << nts[0] << "]\n";
    out << "        token_index = 0\n\n";
    
    out << "        while stack:\n";
    out << "            top = stack.pop()\n";
    out << "            current_token = tokens[token_index][0] if token_index < len(tokens) else TokenType.END_OF_INPUT\n\n";
    
    out << "            if isinstance(top, NonTerminal):\n";
    out << "                if top not in self.table or current_token not in self.table[top]:\n";
    out << "                    print(f'Parse error at {current_token}')\n";
    out << "                    return False\n\n";
    
    out << "                production = self.table[top][current_token]\n";
    out << "                # Apply production (push RHS in reverse)\n";
    out << "                # Simplified implementation\n";
    out << "            else:\n";
    out << "                # Match terminal\n";
    out << "                if token_index >= len(tokens):\n";
    out << "                    print('Unexpected end of input')\n";
    out << "                    return False\n";
    out << "                token_index += 1\n\n";
    
    out << "        return token_index >= len(tokens)\n";
    
    return out.str();
}

}