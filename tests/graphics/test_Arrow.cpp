#include <gtest/gtest.h>
#include <syngt/graphics/Arrow.h>
#include <syngt/graphics/DrawPoint.h>
#include <sstream>

using namespace syngt::graphics;

// Простая реализация DrawPoint для тестов
class TestDrawPoint : public DrawPoint {
public:
    int getLength() const override { return 50; }
};

TEST(ArrowTest, DefaultConstructor) {
    Arrow arrow;
    
    EXPECT_EQ(arrow.getLength(), MinArrowLength);
    EXPECT_EQ(arrow.getFromDO(), nullptr);
    EXPECT_EQ(arrow.ward(), cwNONE);
}

TEST(ArrowTest, ConstructorWithParameters) {
    TestDrawPoint point;
    Arrow arrow(cwFORWARD, &point);
    
    EXPECT_EQ(arrow.ward(), cwFORWARD);
    EXPECT_EQ(arrow.getFromDO(), &point);
    EXPECT_EQ(arrow.getLength(), MinArrowLength + SpikeLength);
}

TEST(ArrowTest, ConstructorWithNoneWard) {
    TestDrawPoint point;
    Arrow arrow(cwNONE, &point);
    
    EXPECT_EQ(arrow.ward(), cwNONE);
    EXPECT_EQ(arrow.getLength(), MinArrowLength);
}

TEST(ArrowTest, SetFromDO) {
    Arrow arrow;
    TestDrawPoint point;
    
    arrow.setFromDO(&point);
    EXPECT_EQ(arrow.getFromDO(), &point);
}

TEST(ArrowTest, SetWard) {
    Arrow arrow;
    
    arrow.setWard(cwBACKWARD);
    EXPECT_EQ(arrow.ward(), cwBACKWARD);
}

TEST(ArrowTest, Copy) {
    TestDrawPoint point;
    Arrow arrow(cwFORWARD, &point);
    
    auto copied = arrow.copy();
    
    ASSERT_NE(copied, nullptr);
    EXPECT_EQ(copied->ward(), cwFORWARD);
    EXPECT_EQ(copied->getFromDO(), &point);
    EXPECT_EQ(copied->getLength(), MinArrowLength + SpikeLength);
}

TEST(ArrowTest, Save) {
    TestDrawPoint point;
    Arrow arrow(cwFORWARD, &point);
    
    std::stringstream buffer;
    std::streambuf* oldCout = std::cout.rdbuf(buffer.rdbuf());
    
    DrawPoint* result = arrow.save();
    
    std::cout.rdbuf(oldCout);
    
    EXPECT_EQ(result, &point);
    
    std::string output = buffer.str();
    EXPECT_EQ(output, "0\n1\n"); // ctArrow=0, cwFORWARD=1
}

// SemanticArrow tests

TEST(SemanticArrowTest, Constructor) {
    TestDrawPoint point;
    auto semantics = std::make_unique<syngt::SemanticIDList>();
    semantics->add(1);
    semantics->add(2);
    
    int semanticsLength = semantics->getLength();
    
    SemanticArrow arrow(cwFORWARD, &point, std::move(semantics));
    
    EXPECT_EQ(arrow.ward(), cwFORWARD);
    EXPECT_EQ(arrow.getFromDO(), &point);
    EXPECT_NE(arrow.getSemantics(), nullptr);
    EXPECT_EQ(arrow.getLength(), MinArrowLength + semanticsLength + SpikeLength);
}

TEST(SemanticArrowTest, GetSemantics) {
    TestDrawPoint point;
    auto semantics = std::make_unique<syngt::SemanticIDList>();
    semantics->add(10);
    semantics->add(20);
    
    SemanticArrow arrow(cwNONE, &point, std::move(semantics));
    
    auto* sems = arrow.getSemantics();
    ASSERT_NE(sems, nullptr);
    EXPECT_EQ(sems->count(), 2);
    EXPECT_EQ(sems->getID(0), 10);
    EXPECT_EQ(sems->getID(1), 20);
}

TEST(SemanticArrowTest, Copy) {
    TestDrawPoint point;
    auto semantics = std::make_unique<syngt::SemanticIDList>();
    semantics->add(100);
    semantics->add(200);
    
    SemanticArrow arrow(cwBACKWARD, &point, std::move(semantics));
    
    auto copied = arrow.copy();
    
    ASSERT_NE(copied, nullptr);
    
    // Проверяем, что это SemanticArrow
    SemanticArrow* semCopied = dynamic_cast<SemanticArrow*>(copied.get());
    ASSERT_NE(semCopied, nullptr);
    
    EXPECT_EQ(semCopied->ward(), cwBACKWARD);
    EXPECT_EQ(semCopied->getFromDO(), &point);
    
    auto* copiedSems = semCopied->getSemantics();
    ASSERT_NE(copiedSems, nullptr);
    EXPECT_EQ(copiedSems->count(), 2);
    EXPECT_EQ(copiedSems->getID(0), 100);
    EXPECT_EQ(copiedSems->getID(1), 200);
}

TEST(SemanticArrowTest, Save) {
    TestDrawPoint point;
    auto semantics = std::make_unique<syngt::SemanticIDList>();
    semantics->add(5);
    semantics->add(10);
    
    SemanticArrow arrow(cwFORWARD, &point, std::move(semantics));
    
    std::stringstream buffer;
    std::streambuf* oldCout = std::cout.rdbuf(buffer.rdbuf());
    
    DrawPoint* result = arrow.save();
    
    std::cout.rdbuf(oldCout);
    
    EXPECT_EQ(result, &point);
    
    std::string output = buffer.str();
    // ctSemanticArrow=1, cwFORWARD=1, затем semantics save (2\n5\n10\n)
    EXPECT_EQ(output, "1\n1\n2\n5\n10\n");
}

TEST(SemanticArrowTest, NullSemantics) {
    TestDrawPoint point;
    SemanticArrow arrow(cwNONE, &point, nullptr);
    
    EXPECT_EQ(arrow.getSemantics(), nullptr);
    EXPECT_EQ(arrow.getLength(), MinArrowLength);
}