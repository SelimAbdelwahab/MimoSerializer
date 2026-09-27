#include <gtest/gtest.h>
#include <string>
#include <utility>

#include "Mimo/Xml/XmlWriter.hpp"

TEST(XmlWriter, writeElement)
{
    mimo::serializer::xml::XmlWriter writer;

    {
        auto _ = writer.scope("A");
        writer.attribute("hello", "world");
        {
            auto _ = writer.scope("B");
            writer.attribute("hello", "world");
            {
                auto _ = writer.scope("C");
                writer.attribute("hello", "world");
            }
        }
        {
            auto _ = writer.scope("D");
            writer.attribute("hello", "world");
            writer.attribute("foo", "bar");
        }
    }

    EXPECT_EQ(writer.toString(),
        "<A hello=\"world\">\n"
        "    <B hello=\"world\">\n"
        "        <C hello=\"world\" />\n"
        "    </B>\n"
        "    <D hello=\"world\" foo=\"bar\" />\n"
        "</A>\n"
    );
}

using mimo::serializer::xml::ClashingWrite;
using mimo::serializer::xml::XmlWriter;

TEST(XmlWriter, WritesText)
{
    XmlWriter writer;
    {
        auto root = writer.scope("root");
        writer.value("hello");
    }
    EXPECT_EQ(writer.toString(), "<root>hello</root>\n");
}

TEST(XmlWriter, WritesEmptyTextAsEmptyElement)
{
    XmlWriter writer;
    {
        auto root = writer.scope("root");
        writer.value("");
    }
    EXPECT_EQ(writer.toString(), "<root />\n");
}

TEST(XmlWriter, EscapesTextAndAttributes)
{
    XmlWriter writer;
    {
        auto root = writer.scope("root");
        writer.attribute("label", "A & \"B\" <C> 'D'\t\n\r");
        writer.value("A & <B> \"C\" 'D' ]]>\t\n\r");
    }
    EXPECT_EQ(writer.toString(),
        "<root label=\"A &amp; &quot;B&quot; &lt;C&gt; 'D'&#9;&#10;&#13;\">"
        "A &amp; &lt;B&gt; \"C\" 'D' ]]&gt;\t\n&#13;</root>\n");
}

TEST(XmlWriter, OwnsAttributeStrings)
{
    XmlWriter writer;
    {
        auto root = writer.scope("root");
        std::string name = "label";
        std::string value = "before";
        writer.attribute(name, value);
        name[0] = 'X';
        value[0] = 'X';
        writer.attribute(std::string("id"), std::to_string(42));
    }
    EXPECT_EQ(writer.toString(), "<root label=\"before\" id=\"42\" />\n");
}

TEST(XmlWriter, RejectsChildrenAfterTextWithoutChangingTree)
{
    XmlWriter writer;
    {
        auto root = writer.scope("root");
        writer.value("text");
        EXPECT_THROW({ auto child = writer.scope("child"); }, ClashingWrite);
        writer.attribute("id", "1");
    }
    EXPECT_EQ(writer.toString(), "<root id=\"1\">text</root>\n");
}

TEST(XmlWriter, RejectsTextAfterChildrenWithoutChangingTree)
{
    XmlWriter writer;
    {
        auto root = writer.scope("root");
        { auto child = writer.scope("child"); }
        EXPECT_THROW(writer.value("text"), ClashingWrite);
    }
    EXPECT_EQ(writer.toString(), "<root>\n    <child />\n</root>\n");
}

TEST(XmlWriter, RejectsWritesOutsideScope)
{
    XmlWriter writer;
    EXPECT_THROW(writer.attribute("id", "1"), std::runtime_error);
    EXPECT_THROW(writer.value("text"), std::runtime_error);
    EXPECT_EQ(writer.toString(), "");
    { auto root = writer.scope("root"); }
    EXPECT_THROW(writer.attribute("id", "1"), std::runtime_error);
    EXPECT_THROW(writer.value("text"), std::runtime_error);
    EXPECT_EQ(writer.toString(), "<root />\n");
}

TEST(XmlWriter, MovingScopesTransfersClosingResponsibility)
{
    XmlWriter writer;
    {
        auto root = writer.scope("root");
        {
            auto child = writer.scope("child");
            auto moved = std::move(child);
            { auto inactive = std::move(child); }
            writer.attribute("id", "1");
            auto movedAgain = std::move(moved);
        }
        { auto sibling = writer.scope("sibling"); }
    }
    EXPECT_EQ(writer.toString(),
        "<root>\n    <child id=\"1\" />\n    <sibling />\n</root>\n");
}

TEST(XmlWriter, ClosesScopesDuringExceptionUnwinding)
{
    XmlWriter writer;
    {
        auto root = writer.scope("root");
        EXPECT_THROW({
            auto child = writer.scope("child");
            throw std::runtime_error("test");
        }, std::runtime_error);
        writer.attribute("id", "1");
    }
    EXPECT_EQ(writer.toString(), "<root id=\"1\">\n    <child />\n</root>\n");
}
