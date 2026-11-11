#include <gtest/gtest.h>
#include <Mimo/Serializer/DeserializeNode.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace
{
using namespace mimo::serializer;

struct Node
{
    std::string name;
    std::variant<int, bool, float, std::string, std::vector<Node>> content;
    std::optional<std::string> type = std::nullopt;
};

// In-memory archive exercising traversal independently of XML parsing/conversion.
class TestInputArchive : public InputArchive<TestInputArchive>
{
    struct Frame
    {
        const std::vector<Node>* nodes;
        std::vector<bool> consumed;
    };
    std::vector<Frame> m_frames;

    const Node* find(NodeMetadata metadata, bool consume)
    {
        auto& frame = m_frames.back();
        std::optional<std::size_t> match;
        for (std::size_t i = 0; i < frame.nodes->size(); ++i)
        {
            if (!frame.consumed[i] && (*frame.nodes)[i].name == metadata.name)
            {
                if (match && metadata.name != "item")
                    throw std::runtime_error("Duplicate field.");
                if (!match)
                    match = i;
            }
        }
        if (!match)
            return nullptr;
        const auto& node = (*frame.nodes)[*match];
        if (metadata.typeDiscriminator && node.type != metadata.typeDiscriminator)
            throw std::runtime_error("Unexpected discriminator.");
        if (consume)
            frame.consumed[*match] = true;
        return &node;
    }

    const Node& take(NodeMetadata metadata)
    {
        const auto* node = find(metadata, true);
        if (!node)
            throw std::runtime_error("Missing node.");
        return *node;
    }

public:
    explicit TestInputArchive(const std::vector<Node>& nodes)
        : m_frames{{&nodes, std::vector<bool>(nodes.size())}} {}

    struct Scope
    {
        TestInputArchive& archive;
        Scope(const Scope&) = delete;
        explicit Scope(TestInputArchive& archive) : archive(archive) {}
        ~Scope() { archive.m_frames.pop_back(); }
        bool hasNext() const
        {
            const auto& frame = archive.m_frames.back();
            return std::ranges::find(frame.consumed, false) != frame.consumed.end();
        }
    };

private:
    friend class mimo::serializer::InputArchive<TestInputArchive>;

    bool containsImpl(NodeMetadata metadata) { return find(metadata, false) != nullptr; }
    std::optional<std::string_view> typeDiscriminatorImpl(NodeMetadata metadata)
    {
        const auto* node = find(metadata, false);
        if (!node || !node->type)
            return std::nullopt;
        return *node->type;
    }
    template <typename T>
    void valueImpl(NodeMetadata metadata, T& value)
    {
        value = std::get<T>(take(metadata).content);
    }
    Scope objectImpl(NodeMetadata metadata)
    {
        const auto& children = std::get<std::vector<Node>>(take(metadata).content);
        m_frames.push_back({&children, std::vector<bool>(children.size())});
        return Scope(*this);
    }
    Scope sequenceImpl(NodeMetadata metadata) { return objectImpl(metadata); }
    void emptyImpl(NodeMetadata metadata)
    {
        if (!std::get<std::vector<Node>>(take(metadata).content).empty())
            throw std::runtime_error("Expected an empty node.");
    }
};

struct Nested { int number = 0; };
struct Object
{
    std::string text;
    Nested nested;
    std::optional<int> absent = 9;
    std::optional<int> present;
    std::vector<int> numbers{99};
};

TEST(DeserializeNode, ReadsMembersByNameAndReplacesContainers)
{
    const std::vector<Node> nodes{{"root", std::vector<Node>{
        {"numbers", std::vector<Node>{{"item", 1}, {"item", 2}}},
        {"present", 7}, {"nested", std::vector<Node>{{"number", 42}}},
        {"text", std::string("hello")}}}};
    TestInputArchive archive(nodes);
    Object object;
    deserializeNode(archive, object, {.name = "root"});
    EXPECT_EQ(object.text, "hello");
    EXPECT_EQ(object.nested.number, 42);
    EXPECT_FALSE(object.absent);
    EXPECT_EQ(object.present, 7);
    EXPECT_EQ(object.numbers, (std::vector{1, 2}));
}

TEST(DeserializeNode, ReadsPrimitiveAndEmptyVectorRoots)
{
    const std::vector<Node> nodes{{"flag", true}, {"ratio", 1.5f},
        {"empty", std::vector<Node>{}}};
    TestInputArchive archive(nodes);
    bool flag = false;
    float ratio = 0;
    std::vector<int> numbers{3};
    deserializeNode(archive, flag, {.name = "flag"});
    deserializeNode(archive, ratio, {.name = "ratio"});
    deserializeNode(archive, numbers, {.name = "empty"});
    EXPECT_TRUE(flag);
    EXPECT_EQ(ratio, 1.5f);
    EXPECT_TRUE(numbers.empty());
}

TEST(DeserializeNode, ReadsVariantAlternativesAndMonostate)
{
    const std::vector<Node> nodes{
        {"integer", 12, "int"},
        {"text", std::string("hello"), "string"},
        {"object", std::vector<Node>{{"number", 5}},
            std::string(TypeDiscriminator<Nested>::value)},
        {"empty", std::vector<Node>{}, "monostate"}};
    TestInputArchive archive(nodes);
    std::variant<std::monostate, int, std::string, Nested> value;
    deserializeNode(archive, value, {.name = "integer"});
    EXPECT_EQ(std::get<int>(value), 12);
    deserializeNode(archive, value, {.name = "text"});
    EXPECT_EQ(std::get<std::string>(value), "hello");
    deserializeNode(archive, value, {.name = "object"});
    EXPECT_EQ(std::get<Nested>(value).number, 5);
    deserializeNode(archive, value, {.name = "empty"});
    EXPECT_TRUE(std::holds_alternative<std::monostate>(value));
}

TEST(DeserializeNode, RejectsMissingUnknownAndAmbiguousDiscriminators)
{
    const std::vector<Node> nodes{{"missing", 1}, {"unknown", 1, "unknown"},
        {"duplicate", 1, "int"}};
    TestInputArchive archive(nodes);
    std::variant<int, std::string> value = 9;
    EXPECT_THROW(deserializeNode(archive, value, {.name = "missing"}), std::runtime_error);
    EXPECT_THROW(deserializeNode(archive, value, {.name = "unknown"}), std::runtime_error);
    EXPECT_EQ(std::get<int>(value), 9);
    std::variant<int, int> ambiguous;
    EXPECT_THROW(deserializeNode(archive, ambiguous, {.name = "duplicate"}), std::runtime_error);
}

TEST(DeserializeNode, ErrorsRestoreScopesAndPreserveVectorOnFailure)
{
    const std::vector<Node> nodes{{"root", std::vector<Node>{}},
        {"numbers", std::vector<Node>{{"item", 1}, {"item", std::string("bad")}}},
        {"after", 42}};
    TestInputArchive archive(nodes);
    Nested object;
    EXPECT_THROW(deserializeNode(archive, object, {.name = "root"}), std::runtime_error);
    std::vector<int> numbers{9};
    EXPECT_THROW(deserializeNode(archive, numbers, {.name = "numbers"}), std::bad_variant_access);
    EXPECT_EQ(numbers, std::vector{9});
    int after = 0;
    deserializeNode(archive, after, {.name = "after"});
    EXPECT_EQ(after, 42);
}

TEST(DeserializeNode, ReadsNestedContainersAndPrivateMembers)
{
    class Private
    {
        int number = 0;
    public:
        int get() const { return number; }
    };
    const std::vector<Node> nodes{
        {"private", std::vector<Node>{{"number", 8}}},
        {"nested", std::vector<Node>{{"item", std::vector<Node>{{"item", true}}}}}};
    TestInputArchive archive(nodes);
    Private object;
    deserializeNode(archive, object, {.name = "private"});
    EXPECT_EQ(object.get(), 8);
    std::optional<std::vector<std::vector<bool>>> nested;
    deserializeNode(archive, nested, {.name = "nested"});
    ASSERT_TRUE(nested);
    EXPECT_EQ(*nested, (std::vector<std::vector<bool>>{{true}}));
}

TEST(DeserializeNode, RejectsDuplicateFieldsAndNonemptyMonostate)
{
    const std::vector<Node> nodes{
        {"duplicate", std::vector<Node>{{"number", 1}, {"number", 2}}},
        {"nonempty", std::vector<Node>{{"number", 1}}, "monostate"}};
    TestInputArchive archive(nodes);
    Nested object;
    EXPECT_THROW(deserializeNode(archive, object, {.name = "duplicate"}), std::runtime_error);
    std::variant<std::monostate, int> value = 5;
    EXPECT_THROW(deserializeNode(archive, value, {.name = "nonempty"}), std::runtime_error);
    EXPECT_EQ(std::get<int>(value), 5);
}
}
