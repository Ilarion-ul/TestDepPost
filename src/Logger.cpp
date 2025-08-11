#include "Logger.h"
#include <spdlog/sinks/basic_file_sink.h>

std::shared_ptr<spdlog::logger> slog::Logger::s_{};

void slog::Logger::init(Level lvl, const std::string& file)
{
    if (s_) return;
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file, true);
    sink->set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
    s_ = std::make_shared<spdlog::logger>("core", sink);
    spdlog::set_default_logger(s_);
    s_->set_level(toSpd(lvl));
}

spdlog::level::level_enum slog::Logger::toSpd(Level lvl)
{
    using E = spdlog::level::level_enum;
    switch (lvl) {
        case Level::Trace:    return E::trace;
        case Level::Debug:    return E::debug;
        case Level::Info:     return E::info;
        case Level::Warn:     return E::warn;
        case Level::Error:    return E::err;
        default:              return E::critical;
    }
}
