#pragma once

#include <meta>
#include <span>
#include <type_traits>
#include <vector>

namespace mimo::serializer
{

template <typename Annotation>
consteval bool has_annotation(std::meta::info source)
{
    return !std::meta::annotations_of_with_type(source, ^^Annotation).empty();
}

namespace detail
{
    template <typename A>
    inline constexpr char s_typeTag = 0;

    struct AnnotationEntry
    {
        const void* key;
        const void* value;
    };

    template <std::meta::info Source>
    consteval void appendAnnotations(std::vector<AnnotationEntry>& entries)
    {
        template for (constexpr auto annotation :
                      std::define_static_array(std::meta::annotations_of(Source)))
        {
            // Annotation objects are const; callers query by the annotation's
            // value type rather than its storage qualification.
            using Stored = [:std::meta::type_of(annotation):];
            using A = std::remove_cv_t<Stored>;
            entries.push_back({
                &s_typeTag<A>,
                std::define_static_object([:std::meta::constant_of(annotation):])
            });
        }
    }
}

// A non-owning view of a compile-time annotation table. Both the table and its
// annotation objects have static storage duration; copies remain valid forever.
class AnnotationView
{
public:
    constexpr AnnotationView() = default;

    // Earlier sources take precedence when the same annotation type is repeated.
    template <std::meta::info... Sources>
    static consteval AnnotationView from()
    {
        std::vector<detail::AnnotationEntry> entries;
        (detail::appendAnnotations<Sources>(entries), ...);
        return AnnotationView(std::define_static_array(entries));
    }

    template <typename A>
    [[nodiscard]] const A* getAnnotation() const noexcept
    {
        for (const auto& entry : m_entries)
        {
            if (entry.key == &detail::s_typeTag<std::remove_cv_t<A>>)
                return static_cast<const A*>(entry.value);
        }
        return nullptr;
    }

    template <typename A>
    [[nodiscard]] bool hasAnnotation() const noexcept
    {
        return getAnnotation<A>() != nullptr;
    }

private:
    constexpr explicit AnnotationView(std::span<const detail::AnnotationEntry> entries)
        : m_entries(entries) {}

    std::span<const detail::AnnotationEntry> m_entries;
};

} // namespace mimo::serializer
