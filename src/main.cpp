#include "DepParser.h"
#include "CsvWriter.h"
#include "Logger.h"
#include <iostream>

int main(int argc,char* argv[])
{
    if(argc<3){
        std::cerr<<"Usage: "<<argv[0]<<" <dep.m> <out.csv>\n"; return 1;
    }
    log::Logger::init(log::Level::Info);

    try{
        DepParser p; p.parseFile(argv[1]);
        CsvWriter::write(p.getData(),argv[2]);

        std::cout<<"Done. Steps: "<<p.getData().steps.size()
                 <<", isotopes: "<<p.getData().ZAI.size()<<'\n';
    }catch(const std::exception& ex){
        log::Logger::error("Fatal: {}",ex.what());
        std::cerr<<"Error: "<<ex.what()<<'\n'; return 2;
    }
    return 0;
}
