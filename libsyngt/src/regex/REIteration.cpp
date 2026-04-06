#include <syngt/regex/REIteration.h>
#include <syngt/regex/RELeaf.h>

namespace syngt {

// Returns true if node serializes as "eps" — i.e., it is the epsilon terminal.
// Using toString() is the only reliable way: id==0 is not sufficient because
// in unit tests Grammar may be constructed without fillNew(), making id==0
// map to an ordinary terminal (e.g. 'a') rather than epsilon.
static bool isEpsilon(const RETree* node) {
    if (!node) return false;
    SelectionMask empty;
    return node->toString(empty, false) == "eps";
}

// Serialize REIteration in RBNF notation so that grammar->save() round-trips
// correctly through the RBNF parser (Parser.cpp):
//
//   REIteration(eps, B)  -> @*(B)   — Kleene star
//   REIteration(A, eps)  -> @+(A)   — positive iteration
//   REIteration(A, B)    -> A#B     — binary iteration
//
// Compound operands are wrapped in parentheses to preserve precedence.
std::string REIteration::toString(const SelectionMask& mask, bool reverse) const {
    if (!m_first || !m_second) {
        return "";
    }

    auto wrap = [&](const RETree* node) -> std::string {
        std::string s = node->toString(mask, reverse);
        if (dynamic_cast<const REBinaryOp*>(node)) {
            return '(' + s + ')';
        }
        return s;
    };

    if (isEpsilon(m_first.get())) {
        // @*(B)
        return "@*" + wrap(m_second.get());
    }

    if (isEpsilon(m_second.get())) {
        // @+(A)
        return "@+" + wrap(m_first.get());
    }

    // A#B — general binary iteration
    return wrap(m_first.get()) + '#' + wrap(m_second.get());
}

} // namespace syngt
