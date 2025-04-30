#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <unordered_map>
#include <chrono>

void GetIrqWithDesc(const std::string fileName, std::unordered_map<int, std::string> &irqWithDesc)
{
    std::ifstream file(fileName);
    if (!file) {
        std::cerr << "Failed to open file." << std::endl;
        return;
    }

    std::string line;
    std::regex pattern(R"(^\s*(\d+):.*?\s+([a-zA-Z].*)$)");
    getline(file, line);
    while (std::getline(file, line)) {
        std::smatch match;
        if (std::regex_match(line, match, pattern)) {
            std::string irqNum = match[1].str();
            std::string desc = match[2].str();
            int value;
            try {
                value = std::stoi(irqNum);
            }
            catch (...) {
                continue;
            }
            irqWithDesc[value] = desc;
        }
    }

    file.close();
}

void GetIrqWithDesc1(const std::string fileName, std::unordered_map<int, std::string> &irqWithDesc)
{
    std::ifstream file(fileName);
    if (!file) {
        std::cerr << "Failed to open file." << std::endl;
        return;
    }

    std::string line;
    // read the first line (cpu header)
    if (!std::getline(file, line)) {
        std::cerr << "Empty file or failed to read the first line." << std::endl;
        return;
    }

    // calculate the width of the middle part (cpu irq)
    size_t descStart = line.find_last_not_of(' ');
    if (descStart == std::string::npos) {
        std::cerr << "Invalid format in the first line." << std::endl;
        return;
    }
    descStart++;

    while (std::getline(file, line)) {
        if (line.empty()) {
            break;
        }
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        int irqNum = 0;
        try {
            irqNum = std::stoi(line.substr(0, colonPos));
        } catch (...) {
            continue;
        }

        if (descStart >= line.size()) {
            continue; // not have desc
        }
        // delete space and middle cpu hirq
        std::string desc = line.substr(descStart);
        size_t firstNonSpace = desc.find_first_not_of(" \t");
        size_t lastNonSpace = desc.find_last_not_of(" \t");
        if (firstNonSpace != std::string::npos && lastNonSpace != std::string::npos) {
            desc = desc.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
        }

        irqWithDesc[irqNum] = desc;
    }

    file.close();
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <times>" << std::endl;
        return -1;
    }
    int times = atoi(argv[1]);
    std::string name = "/proc/interrupts";
    std::unordered_map<int, std::string> irqWithDesc;
    auto t1 = std::chrono::system_clock::now();
    for (int i = 0; i < times; i++) {
        GetIrqWithDesc(name, irqWithDesc);
    }
    auto t2 = std::chrono::system_clock::now();
    for (int i = 0; i < times; i++) {
        GetIrqWithDesc1(name, irqWithDesc);
    }
    auto t3 = std::chrono::system_clock::now();
    std::cout << "GetIrqWithDesc time cost: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "ms" << std::endl;
    std::cout << "GetIrqWithDesc1 time cost: " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << "ms" << std::endl;
    return 0;
}