#pragma once

#include <string>
#include <vector>
#include <fstream>

class FileReader {
public:
    static std::string ReadFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) return "";

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string content(size, '\0');
        file.read(&content[0], size);
        return content;
    }

    static std::vector<uint8_t> ReadFileBytes(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) return {};

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> bytes(size);
        file.read(reinterpret_cast<char*>(bytes.data()), size);
        return bytes;
    }
};
