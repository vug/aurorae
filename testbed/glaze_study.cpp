#include <print>
#include <string>
#include <map>

#include <glaze/glaze.hpp>

using i32 = int32_t;
using f32 = float;

// struct my_struct
// {
//     int i = 287;
//     double d = 3.14;
//     std::string hello = "Hello World";
//     std::array<uint64_t, 3> arr = {1, 2, 3};
//     std::map<std::string, int> map{{"one", 1}, {"two", 2}};
// };

// using Map = std::map<std::string, int>;

struct IntValue
{
    i32 data;
};

template <>
struct glz::meta<IntValue>
{
    using T = IntValue;
    static constexpr auto value = glz::object(&T::data);
};

struct FloatValue
{
    f32 data;
};

template <>
struct glz::meta<FloatValue>
{
    using T = FloatValue;
    static constexpr auto value = glz::object(&T::data);
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
    std::print("Glaze study testbed");

    // {
    //     my_struct s{};
    //     std::string buffer = glz::write_json(s).value_or("error");
    //     std::print("Serialized:\n{}\n", buffer);
    // }

    // {
    //     std::string buffer = R"({"i":287,"d":3.14,"hello":"Hello World","arr":[1,2,3],"map":{"one":1,"two":2}})";
    //     my_struct s = glz::read_json<my_struct>(buffer).value();
    //     std::print("Deserialized:\n{}\n", s.i);
    // }

    // {
    //     Map m{{"apple", 5}, {"banana", 10}};
    //     std::string buffer = glz::write_json(m).value_or("error");
    //     std::print("Serialized map:\n{}\n", buffer);
    // }

    // {
    //     std::string buffer = R"({"apple":5,"banana":10})";
    //     Map m = glz::read_json<Map>(buffer).value();
    //     std::print("Deserialized map:\n{}\n", m["banana"]);
    // }

    {
        MaterialUniformValues vals;
        vals["myInt"] = IntValue{42};
        vals["myFloat"] = FloatValue{3.14f};

        std::string buffer = glz::write_json(vals).value_or("error");
        std::print("Serialized MaterialUniformValues:\n{}\n", buffer);

        auto expected = glz::read_json<MaterialUniformValues>(buffer);
        if (expected)
        {
            MaterialUniformValues &deserializedVals = expected.value();
            std::print("Deserialized MaterialUniformValues:\nmyInt: {}, myFloat: {}\n",
                       std::get<IntValue>(deserializedVals["myInt"]).data,
                       std::get<FloatValue>(deserializedVals["myFloat"]).data);
        }
        else
        {
            std::print("Deserialization failed\n");
        }
    }
    return 0;
}