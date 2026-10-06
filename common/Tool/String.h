#ifndef STRING_H
#define STRING_H

#include <string>

int safe_stoi(const std::string& str, int default_value=0);
float safe_stof(const std::string& str, float default_value=0.0f);

#endif

