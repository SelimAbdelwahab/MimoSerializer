#include <gtest/gtest.h>

#include <Mimo/Serializer/DeserializeNode.hpp>
#include <Mimo/Serializer/SerializeNode.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace
{
using namespace mimo::serializer;

struct Tag { int value; };
struct Extra { int value; };
struct Missing {};

struct [[=Tag{10}]] [[=Extra{11}]] AnnotatedValue
{
    int value = 7;
};

struct [[=Tag{1}]] AnnotatedObject
{
    [[=Tag{20}]] AnnotatedValue overridden;
    AnnotatedValue fallback;

    [[=Tag{30}]] std::optional<AnnotatedValue> optional = AnnotatedValue{};
    [[=Tag{40}]] std::variant<int, AnnotatedValue> variant = AnnotatedValue{};
    std::vector<AnnotatedValue> sequence{AnnotatedValue{}};
};

struct Scope
{
    int remaining = 0;
    bool hasNext() { return remaining-- > 0; }
};

struct RecordingOutputArchive : OutputArchive<RecordingOutputArchive>
{
    std::vector<NodeMetadata> nodes;

    template <typename T>
    void valueImpl(NodeMetadata metadata, const T&) { nodes.push_back(metadata); }

    Scope objectImpl(NodeMetadata metadata) { nodes.push_back(metadata); return {}; }
    Scope sequenceImpl(NodeMetadata metadata) { nodes.push_back(metadata); return {}; }
    void emptyImpl(NodeMetadata metadata) { nodes.push_back(metadata); }
};

struct RecordingInputArchive : InputArchive<RecordingInputArchive>
{
    std::vector<NodeMetadata> nodes;

    bool containsImpl(NodeMetadata) { return true; }

    std::optional<std::string_view> typeDiscriminatorImpl(NodeMetadata)
    {
        return TypeDiscriminator<AnnotatedValue>::value;
    }

    template <typename T>
    void valueImpl(NodeMetadata metadata, T& value)
    {
        nodes.push_back(metadata);
        value = T{};
    }

    Scope objectImpl(NodeMetadata metadata) { nodes.push_back(metadata); return {}; }
    Scope sequenceImpl(NodeMetadata metadata) { nodes.push_back(metadata); return {1}; }
    void emptyImpl(NodeMetadata metadata) { nodes.push_back(metadata); }
};

void expectTag(const NodeMetadata& metadata, int expected)
{
    const auto* tag = metadata.getAnnotation<Tag>();
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->value, expected);
    EXPECT_TRUE(metadata.hasAnnotation<Tag>());
    EXPECT_FALSE(metadata.hasAnnotation<Missing>());
    EXPECT_EQ(metadata.getAnnotation<Missing>(), nullptr);
}

void expectMetadata(const std::vector<NodeMetadata>& nodes)
{
    auto node = [&](std::string_view name) -> const NodeMetadata&
    {
        auto found = std::ranges::find(nodes, name, &NodeMetadata::name);
        if (found == nodes.end())
            throw std::runtime_error("Expected node was not visited.");
        return *found;
    };
    expectTag(node("root"), 1);
    expectTag(node("overridden"), 20);
    expectTag(node("fallback"), 10);
    expectTag(node("optional"), 30);
    expectTag(node("variant"), 40);
    expectTag(node("item"), 10);
    for (auto name : {"overridden", "fallback", "optional", "variant", "item"})
    {
        auto extra = node(name).getAnnotation<Extra>();
        ASSERT_NE(extra, nullptr);
        EXPECT_EQ(extra->value, 11);
    }
    // A member's annotations must not leak into its own children.
    const auto& primitive = node("value");
    EXPECT_FALSE(primitive.hasAnnotation<Tag>());
    EXPECT_FALSE(primitive.hasAnnotation<Extra>());
}

TEST(AnnotationMetadata, SerializationExposesAnnotationsAndPreservesMemberOverrides)
{
    RecordingOutputArchive archive;
    serializeNode(archive, AnnotatedObject{}, {.name = "root"});
    // Metadata copies kept by the archive remain valid after traversal returns.
    expectMetadata(archive.nodes);
}

TEST(AnnotationMetadata, DeserializationExposesTheSameAnnotations)
{
    RecordingInputArchive archive;
    AnnotatedObject object;
    deserializeNode(archive, object, {.name = "root"});
    expectMetadata(archive.nodes);
}

TEST(AnnotationMetadata, ExplicitRootAnnotationsOverrideTypeAndPreserveIdentity)
{
    struct [[=Tag{99}]] RootOverrides {};
    NodeMetadata metadata{.name = "custom", .typeDiscriminator = "custom-type",
        .annotations = AnnotationView::from<^^RootOverrides>()};
    RecordingOutputArchive archive;
    serializeNode(archive, AnnotatedValue{}, metadata);
    ASSERT_FALSE(archive.nodes.empty());
    const auto& root = archive.nodes.front();
    expectTag(root, 99);
    EXPECT_EQ(root.name, "custom");
    EXPECT_EQ(root.typeDiscriminator, "custom-type");
    ASSERT_NE(root.getAnnotation<Extra>(), nullptr);
    EXPECT_EQ(root.getAnnotation<Extra>()->value, 11);
}
}
