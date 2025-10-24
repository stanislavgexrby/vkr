#include <gtest/gtest.h>
#include <syngt/graphics/DrawPoint.h>

using namespace syngt::graphics;

class TestDrawPoint : public DrawPoint {
private:
    int m_length = 100; // Фиксированная длина для теста
    
public:
    explicit TestDrawPoint(int length = 100) : m_length(length) {}
    
    int getLength() const override {
        return m_length;
    }
    
    void setLength(int length) {
        m_length = length;
    }
};

class DrawPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        point = std::make_unique<TestDrawPoint>(100);
    }
    
    std::unique_ptr<TestDrawPoint> point;
};

TEST_F(DrawPointTest, InitialCoordinates) {
    EXPECT_EQ(point->x(), 0);
    EXPECT_EQ(point->y(), 0);
    EXPECT_EQ(point->endX(), 100); // 0 + length(100)
    EXPECT_EQ(point->endY(), 0);
}

TEST_F(DrawPointTest, SetPosition) {
    point->setPosition(50, 75);
    
    EXPECT_EQ(point->x(), 50);
    EXPECT_EQ(point->y(), 75);
    EXPECT_EQ(point->endX(), 150); // 50 + 100
    EXPECT_EQ(point->endY(), 75);
}

TEST_F(DrawPointTest, SetXCoord) {
    point->setXCoord(200);
    
    EXPECT_EQ(point->x(), 200);
    EXPECT_EQ(point->y(), 0);
    EXPECT_EQ(point->endX(), 300); // 200 + 100
}

TEST_F(DrawPointTest, SetYCoord) {
    point->setYCoord(150);
    
    EXPECT_EQ(point->x(), 0);
    EXPECT_EQ(point->y(), 150);
    EXPECT_EQ(point->endY(), 150);
}

TEST_F(DrawPointTest, Move) {
    point->setPosition(100, 200);
    point->move(50, 30);
    
    EXPECT_EQ(point->x(), 150); // 100 + 50
    EXPECT_EQ(point->y(), 230); // 200 + 30
    EXPECT_EQ(point->endX(), 250); // 150 + 100
}

TEST_F(DrawPointTest, MoveNegative) {
    point->setPosition(100, 200);
    point->move(-25, -40);
    
    EXPECT_EQ(point->x(), 75);  // 100 - 25
    EXPECT_EQ(point->y(), 160); // 200 - 40
}

TEST_F(DrawPointTest, DifferentLengths) {
    auto shortPoint = std::make_unique<TestDrawPoint>(50);
    auto longPoint = std::make_unique<TestDrawPoint>(200);
    
    shortPoint->setPosition(0, 0);
    longPoint->setPosition(0, 0);
    
    EXPECT_EQ(shortPoint->endX(), 50);  // 0 + 50
    EXPECT_EQ(longPoint->endX(), 200);  // 0 + 200
}

TEST_F(DrawPointTest, VirtualSetters) {
    // Проверяем, что виртуальные методы вызываются правильно
    point->setXCoord(10);
    point->setYCoord(20);
    
    EXPECT_EQ(point->x(), 10);
    EXPECT_EQ(point->y(), 20);
}

// Пример теста для переопределения виртуальных методов
class DrawPointWithNotification : public DrawPoint {
private:
    int m_length = 50;
    bool m_xChanged = false;
    bool m_yChanged = false;
    
protected:
    void setX(int x) override {
        DrawPoint::setX(x);
        m_xChanged = true;
    }
    
    void setY(int y) override {
        DrawPoint::setY(y);
        m_yChanged = true;
    }
    
public:
    int getLength() const override { return m_length; }
    bool wasXChanged() const { return m_xChanged; }
    bool wasYChanged() const { return m_yChanged; }
};

TEST(DrawPointVirtualTest, SettersCanBeOverridden) {
    DrawPointWithNotification point;
    
    EXPECT_FALSE(point.wasXChanged());
    EXPECT_FALSE(point.wasYChanged());
    
    point.setXCoord(100);
    EXPECT_TRUE(point.wasXChanged());
    EXPECT_FALSE(point.wasYChanged());
    
    point.setYCoord(200);
    EXPECT_TRUE(point.wasYChanged());
}

// Тест для проверки последовательности операций
TEST(DrawPointComplexTest, ChainedOperations) {
    TestDrawPoint point(80);
    
    // Последовательность операций как в реальном использовании
    point.setPosition(100, 150);
    EXPECT_EQ(point.x(), 100);
    EXPECT_EQ(point.y(), 150);
    
    point.move(20, 30);
    EXPECT_EQ(point.x(), 120);
    EXPECT_EQ(point.y(), 180);
    
    point.setXCoord(200);
    EXPECT_EQ(point.x(), 200);
    EXPECT_EQ(point.y(), 180); // Y не изменился
    
    EXPECT_EQ(point.endX(), 280); // 200 + 80
}