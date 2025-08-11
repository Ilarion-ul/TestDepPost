#pragma once
#include <vector>
#include <string>
#include <map>

struct DepData {
    std::vector<int>                ZAI;
    std::vector<std::string>        NAMES;
    std::map<int,
      std::map<std::string,
        std::map<std::string, std::vector<double>>>> steps;
    std::vector<double> BU;     // burnup per step
    std::vector<double> DAYS;   // days per step
};

class DepParser {
public:
    void parseFile(const std::string& file);
    const DepData& getData() const { return data_; }

private:
    DepData data_;
    size_t  nSteps_ = 0;

    static std::vector<double> parseNumbersLine(const std::string& l);
    void storeMatrix(const std::string& mat,
                     const std::string& param,
                     const std::vector<std::vector<double>>& rows);
};
