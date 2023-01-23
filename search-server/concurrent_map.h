#pragma once

#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>

template<typename Key, typename Value>
class ConcurrentMap
{
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");
    struct Access
    {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
        std::map<Key, Value>& ref_to_map;
        explicit Access(std::map<Key, Value>& ordinary_map, const Key& key, std::mutex& m) : guard(m), ref_to_value(ordinary_map[key]), ref_to_map(ordinary_map) {}
    };
    
    explicit ConcurrentMap(size_t bucket_count) : bucket_count_(bucket_count), locks_(bucket_count), map_collection_(
            bucket_count) {}
    
    Access operator[](const Key& key)
    {
        uint64_t number_map = key % bucket_count_;
        return Access(map_collection_[number_map], key, locks_[number_map]);
    }
    
    std::map<Key, Value> BuildOrdinaryMap()
    {
        std::map<Key, Value> ordinary_map;
        for (uint64_t i = 0; i < bucket_count_; ++i) {
            std::lock_guard guard(locks_[i]);
            ordinary_map.merge(map_collection_[i]);
        }
        return ordinary_map;
    }

private:
    uint64_t bucket_count_;
    std::vector<std::mutex> locks_;
    std::vector<std::map<Key, Value>> map_collection_;
};
