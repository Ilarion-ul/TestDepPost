// src/CsvWriter.h
#pragma once
#include "DepParser.h"
#include <string>

class CsvWriter{
public:
    /// формує файл: step,material,param,ізотопи...
    static void write(const DepData& d,const std::string& file);
};