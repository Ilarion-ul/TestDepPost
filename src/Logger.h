#pragma once
#include <spdlog/spdlog.h>
#include <memory>

namespace slog {

enum class Level { Trace, Debug, Info, Warn, Error, Critical };

class Logger {
public:
    static void init(Level lvl = Level::Info,
                     const std::string& file = "serpent.log");

    template<typename... A>
    static void info (fmt::format_string<A...> f, A&&... a) { s_->info (f,std::forward<A>(a)...); }
    template<typename... A>
    static void warn (fmt::format_string<A...> f, A&&... a) { s_->warn (f,std::forward<A>(a)...); }
    template<typename... A>
    static void error(fmt::format_string<A...> f, A&&... a) { s_->error(f,std::forward<A>(a)...); }

private:
    static std::shared_ptr<spdlog::logger> s_;   ///< визначення – у Logger.cpp
    static spdlog::level::level_enum toSpd(Level);
};

} // namespace slog
