#include <cstring>
#include "Network.h"
using namespace std;

void PlayerJoin::make(const std::string& name, const std::string& country, const std::string& province, uint64_t id){
    strncpy(this->name, name.c_str(), MAX_HUMAN_NAME_SIZE-1);
    this->name[MAX_HUMAN_NAME_SIZE-1]='\0';
    strncpy(this->country, country.c_str(), MAX_LOCATION_NAME_SIZE-1);
    this->country[MAX_LOCATION_NAME_SIZE-1]='\0';
    strncpy(this->province, province.c_str(), MAX_LOCATION_NAME_SIZE-1);
    this->province[MAX_LOCATION_NAME_SIZE-1]='\0';
    this->id=id;
}

