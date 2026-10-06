#ifndef FORM_TEXT_INPUT_H
#define FORM_TEXT_INPUT_H

#include <string>
#include <functional>

std::string read_input(int x, int y, int max_len=100, const std::string& default_value="", std::function<bool(SDL_KeyboardEvent&, std::string&)> on_keydown=nullptr);
int8_t prompt_yes_no();
int prompt_choice(int choice_size);

#endif

