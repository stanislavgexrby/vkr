#include <gtest/gtest.h>
#include <syngt/parser/CharProducer.h>

using namespace syngt;

TEST(CharProducerTest, InitialChar) {
    CharProducer cp("abc");
    EXPECT_EQ(cp.currentChar(), 'a');
    EXPECT_EQ(cp.index(), 0u);
    EXPECT_FALSE(cp.isEnd());
}

TEST(CharProducerTest, AdvanceNext) {
    CharProducer cp("abc");
    EXPECT_TRUE(cp.next());
    EXPECT_EQ(cp.currentChar(), 'b');
    EXPECT_EQ(cp.index(), 1u);

    EXPECT_TRUE(cp.next());
    EXPECT_EQ(cp.currentChar(), 'c');
    EXPECT_EQ(cp.index(), 2u);
}

TEST(CharProducerTest, EndDetection) {
    CharProducer cp("ab");
    cp.next(); // 'b'
    EXPECT_FALSE(cp.isEnd());

    cp.next(); // past end
    EXPECT_TRUE(cp.isEnd());
    EXPECT_EQ(cp.currentChar(), '\0');
}

TEST(CharProducerTest, NextReturnsFalseAtEnd) {
    CharProducer cp("x");
    EXPECT_FALSE(cp.isEnd());
    bool advanced = cp.next(); // moves past 'x'
    EXPECT_TRUE(cp.isEnd());
    // next() at end returns false
    EXPECT_FALSE(cp.next());
}

TEST(CharProducerTest, EmptyString) {
    CharProducer cp("");
    EXPECT_TRUE(cp.isEnd());
    EXPECT_EQ(cp.currentChar(), '\0');
    EXPECT_FALSE(cp.next());
}

TEST(CharProducerTest, Reset) {
    CharProducer cp("hello");
    cp.next();
    cp.next();
    EXPECT_EQ(cp.currentChar(), 'l');

    cp.reset();
    EXPECT_EQ(cp.index(), 0u);
    EXPECT_EQ(cp.currentChar(), 'h');
    EXPECT_FALSE(cp.isEnd());
}

TEST(CharProducerTest, GetString) {
    CharProducer cp("test string");
    EXPECT_EQ(cp.getString(), "test string");
    cp.next();
    cp.next();
    // getString always returns the full original string
    EXPECT_EQ(cp.getString(), "test string");
}

TEST(CharProducerTest, SingleChar) {
    CharProducer cp("z");
    EXPECT_EQ(cp.currentChar(), 'z');
    EXPECT_FALSE(cp.isEnd());
    cp.next();
    EXPECT_TRUE(cp.isEnd());
}

TEST(CharProducerTest, TraverseFullString) {
    std::string s = "abcde";
    CharProducer cp(s);
    std::string result;
    while (!cp.isEnd()) {
        result += cp.currentChar();
        cp.next();
    }
    EXPECT_EQ(result, s);
}
