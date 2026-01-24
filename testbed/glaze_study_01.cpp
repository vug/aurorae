#include <print>

#include <glaze/glaze.hpp>

struct my_struct
{
    int i = 287;
    double d = 3.14;
    std::string hello = "Hello World";
    std::array<uint64_t, 3> arr = {1, 2, 3};
    std::map<std::string, int> map{{"one", 1}, {"two", 2}};
};

int main()
{
    std::print("Glaze study testbed");

    {
        my_struct s{};
        std::string buffer = glz::write_json(s).value_or("error");
        std::print("Serialized:\n{}\n", buffer);
    }

    {
        std::string buffer = R"({"i":287,"d":3.14,"hello":"Hello World","arr":[1,2,3],"map":{"one":1,"two":2}})";
        my_struct s = glz::read_json<my_struct>(buffer).value();
        std::print("Deserialized:\n{}\n", s.i);
    }

    return 0;
}