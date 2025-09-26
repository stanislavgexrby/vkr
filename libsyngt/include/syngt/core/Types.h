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

inline SelectionMask EmptyMask() {
    return SelectionMask();
}

}