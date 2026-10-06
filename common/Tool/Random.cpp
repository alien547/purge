#include <random>
#include <chrono>
#include <cstdint>
#include "Random.h"
using namespace std;

static int gen_temp=0;
static thread_local mt19937_64 gen(chrono::high_resolution_clock::now().time_since_epoch().count()^uintptr_t(&gen_temp));

long long random(long long value_min, long long value_max){
    static uniform_int_distribution<long long> dist;
    return dist(gen, decltype(dist)::param_type{value_min, value_max});
}

float randomf(float value_min, float value_max){
    static uniform_real_distribution<float> dist(0.0f, 1.0f);
    return value_min+(value_max-value_min)*dist(gen);
}

