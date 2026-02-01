#include <print>
#include <string>
#include <map>

#include <glaze/glaze.hpp>

using Map = std::map<std::string, int>;

int main()
{
    std::print("Glaze study testbed");

    {
        Map m{{"apple", 5}, {"banana", 10}};
        std::string buffer = glz::write_json(m).value_or("error");
        std::print("Serialized map:\n{}\n", buffer);
    }

    {
        std::string buffer = R"({"apple":5,"banana":10})";
        Map m = glz::read_json<Map>(buffer).value();
        std::print("Deserialized map:\n{}\n", m["banana"]);
    }

    return 0;
}