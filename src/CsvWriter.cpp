// src/CsvWriter.cpp
#include "CsvWriter.h"
#include <fstream>
#include <sstream>

void CsvWriter::write(const DepData& d,const std::string& fn){
    std::ofstream out(fn); if(!out) throw std::runtime_error("csv open");
    /* header */
    out<<"step,material,param";
    for(const auto& n:d.NAMES) out<<','<<n;
    out<<'\n';

    for(const auto& [step,matMap]: d.steps)
        for(const auto& [mat,paramMap]: matMap)
            for(const auto& [param,vec]: paramMap){
                out<<step<<','<<mat<<','<<param;
                for(double v: vec) out<<','<<v;
                out<<'\n';
            }
}