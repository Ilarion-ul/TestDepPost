// src/Logger.cpp
#include "Logger.h"
#include <spdlog/sinks/basic_file_sink.h>

void log::Logger::init(Level lvl,const std::string& file){
    if (s_) return;
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file,true);
    sink->set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
    s_ = std::make_shared<spdlog::logger>("core",sink);
    spdlog::set_default_logger(s_);
    s_->set_level(toSpd(lvl));
}
spdlog::level::level_enum log::Logger::toSpd(Level L){
    using spd = spdlog::level::level_enum;
    switch(L){case Level::Trace: return spd::trace;case Level::Debug:return spd::debug;
        case Level::Info:return spd::info;case Level::Warn:return spd::warn;
        case Level::Error:return spd::err;default:return spd::critical;}
}