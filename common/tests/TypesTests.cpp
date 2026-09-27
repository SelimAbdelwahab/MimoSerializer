// #include <gtest/gtest.h>
//
// #include <string>
//
// #include <Mimo/Serializer/Reflection.hpp>
//
// struct Nested {
//     int test = 3;
// };
//
// struct Example {
//     int alpha = 0;
//     Nested nested;
// };
//
// struct SupportedFieldKinds {
//     int integer = 0;
//     bool boolean = false;
//     float floatingPoint = 0.0F;
//     std::string text;
//     Nested nested;
// };
//
// TEST(SerializerFields, ClassifiesPrimitiveAndComplexFields)
// {
//     constexpr std::size_t primitiveCount =
//         mimo::serializer::primitive_fields<Example>().size();
//     constexpr std::size_t complexCount =
//         mimo::serializer::complex_fields<Example>().size();
//
//     EXPECT_EQ(primitiveCount, 1U);
//     EXPECT_EQ(complexCount, 1U);
// }
//
// TEST(SerializerFields, TreatsStringAsPrimitiveRatherThanComplex)
// {
//     constexpr std::size_t primitiveCount =
//         mimo::serializer::primitive_fields<SupportedFieldKinds>().size();
//     constexpr std::size_t complexCount =
//         mimo::serializer::complex_fields<SupportedFieldKinds>().size();
//
//     EXPECT_EQ(primitiveCount, 4U);
//     EXPECT_EQ(complexCount, 1U);
// }
