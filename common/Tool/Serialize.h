#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <vector>
#include <cstring>
#include <cstdint>
#include <lz4.h>

template<typename Stream>
void save_string(Stream& file, const std::string& str){
    uint32_t len=str.size();
    file.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
    if(len>0)file.write(str.data(), len);
}

template<typename Stream>
void load_string(Stream& file, std::string& str){
    uint32_t len=0;
    file.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));
    str.resize(len);
    if(len>0)file.read(&str[0], len);
}

template<typename Stream>
void save_value(Stream&){}

template<typename Stream>
void load_value(Stream&){}

template<typename Stream, typename T, typename... Data>
void save_value(Stream& file, const T& first, const Data&... data){
    file.write(reinterpret_cast<const char*>(&first), sizeof(T));
    save_value(file, data...);
}
template<typename Stream, typename T, typename... Data>
void load_value(Stream& file, T& first, Data&... data){
    file.read(reinterpret_cast<char*>(&first), sizeof(T));
    load_value(file, data...);
}

static std::vector<char> compress_raw(const char* data, int size){
    int max_comp=LZ4_compressBound(size);
    std::vector<char> comp(max_comp);
    int comp_size=LZ4_compress_default(data, comp.data(), size, max_comp);
    comp.resize(comp_size);
    std::vector<char> packet(4+comp_size);
    memcpy(packet.data(), &size, 4);
    memcpy(packet.data()+4, comp.data(), comp_size);
    return packet;
}

static bool decompress_raw(const std::vector<char>& packet, char* out, int& out_size){
    if(packet.size()<4)return false;
    int orig_size;
    memcpy(&orig_size, packet.data(), 4);
    if(orig_size<=0)return false;
    out_size=orig_size;
    int ret=LZ4_decompress_safe(packet.data()+4, out, static_cast<int>(packet.size()-4), orig_size);
    return ret>=0;
}

inline std::vector<char> compress_data(const void* data, int size){
    return compress_raw(static_cast<const char*>(data), size);
}

template<typename T>
std::vector<char> compress_data(const T& data){
    return compress_raw(reinterpret_cast<const char*>(&data), sizeof(T));
}

inline bool decompress_data(const std::vector<char>& packet, std::string& out){
    int out_size, orig_size;
    memcpy(&orig_size, packet.data(), 4);
    if(orig_size<=0)return false;
    out.resize(orig_size);
    if(!decompress_raw(packet, &out[0], out_size))return false;
    out.resize(out_size);
    return true;
}

template<typename T>
bool decompress_data(const std::vector<char>& packet, T& data){
    int out_size;
    if(!decompress_raw(packet, reinterpret_cast<char*>(&data), out_size))return false;
    return true;
}

#endif

