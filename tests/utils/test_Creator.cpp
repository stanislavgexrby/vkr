#include <gtest/gtest.h>
#include <syngt/utils/Creator.h>
#include <syngt/core/Grammar.h>
#include <syngt/graphics/DrawObject.h>
#include <syngt/regex/RETree.h>
#include <syngt/regex/RETerminal.h>
#include <syngt/regex/REAnd.h>
#include <syngt/regex/REOr.h>

using namespace syngt;
using namespace syngt::graphics;

class CreatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        grammar = std::make_unique<Grammar>();
        grammar->fillNew();
        list = std::make_unique<DrawObjectList>(grammar.get());
    }
    
    std::unique_ptr<Grammar> grammar;
    std::unique_ptr<DrawObjectList> list;
};

TEST_F(CreatorTest, CreateEmptyDiagram) {
    // Простейшее дерево - один терминал
    int termId = grammar->addTerminal("begin");
    auto tree = std::make_unique<RETerminal>(grammar.get(), termId);
    
    Creator::createDrawObjects(list.get(), tree.get(), grammar.get());
    
    // Должны быть First, Terminal, Last
    EXPECT_GE(list->count(), 2); // Минимум First и Last
    EXPECT_GT(list->width(), 0);
    EXPECT_GT(list->height(), 0);
}

TEST_F(CreatorTest, CreateSequence) {
    // Последовательность: begin , end
    int id1 = grammar->addTerminal("begin");
    int id2 = grammar->addTerminal("end");
    
    auto term1 = std::make_unique<RETerminal>(grammar.get(), id1);
    auto term2 = std::make_unique<RETerminal>(grammar.get(), id2);
    auto seq = REAnd::make(std::move(term1), std::move(term2));
    
    Creator::createDrawObjects(list.get(), seq.get(), grammar.get());
    
    EXPECT_GE(list->count(), 2); // First и Last минимум
    EXPECT_GT(list->width(), 40); // Минимальная ширина для двух элементов
}

TEST_F(CreatorTest, CreateAlternative) {
    // Альтернатива: begin ; end
    int id1 = grammar->addTerminal("begin");
    int id2 = grammar->addTerminal("end");
    
    auto term1 = std::make_unique<RETerminal>(grammar.get(), id1);
    auto term2 = std::make_unique<RETerminal>(grammar.get(), id2);
    auto alt = REOr::make(std::move(term1), std::move(term2));
    
    Creator::createDrawObjects(list.get(), alt.get(), grammar.get());
    
    EXPECT_GE(list->count(), 2);
    EXPECT_GT(list->width(), 0);
    // Альтернатива может иметь большую высоту (идет вниз)
}

TEST_F(CreatorTest, DiagramSizePositive) {
    int termId = grammar->addTerminal("test");
    auto tree = std::make_unique<RETerminal>(grammar.get(), termId);
    
    Creator::createDrawObjects(list.get(), tree.get(), grammar.get());
    
    // Проверка что размеры положительные
    EXPECT_GT(list->width(), 0);
    EXPECT_GT(list->height(), 0);
}

TEST_F(CreatorTest, MultipleCreations) {
    // Создание диаграммы несколько раз - должно очищать предыдущее
    int termId = grammar->addTerminal("test");
    auto tree = std::make_unique<RETerminal>(grammar.get(), termId);
    
    Creator::createDrawObjects(list.get(), tree.get(), grammar.get());
    int count1 = list->count();
    
    Creator::createDrawObjects(list.get(), tree.get(), grammar.get());
    int count2 = list->count();
    
    EXPECT_EQ(count1, count2); // Должно быть одинаково
}

TEST_F(CreatorTest, NullPointerHandling) {
    int termId = grammar->addTerminal("test");
    auto tree = std::make_unique<RETerminal>(grammar.get(), termId);
    
    // Не должно падать с nullptr
    Creator::createDrawObjects(nullptr, tree.get(), grammar.get());
    Creator::createDrawObjects(list.get(), nullptr, grammar.get());
    
    EXPECT_EQ(list->count(), 0); // Список должен остаться пустым
}