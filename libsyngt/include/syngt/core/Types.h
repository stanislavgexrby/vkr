#pragma once
#include <vector>
#include <string>

namespace syngt {

using SelectionMask = std::vector<int>;

enum MarkType {
    cmNotMarked = 0,
    cmMarked = 1,
    cmOpenMacro = 2
};

constexpr int cgTerminalList = 0;
constexpr int cgSemanticList = 1;
constexpr int cgNonTerminalList = 2;
constexpr int cgMacroList = 3;

inline SelectionMask EmptyMask() {
    return SelectionMask();
}

}