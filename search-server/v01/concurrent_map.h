#pragma once
#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <memory>

const size_t N_BUCKETS_DEFAULT = 20;

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);
    
    struct Bucket {
        std::mutex mx_;
        std::map<Key, Value> map_;
    };

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count=N_BUCKETS_DEFAULT);
    Access operator[](const Key& key);
    std::map<Key, Value> BuildOrdinaryMap();
    void erase(const Key& key);

private:
    size_t bucket_count_;
    std::vector<Bucket> buckets_;
};

template <typename Key, typename Value>
ConcurrentMap<Key, Value>::ConcurrentMap(size_t bucket_count)
:bucket_count_(bucket_count), buckets_(bucket_count)
{ }

template <typename Key, typename Value>
typename ConcurrentMap<Key, Value>::Access 
ConcurrentMap<Key, Value>::operator[](const Key& key)
{
    size_t bucket = static_cast<uint64_t>(key) % buckets_.size();
    return {
        std::lock_guard<std::mutex>(buckets_[bucket].mx_),
        buckets_[bucket].map_[key]
    };
}

template <typename Key, typename Value>
void ConcurrentMap<Key, Value>::erase(const Key& key) {
    size_t bucket = static_cast<uint64_t>(key) % buckets_.size();
    {
        std::lock_guard<std::mutex>(buckets_[bucket].mx_);
        buckets_[bucket].map_.erase(key);
    }
}

template <typename Key, typename Value>
std::map<Key, Value> 
ConcurrentMap<Key, Value>::BuildOrdinaryMap() {
    std::map<Key, Value> result;
    for(auto& bucket: buckets_){
        std::lock_guard g(bucket.mx_);
        result.insert(bucket.map_.begin(),bucket.map_.end());
    }
    return result;
}
