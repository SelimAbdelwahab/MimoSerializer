#include <gtest/gtest.h>

#include <any>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <Mimo/Serializer/XmlSerializer.hpp>


struct Nested {
    int test = 3;
};

struct Example {
    int alpha = 0;
    Nested nested;
};

TEST(SerializerXml, SerializesNestedObject)
{
    const Example value;

    EXPECT_EQ(
        mimo::serializer::xml::serialize(value),
            "<Example>\n"
            "    <alpha>0</alpha>\n"
            "    <nested>\n"
            "        <test>3</test>\n"
            "    </nested>\n"
            "</Example>\n"
        );
}

namespace Long::Path::Ns {
    struct Inner{};
}
struct Outer
{
    Long::Path::Ns::Inner inner;
};


TEST(SerializerXml, SerializesNestedNamespaces)
{
    const Outer value;

    EXPECT_EQ(
        mimo::serializer::xml::serialize(value),
            "<Outer>\n"
            "    <inner />\n"
            "</Outer>\n"
        );
}

class Class
{
    int alpha = 10;
    bool boolean = true;
};

TEST(SerializerXml, SerializesPrivateMembers)
{
    EXPECT_EQ(mimo::serializer::xml::serialize(Class{}),
        "<Class>\n"
        "    <alpha>10</alpha>\n"
        "    <boolean>true</boolean>\n"
        "</Class>\n");
}

struct TextExample
{
    std::string text = "A & <B>";
};

TEST(SerializerXml, EscapesStringFields)
{
    EXPECT_EQ(mimo::serializer::xml::serialize(TextExample{}),
        "<TextExample>\n"
        "    <text>A &amp; &lt;B&gt;</text>\n"
        "</TextExample>\n");
}

TEST(SerializerXml, SupportsPrimitiveRoots)
{
    EXPECT_EQ(mimo::serializer::xml::serialize(42), "<int>42</int>\n");
}

TEST(SerializerXml, SupportsCustomRootMetadata)
{
    using mimo::serializer::NodeMetadata;

    EXPECT_EQ(mimo::serializer::xml::serialize(
        Example{},
        NodeMetadata{.name = "example", .typeDiscriminator = "example-v1"}),
        "<example type=\"example-v1\">\n"
        "    <alpha>0</alpha>\n"
        "    <nested>\n"
        "        <test>3</test>\n"
        "    </nested>\n"
        "</example>\n");
}

struct VectorExample
{
    std::vector<int> numbers{1, 2};
    std::vector<Nested> objects{{3}, {4}};
    std::vector<int> empty;
};

TEST(SerializerXml, SerializesVectorsAsItemElements)
{
    EXPECT_EQ(mimo::serializer::xml::serialize(VectorExample{}),
        "<VectorExample>\n"
        "    <numbers>\n"
        "        <item>1</item>\n"
        "        <item>2</item>\n"
        "    </numbers>\n"
        "    <objects>\n"
        "        <item>\n"
        "            <test>3</test>\n"
        "        </item>\n"
        "        <item>\n"
        "            <test>4</test>\n"
        "        </item>\n"
        "    </objects>\n"
        "    <empty />\n"
        "</VectorExample>\n");
}

TEST(SerializerXml, SupportsVectorRoots)
{
    EXPECT_EQ(mimo::serializer::xml::serialize(std::vector{1, 2}),
        "<vector>\n"
        "    <item>1</item>\n"
        "    <item>2</item>\n"
        "</vector>\n");
}

struct OptionalExample
{
    std::optional<int> present = 7;
    std::optional<int> absent;
    std::optional<Nested> object = Nested{8};
};

TEST(SerializerXml, SerializesPresentAndSkipsAbsentOptionals)
{
    EXPECT_EQ(mimo::serializer::xml::serialize(OptionalExample{}),
        "<OptionalExample>\n"
        "    <present>7</present>\n"
        "    <object>\n"
        "        <test>8</test>\n"
        "    </object>\n"
        "</OptionalExample>\n");

    EXPECT_EQ(mimo::serializer::xml::serialize(std::optional<int>{}), "");
    EXPECT_EQ(mimo::serializer::xml::serialize(std::optional{7}),
        "<optional>7</optional>\n");
}

struct VariantExample
{
    std::variant<std::monostate, int, std::string, Nested> value =
        std::string("42");
};

TEST(SerializerXml, SerializesVariantWithAlternativeDiscriminator)
{
    EXPECT_EQ(mimo::serializer::xml::serialize(VariantExample{}),
        "<VariantExample>\n"
        "    <value type=\"string\">42</value>\n"
        "</VariantExample>\n");

    VariantExample value;
    value.value = Nested{9};
    EXPECT_EQ(mimo::serializer::xml::serialize(value),
        "<VariantExample>\n"
        "    <value type=\"Nested\">\n"
        "        <test>9</test>\n"
        "    </value>\n"
        "</VariantExample>\n");

    value.value = std::monostate{};
    EXPECT_EQ(mimo::serializer::xml::serialize(value),
        "<VariantExample>\n"
        "    <value type=\"monostate\" />\n"
        "</VariantExample>\n");
}

struct AnyExample
{
    std::any primitive = std::string("hello");
    std::any object = Nested{10};
    std::any empty;
};

TEST(SerializerXml, SerializesAnyWithRegisteredTypeDiscriminator)
{
    mimo::serializer::xml::XmlArchive::registerAnyType<Nested>("nested");

    EXPECT_EQ(mimo::serializer::xml::serialize(AnyExample{}),
        "<AnyExample>\n"
        "    <primitive type=\"string\">hello</primitive>\n"
        "    <object type=\"nested\">\n"
        "        <test>10</test>\n"
        "    </object>\n"
        "</AnyExample>\n");
}

TEST(SerializerXml, RejectsUnregisteredAnyTypes)
{
    EXPECT_THROW(
        static_cast<void>(
            mimo::serializer::xml::serialize(std::any{double{1.5}})),
        std::runtime_error);
}
