#include <fstream>
#include "helpers.hpp"

std::string read_line(const std::string& filename)
{
    std::string line;
    std::ifstream file(filename);

    if (file.fail())
        return line;

    std::getline(file, line);
    return line;
}
