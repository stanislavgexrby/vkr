#include <gtest/gtest.h>
#include <syngt/utils/Semantic.h>
#include <sstream>

using namespace syngt;

TEST(SemanticIDListTest, DefaultConstruction) {
    SemanticIDList list;
    EXPECT_EQ(list.count(), 0);
    EXPECT_TRUE(list.isEmpty());
}

TEST(SemanticIDListTest, AddElements) {
    SemanticIDList list;
    
    list.add(1);
    list.add(2);
    list.add(3);
    
    EXPECT_EQ(list.count(), 3);
    EXPECT_FALSE(list.isEmpty());
    EXPECT_EQ(list.getID(0), 1);
    EXPECT_EQ(list.getID(1), 2);
    EXPECT_EQ(list.getID(2), 3);
}

TEST(SemanticIDListTest, Clear) {
    SemanticIDList list;
    list.add(1);
    list.add(2);
    
    EXPECT_EQ(list.count(), 2);
    
    list.clear();
    
    EXPECT_EQ(list.count(), 0);
    EXPECT_TRUE(list.isEmpty());
}

TEST(SemanticIDListTest, GetLength) {
    SemanticIDList list;
    
    EXPECT_EQ(list.getLength(), 0);
    
    list.add(1);
    EXPECT_EQ(list.getLength(), 20);
    
    list.add(2);
    EXPECT_EQ(list.getLength(), 40);
    
    list.add(3);
    EXPECT_EQ(list.getLength(), 60);
}

TEST(SemanticIDListTest, Copy) {
    SemanticIDList list;
    list.add(10);
    list.add(20);
    list.add(30);
    
    auto copied = list.copy();
    
    ASSERT_NE(copied, nullptr);
    EXPECT_EQ(copied->count(), 3);
    EXPECT_EQ(copied->getID(0), 10);
    EXPECT_EQ(copied->getID(1), 20);
    EXPECT_EQ(copied->getID(2), 30);
    
    list.add(40);
    EXPECT_EQ(list.count(), 4);
    EXPECT_EQ(copied->count(), 3);
}

TEST(SemanticIDListTest, GetItems) {
    SemanticIDList list;
    list.add(5);
    list.add(10);
    list.add(15);
    
    const auto& items = list.getItems();
    
    EXPECT_EQ(items.size(), 3);
    EXPECT_EQ(items[0], 5);
    EXPECT_EQ(items[1], 10);
    EXPECT_EQ(items[2], 15);
}

TEST(SemanticIDListTest, SaveLoad) {
    SemanticIDList list;
    list.add(100);
    list.add(200);
    list.add(300);
    
    std::stringstream buffer;
    std::streambuf* oldCout = std::cout.rdbuf(buffer.rdbuf());
    
    list.save();
    
    std::cout.rdbuf(oldCout);
    
    std::string output = buffer.str();
    EXPECT_EQ(output, "3\n100\n200\n300\n");
    
    SemanticIDList list2;
    std::streambuf* oldCin = std::cin.rdbuf(buffer.rdbuf());
    
    list2.load();
    
    std::cin.rdbuf(oldCin);
    
    EXPECT_EQ(list2.count(), 3);
    EXPECT_EQ(list2.getID(0), 100);
    EXPECT_EQ(list2.getID(1), 200);
    EXPECT_EQ(list2.getID(2), 300);
}

TEST(SemanticIDListTest, MultipleAdds) {
    SemanticIDList list;
    
    for (int i = 0; i < 10; ++i) {
        list.add(i * 10);
    }
    
    EXPECT_EQ(list.count(), 10);
    EXPECT_EQ(list.getLength(), 200);
    
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(list.getID(i), i * 10);
    }
}