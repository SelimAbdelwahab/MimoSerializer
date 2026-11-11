#pragma once

#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include <Mimo/Serializer/NodeMetadata.hpp>

namespace mimo::serializer
{

namespace detail
{
    template <typename Scope>
    concept InputObjectScope = std::is_class_v<Scope>
        && std::is_nothrow_destructible_v<Scope>;

    template <typename Scope>
    concept InputSequenceScope = InputObjectScope<Scope> && requires(Scope& scope)
    {
        { scope.hasNext() } -> std::same_as<bool>;
    };
}

// Derive as: class MyArchive : public InputArchive<MyArchive>.
// Implement the six *Impl hooks below, either publicly or privately with
// `friend class mimo::serializer::InputArchive<MyArchive>;`.
// Construction checks the entire interface, even operations not used by a read.
// The checks verify signatures; implementations must honor the behavior below.
//
// Metadata selects a node by name in the current scope. A supplied discriminator
// is an expected value and must be validated. Missing required nodes, duplicate
// fields, invalid values, and unexpected sequence entries must raise errors.
// Unknown object fields may be ignored or rejected by the implementation.
template <typename Derived>
class InputArchive
{
public:
    // bool containsImpl(NodeMetadata): inspect without consuming the node.
    [[nodiscard]] bool contains(NodeMetadata metadata)
    {
        return derived().containsImpl(metadata);
    }

    // optional<string_view> typeDiscriminatorImpl(NodeMetadata): inspect without
    // consuming. The returned view must remain valid until the node is consumed.
    [[nodiscard]] std::optional<std::string_view> typeDiscriminator(NodeMetadata metadata)
    {
        return derived().typeDiscriminatorImpl(metadata);
    }

    // void valueImpl(NodeMetadata, T&): validate, convert, and consume one node.
    // Support int, bool, float, and std::string, via overloads or a template.
    template <typename T>
    void value(NodeMetadata metadata, T& destination)
    {
        derived().valueImpl(metadata, destination);
    }

    // Scope objectImpl(NodeMetadata): consume/open an object. Member reads select
    // children by name, independently of their order in the input.
    [[nodiscard]] auto object(NodeMetadata metadata)
    {
        return derived().objectImpl(metadata);
    }

    // Scope sequenceImpl(NodeMetadata): consume/open a sequence. Scope::hasNext()
    // reports whether another item remains; reading "item" consumes the next
    // item in input order. Reject unexpected entries rather than skipping them.
    [[nodiscard]] auto sequence(NodeMetadata metadata)
    {
        return derived().sequenceImpl(metadata);
    }

    // void emptyImpl(NodeMetadata): validate and consume an empty node.
    void empty(NodeMetadata metadata)
    {
        derived().emptyImpl(metadata);
    }

    // Object/sequence scopes restore the parent on destruction, including during
    // unwinding. Destructors must not throw; validation happens during reads.
    // Scope types need not be copyable or movable.

private:
    friend Derived;

    InputArchive()
    {
        static_assert(requires(Derived& archive, NodeMetadata metadata,
                               int& integer, bool& boolean, float& real, std::string& text)
        {
            { archive.containsImpl(metadata) } -> std::same_as<bool>;
            { archive.typeDiscriminatorImpl(metadata) }
                -> std::same_as<std::optional<std::string_view>>;
            { archive.valueImpl(metadata, integer) } -> std::same_as<void>;
            { archive.valueImpl(metadata, boolean) } -> std::same_as<void>;
            { archive.valueImpl(metadata, real) } -> std::same_as<void>;
            { archive.valueImpl(metadata, text) } -> std::same_as<void>;
            { archive.objectImpl(metadata) } -> detail::InputObjectScope;
            { archive.sequenceImpl(metadata) } -> detail::InputSequenceScope;
            { archive.emptyImpl(metadata) } -> std::same_as<void>;
        }, "InputArchive requires containsImpl, typeDiscriminatorImpl, valueImpl "
           "for int/bool/float/string, objectImpl, sequenceImpl, and emptyImpl. "
           "Scopes must have noexcept destructors; sequence scopes need bool hasNext().");
    }

    // Keep the interface tied to a concrete archive; it is not a polymorphic owner.
    ~InputArchive() = default;

    Derived& derived() noexcept { return static_cast<Derived&>(*this); }
};

} // namespace mimo::serializer
