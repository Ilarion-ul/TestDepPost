#include "DepParser.h"
#include "CsvWriter.h"
#include "Logger.h"
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc < 3) { std::cerr << "Usage: " << argv[0] << " <dep.m> <outDir>\n"; return 1; }

    slog::Logger::init(slog::Level::Info);

    try {
        DepParser parser;  parser.parseFile(argv[1]);

        std::filesystem::path outDir = argv[2];
        CsvWriter::writePerMaterial(parser.getData(), outDir);

        std::cout << "Done. Steps: " << parser.getData().steps.size()
                  << ", isotopes: " << parser.getData().ZAI.size() << '\n';
    }
    catch (const std::exception& ex) {
        slog::Logger::error("Fatal: {}", ex.what());
        std::cerr << ex.what() << '\n';  return 2;
    }
}
