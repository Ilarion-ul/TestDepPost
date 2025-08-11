// src/CsvWriter.h
#pragma once
#include "DepParser.h"
#include <filesystem>
#include <string>

class CsvWriter {
public:
    static void writePerMaterial(const DepData& d,
                                 const std::filesystem::path& outDir);
};