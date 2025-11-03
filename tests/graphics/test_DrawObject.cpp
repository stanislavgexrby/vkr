#include <syngt/graphics/DrawObject.h>
#include <syngt/core/Grammar.h>

using namespace syngt;
using namespace syngt::graphics;

int main() {
    Grammar grammar;
    grammar.fillNew();
    
    DrawObjectList list(&grammar);
    list.initialize();
    
    DrawObject* first = list[0];
    DrawObject* last = list[1];
    
    int termId = grammar.addTerminal("begin");
    auto terminal = std::make_unique<DrawObjectTerminal>(&grammar, termId);
    terminal->setPosition(50, 50);

    int ntId = grammar.addNonTerminal("Program");
    auto nonterminal = std::make_unique<DrawObjectNonTerminal>(&grammar, ntId);
    nonterminal->setPosition(100, 50);

    terminal->setSelected(true);
    
    bool inside = terminal->internalPoint(51, 50);
    
    list.add(std::move(terminal));
    list.add(std::move(nonterminal));

    
    return 0;
}