#include <print>
#include <string>
#include <map>

#include <glaze/glaze.hpp>

using i32 = int32_t;
using f32 = float;

struct IntValue
{
    i32 val;
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
};

template <>
struct glz::meta<FloatValue>
{
    using T = FloatValue;
    static constexpr auto value = glz::object(&T::val);
};

using SimpleValue = std::variant<IntValue, FloatValue>;

template <>
struct glz::meta<SimpleValue>
{
    static constexpr std::string_view tag = "type";
    static constexpr auto ids = std::array{"int", "float"};
};

using MaterialUniformValues = std::map<std::string, SimpleValue>;

int main()
{
    std::println("Glaze study testbed");

    {
        MaterialUniformValues valMap;
        valMap["myInt"] = IntValue{42};
        valMap["myFloat"] = FloatValue{3.14f};

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
            std::visit([&key](const auto &var)
                       { std::println("{}: {}", key, var.val); }, var);
        }
    }
    return 0;
}