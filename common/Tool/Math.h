#ifndef MATH_H
#define MATH_H

constexpr float PI=3.1415926f, EPSILON=0.05f;

namespace Math{
    template<typename T>
    constexpr const T& min(const T& a, const T& b){
        return a<b?a:b;
    }

    template<typename T>
    constexpr const T& max(const T& a, const T& b){
        return a<b?b:a;
    }

    template<typename T>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi){
        return v<lo?lo:(hi<v?hi:v);
    }
}

#endif

