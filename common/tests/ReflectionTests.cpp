#include <gtest/gtest.h>
#include <Mimo/Serializer/Reflection.hpp>

struct SimpleStruct
{
    int a = 1;
    int b = 2;
    int c = 3;
};

struct InnerStruct
{
    bool a = false;
    bool b = true;
};

struct OuterStruct
{
    int a = 1;
    int b = 2;
    InnerStruct innerStruct;
};

TEST(ReflectionTests, FlatStruct)
{
    const SimpleStruct s;
    mimo::serializer::ReflectionNode tree(s);

    EXPECT_EQ(tree.getName(), "root");
    EXPECT_EQ(tree.getTypeName(), "SimpleStruct");
    const auto& children = tree.getChildren();
    ASSERT_EQ(children.size(), 3U);
    EXPECT_EQ(children[0].getName(), "a");
    EXPECT_EQ(children[1].getName(), "b");
    EXPECT_EQ(children[2].getName(), "c");
    for (const auto& child : children)
    {
        EXPECT_EQ(child.getTypeName(), "int");
        EXPECT_TRUE(child.getChildren().empty());
    }
}

TEST(ReflectionTests, NestedStruct)
{
    const OuterStruct s;
    mimo::serializer::ReflectionNode tree(s, "outer");

    EXPECT_EQ(tree.getName(), "outer");
    EXPECT_EQ(tree.getTypeName(), "OuterStruct");
    const auto& children = tree.getChildren();
    ASSERT_EQ(children.size(), 3U);
    EXPECT_EQ(children[0].getName(), "a");
    EXPECT_EQ(children[1].getName(), "b");
    EXPECT_EQ(children[2].getName(), "innerStruct");
    EXPECT_EQ(children[2].getTypeName(), "InnerStruct");

    const auto& innerChildren = children[2].getChildren();
    ASSERT_EQ(innerChildren.size(), 2U);
    EXPECT_EQ(innerChildren[0].getName(), "a");
    EXPECT_EQ(innerChildren[1].getName(), "b");
    for (const auto& child : innerChildren)
    {
        EXPECT_EQ(child.getTypeName(), "bool");
        EXPECT_TRUE(child.getChildren().empty());
    }
}
