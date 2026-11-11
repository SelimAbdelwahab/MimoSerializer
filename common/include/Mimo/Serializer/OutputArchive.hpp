#pragma once

#include <concepts>
#include <string>
#include <type_traits>

#include <Mimo/Serializer/NodeMetadata.hpp>

namespace mimo::serializer
{

namespace detail
{
    template <typename Scope>
    concept OutputScope = std::is_class_v<Scope>
        && std::is_nothrow_destructible_v<Scope>;
}

// Derive as: class MyArchive : public OutputArchive<MyArchive>.
// Implement the four *Impl hooks below, either publicly or privately with
// `friend class mimo::serializer::OutputArchive<MyArchive>;`.
// Construction checks all hook signatures, even operations not used by a write.
// The implementation must honor names, discriminators, and any supported
// annotations in NodeMetadata. Copy any metadata strings retained after a call.
template <typename Derived>
class OutputArchive
{
public:
    // void valueImpl(NodeMetadata, const T&): write a complete primitive node.
    // Support int, bool, float, and std::string, via overloads or a template.
    template <typename T>
    void value(NodeMetadata metadata, const T& source)
    {
        derived().valueImpl(metadata, source);
    }

    // Scope objectImpl(NodeMetadata): open an object whose members are written
    // by name within the returned scope.
    [[nodiscard]] auto object(NodeMetadata metadata)
    {
        return derived().objectImpl(metadata);
    }

    // Scope sequenceImpl(NodeMetadata): open a sequence. Subsequent writes named
    // "item" append entries in traversal order.
    [[nodiscard]] auto sequence(NodeMetadata metadata)
    {
        return derived().sequenceImpl(metadata);
    }

    // void emptyImpl(NodeMetadata): write and close a complete empty node.
    void empty(NodeMetadata metadata)
    {
        derived().emptyImpl(metadata);
    }

    // Object/sequence scopes close their node and restore the parent when
    // destroyed, including during unwinding. Destructors must not throw.
    // Scope types need not be copyable or movable.

private:
    friend Derived;

    OutputArchive()
    {
        static_assert(requires(Derived& archive, NodeMetadata metadata,
                               const int& integer, const bool& boolean,
                               const float& real, const std::string& text)
        {
            { archive.valueImpl(metadata, integer) } -> std::same_as<void>;
            { archive.valueImpl(metadata, boolean) } -> std::same_as<void>;
            { archive.valueImpl(metadata, real) } -> std::same_as<void>;
            { archive.valueImpl(metadata, text) } -> std::same_as<void>;
            { archive.objectImpl(metadata) } -> detail::OutputScope;
            { archive.sequenceImpl(metadata) } -> detail::OutputScope;
            { archive.emptyImpl(metadata) } -> std::same_as<void>;
        }, "OutputArchive requires valueImpl for int/bool/float/string, objectImpl, "
           "sequenceImpl, and emptyImpl. Scopes must have noexcept destructors.");
    }

    ~OutputArchive() = default;

    Derived& derived() noexcept { return static_cast<Derived&>(*this); }
};

} // namespace mimo::serializer
