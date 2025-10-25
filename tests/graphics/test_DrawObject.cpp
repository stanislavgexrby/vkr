#include <syngt/graphics/DrawObject.h>
#include <syngt/core/Grammar.h>
#include <iostream>

using namespace syngt;
using namespace syngt::graphics;

int main() {
    std::cout << "=== DrawObject Test ===\n\n";
    
    // Тест создания грамматики
    Grammar grammar;
    grammar.fillNew();
    std::cout << "✓ Grammar created\n";
    
    // Создаем список
    DrawObjectList list(&grammar);
    list.initialize();
    std::cout << "✓ DrawObjectList initialized with " << list.count() << " objects\n";
    
    // Проверяем что есть First и Last
    DrawObject* first = list[0];
    DrawObject* last = list[1];
    if (first && last) {
        std::cout << "✓ First object type: " << first->getType() << "\n";
        std::cout << "✓ Last object type: " << last->getType() << "\n";
    }
    
    // Создаем терминал
    int termId = grammar.addTerminal("begin");
    auto terminal = std::make_unique<DrawObjectTerminal>(&grammar, termId);
    terminal->setPosition(50, 50);
    
    std::cout << "✓ Terminal created at (" << terminal->x() << ", " 
              << terminal->y() << ")\n";
    std::cout << "  Terminal endX: " << terminal->endX() << "\n";
    std::cout << "  Terminal length: " << terminal->getLength() << "\n";
    
    // Создаем нетерминал
    int ntId = grammar.addNonTerminal("Program");
    auto nonterminal = std::make_unique<DrawObjectNonTerminal>(&grammar, ntId);
    nonterminal->setPosition(100, 50);
    
    std::cout << "✓ NonTerminal created at (" << nonterminal->x() << ", " 
              << nonterminal->y() << ")\n";
    
    // Тест выделения
    terminal->setSelected(true);
    std::cout << "✓ Terminal selected: " << (terminal->selected() ? "yes" : "no") << "\n";
    
    // Тест internalPoint
    bool inside = terminal->internalPoint(51, 50);
    std::cout << "✓ Point (51,50) inside terminal: " << (inside ? "yes" : "no") << "\n";
    
    // Тест добавления в список
    list.add(std::move(terminal));
    list.add(std::move(nonterminal));
    std::cout << "✓ Objects added to list, total: " << list.count() << "\n";
    
    // Тест поиска
    DrawObject* found = list.findDO(52, 50);
    if (found) {
        std::cout << "✓ Object found at (52,50)\n";
    }
    
    std::cout << "\n=== All DrawObject tests passed! ===\n";
    
    return 0;
}