#ifndef PROCESS_H
#define PROCESS_H

#include <string>

void open_url(const std::string& url);
bool start_server_process(const std::string& server_path, const std::string& args);

#endif

