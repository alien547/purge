#ifndef FORM_H
#define FORM_H

#include <functional>
#include <vector>
#include <string>

constexpr short
FORM_TEXT   = 0,
FORM_NUM    = 1,
FORM_SELECT = 2,
FORM_CHECK  = 3;

struct FormField{
    std::string label, value;
    short type=0;
    float low=0, high=0;
    std::vector<std::string> options;
    int select=0;
    std::string def, err;
};

class Form{
    public:
        Form(const std::string& t="");

        std::vector<FormField> fields;
        int focus=0;
        std::string title;
        std::function<void(Form&)> on_submit;

        Form& add(const std::string& label, short type, const std::string& value="", float low=0, float high=0, std::vector<std::string> options={}, int select=0);
        std::string get(const std::string& l)const;
        int int_get(const std::string& l)const;
        float float_get(const std::string& l)const;
        bool check_get(const std::string& l)const;
        int idx(const std::string& l)const;
        void reset();
        bool validate(FormField& f);
        bool run(int x, int y);
};

#endif

