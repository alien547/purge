#include "String.h"
using namespace std;

int safe_stoi(const string& str, int default_value){
    try{
        return stoi(str);
    }catch(const exception&){
        return default_value;
    }
}

float safe_stof(const string& str, float default_value){
    try{
        return stof(str);
    }catch(const exception&){
        return default_value;
    }
}

