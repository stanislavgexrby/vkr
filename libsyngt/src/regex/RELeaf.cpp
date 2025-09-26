#include <syngt/regex/RELeaf.h>
#include <iostream>

namespace syngt {

void RELeaf::save() {
    // TODO: В будущем это будет сохранение в файл
    // Пока выводим ID для отладки
    std::cout << m_id << std::endl;
}

} // namespace syngt