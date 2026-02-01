#include <print>
#include <string>
#include <map>

#include <glaze/glaze.hpp>
#include <glm/vec2.hpp>

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

using i32 = int32_t;
using f32 = float;

struct IntValue
{
    i32 val;

    auto operator<=>(const IntValue &) const = default;
};

template <>
struct glz::meta<IntValue>
{
    using T = IntValue;
    static constexpr auto value = glz::object(&T::val);
};

struct FloatValue
{
    f32 val;

    auto operator<=>(const FloatValue &) const = default;
};

template <>
struct glz::meta<FloatValue>
{
    using T = FloatValue;
    static constexpr auto value = glz::object(&T::val);
};

template <>
struct glz::meta<glm::vec2>
{
    using T = glm::vec2;
    static constexpr auto value = object(&T::x, &T::y);
};

using SimpleValue = std::variant<IntValue, FloatValue, glm::vec2>;
template <>
struct glz::meta<SimpleValue>
{
    static constexpr std::string_view tag = "type";
    static constexpr auto ids = std::array{"int", "float", "vec2"};
};

using SimpleArrayValue = std::vector<SimpleValue>;

using StructValue = std::map<std::string, SimpleValue>;

struct ScalarValue
{
    SimpleValue value;
};

template <>
struct glz::meta<ScalarValue>
{
    using T = ScalarValue;
    static constexpr auto value = glz::object(&T::value);
};

struct ArrayValue
{
    SimpleArrayValue value;
};

template <>
struct glz::meta<ArrayValue>
{
    using T = ArrayValue;
    static constexpr auto value = glz::object(&T::value);
};

struct MapValue
{
    StructValue value;
};

template <>
struct glz::meta<MapValue>
{
    using T = MapValue;
    static constexpr auto value = glz::object(&T::value);
};

using MaterialUniformValue = std::variant<ScalarValue, ArrayValue, MapValue>;

template <>
struct glz::meta<MaterialUniformValue>
{
    static constexpr std::string_view tag = "kind";
    static constexpr auto ids = std::array{"scalar", "array", "map"};
};

using MaterialUniformValues = std::map<std::string, MaterialUniformValue>;

int main()
{
    std::println("Glaze study testbed");

    {
        MaterialUniformValues valMap;
        valMap["myInt"] = ScalarValue{IntValue{42}};
        valMap["myFloat"] = ScalarValue{FloatValue{3.14f}};
        valMap["myVec2"] = ScalarValue{glm::vec2{2, 3}};
        valMap["myArray"] = ArrayValue{std::vector<SimpleValue>{IntValue{1}, FloatValue{2.5f}, glm::vec2{3, 4}}};
        valMap["myMap"] = MapValue{StructValue{{"field1", IntValue{7}}, {"field2", glm::vec2{5, 6}}}};

        std::string buffer = glz::write_json(valMap).value_or("error");
        std::print("Serialized MaterialUniformValues:\n{}\n", buffer);

        auto expected = glz::read_json<MaterialUniformValues>(buffer);
        if (!expected)
        {
            std::println("Deserialization failed with error code: {}", (int)expected.error().ec);
            return 1;
        }
        MaterialUniformValues &deserValMap = expected.value();

        for (const auto &[key, var] : deserValMap)
        {
            if (std::holds_alternative<ScalarValue>(var))
            {
                const ScalarValue &v = std::get<ScalarValue>(var);
                std::visit(overloaded{
                               [&key](const IntValue &var)
                               { std::println("{}: {}", key, var.val); },
                               [&key](const FloatValue &var)
                               { std::println("{}: {}", key, var.val); },
                               [&key](const glm::vec2 &var)
                               { std::println("{}: ({},{})", key, var.x, var.y); },
                           },
                           v.value);
            }
            else
            {
                std::println("non-scalar key {}", key);
            }
        }
    }
    return 0;
}