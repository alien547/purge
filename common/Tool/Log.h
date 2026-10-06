#ifndef LOG_H
#define LOG_H

#include <string>

constexpr short
DEBUG_INFO  = 0,
DEBUG_ERROR = 1;

void set_log_path(const std::string& path);
void flush_log_to_file();
void debug(const std::string& message, int type);

#endif

