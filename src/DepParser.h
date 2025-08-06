// src/DepParser.h
#pragma once
#include <vector>
#include <string>
#include <map>

struct DepData {
    std::vector<int>                ZAI;        ///< ідентифікатори ізотопів
    std::vector<std::string>        NAMES;      ///< їх назви ( + lost,total )
    /// step → material → param → значення (ізотопний або скалярний стовпчик)
    std::map<int,std::map<std::string,
              std::map<std::string,std::vector<double>>>> steps;
};

class DepParser {
public:
    void parseFile(const std::string& file);
    const DepData& getData() const { return data_; }
private:
    DepData       data_;
    size_t        nSteps_ = 0;
    void          storeMatrix(const std::string& material,
                              const std::string& param,
                              const std::vector<std::vector<double>>& rows);
};
