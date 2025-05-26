#include "my_pipeline.h"

// std
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

MyPipeline::MyPipeline(const std::string& vertFilepath, const std::string& fragFilepath)
{
    _createGraphicsPipeline(vertFilepath, fragFilepath);
}

std::vector<char> MyPipeline::_readFile(const std::string& filename)
{
    std::ifstream file{ filename, std::ios::ate | std::ios::binary };

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

void MyPipeline::_createGraphicsPipeline(
    const std::string& vertFilepath, const std::string& fragFilepath) 
{
    auto vertCode = _readFile(vertFilepath);
    auto fragCode = _readFile(fragFilepath);

    std::cout << "Vertex Shader Code Size: " << vertCode.size() << '\n';
    std::cout << "Fragment Shader Code Size: " << fragCode.size() << '\n';
}

