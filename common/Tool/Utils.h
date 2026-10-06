#ifndef UTILS_H
#define UTILS_H

template<typename Map, typename K>
typename Map::mapped_type safe_map_find(const Map& map, const K& key, const typename Map::mapped_type& default_value=typename Map::mapped_type()){
    auto it=map.find(key);
    if(it!=map.end())return it->second;
    return default_value;
}

template<typename T>
bool assign_if_changed(T& dest, const T& new_value){
    if(dest!=new_value){
        dest=new_value;
        return true;
    }
    return false;
}

#endif

