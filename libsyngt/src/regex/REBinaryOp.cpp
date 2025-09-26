#include <syngt/regex/REBinaryOp.h>
#include <iostream>

namespace syngt {

void REBinaryOp::save() {
    std::cout << static_cast<int>(getOperationChar()) << std::endl;
    
    if (m_first) {
        m_first->save();
    }
    if (m_second) {
        m_second->save();
    }
}

}