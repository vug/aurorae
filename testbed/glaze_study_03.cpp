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

using MaterialUniformValues = std::map<std::string, SimpleValue>;

using SimpleArrayValue = std::vector<SimpleValue>;

using MaterialUniformValue = std::variant<SimpleValue>;

// using MaterialUniformValue = std::variant<SimpleValue, SimpleArrayValue>;
// template <>
// struct glz::meta<MaterialUniformValue>
// {
//     static constexpr std::string_view tag = "type";
//     static constexpr auto ids = std::array{"simple", "simple-array"};
// };

// using MaterialUniformValues = std::map<std::string, MaterialUniformValue>;

int main()
{
    std::println("Glaze study testbed");

    {
        MaterialUniformValues valMap;
        valMap["myInt"] = IntValue{42};
        valMap["myFloat"] = FloatValue{3.14f};
        valMap["myVec2"] = glm::vec2{2, 3};

        std::string buffer = glz::write_json(valMap).value_or("error");
        std::print("Serialized MaterialUniformValues:\n{}\n", buffer);

        auto expected = glz::read_json<MaterialUniformValues>(buffer);
        if (!expected)
        {
            std::println("Deserialization failed");
            return 1;
        }
        MaterialUniformValues &deserValMap = expected.value();
        for (const auto &[key, var] : deserValMap)
        {
            std::println("key {}", key);
            // std::visit(overloaded{
            //                [&key](const IntValue &var)
            //                { std::println("{}: {}", key, var.val); },
            //                [&key](const FloatValue &var)
            //                { std::println("{}: {}", key, var.val); },
            //                [&key](const glm::vec2 &var)
            //                { std::println("{}: ({},{})", key, var.x, var.y); },
            //            },
            //            var);
        }
    }
    return 0;
}