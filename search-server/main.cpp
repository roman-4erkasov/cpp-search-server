
#include "search_server.h"
#include "log_duration.h"
#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}
vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}
string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}
vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}
template <typename ExecutionPolicy>
void Test(string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << total_relevance << endl;
}
#define TEST(policy) Test(#policy, search_server, queries, execution::policy)
int main() {
    mt19937 generator;
    const auto dictionary = GenerateDictionary(generator, 1000, 10);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);
    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
    }
    const auto queries = GenerateQueries(generator, dictionary, 100, 70);
    TEST(seq);
    TEST(par);
}


/*
#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include "process_queries.h"
#include "search_server.h"
#include <execution>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

int main() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const string& text : {
            "white cat and yellow hat"s,
            "curly cat curly tail"s,
            "nasty dog with big eyes"s,
            //"nasty pigeon john"s,
            
	    //"white cat and yellow hat"s,
            //"curly cat curly tail"s,
            //"nasty dog with big eyes"s,
            "nasty pigeon john"s
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }
    cout << "ACTUAL by default:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "seq:"s << endl;
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "par:"s << endl;
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s)) {
        PrintDocument(document);
    }
    //cout << "BANNED:"s << endl;
    // последовательная версия
    //for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
    //    PrintDocument(document);
    //}
    //cout << "Even ids:"s << endl;
    // параллельная версия
    //for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
    //    PrintDocument(document);
    //}
    return 0;
}
*/

/*
#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <memory>

#include "log_duration.h"
#include "test_framework.h"

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
        
        //Access(const Key& key, Bucket& bucket) 
        //: guard(bucket.mx_), ref_to_value(bucket.map_[key])
        //{}
    };
    
    

    explicit ConcurrentMap(size_t bucket_count);

    Access operator[](const Key& key);

    std::map<Key, Value> BuildOrdinaryMap();

private:
    // ...
    //std::vector<std::mutex> mutexes_;
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
    //auto& bucket = buckets_[
    //    static_cast<size_t>(key) % buckets_.size()
    //];
    //if(0==buckets_[bucket].map_.count(key)){
    //    std::lock_guard guard(buckets_[bucket].mx_);
    //    buckets_[bucket].map_.insert({key, Value()});
    //}
    size_t bucket = static_cast<uint64_t>(key) % buckets_.size();
    return {
        std::lock_guard<std::mutex>(buckets_[bucket].mx_),
        buckets_[bucket].map_[key]
    };
    //return {key, bucket};
}

template <typename Key, typename Value>
std::map<Key, Value> 
ConcurrentMap<Key, Value>::BuildOrdinaryMap() {
    std::map<Key, Value> result;
    for(auto& bucket: buckets_){
        std::lock_guard g(bucket.mx_);
        result.insert(
            bucket.map_.begin(),
            bucket.map_.end()
        );
    }
    return result;
}
*/
/*using namespace std;
template <typename K, typename V>
class ConcurrentMap {
public:
  static_assert(is_integral_v<K>, "ConcurrentMap supports only integer keys");

  struct Access
  {
    lock_guard<mutex> lock;
    V& ref_to_value;
  };

  explicit ConcurrentMap(size_t bucket_count) : segments(bucket_count) {}

  Access operator[](const K& key)
  {
    Segment& seg = segments[GetSegmentByKey(key)]; //?
    return {lock_guard<mutex> (seg.m_), seg.submap[key]};
  }

  map<K, V> BuildOrdinaryMap()
  {
    map<K, V> res;
    for (auto &s : segments)
     {
      lock_guard<mutex> g(s.m_);
      for (auto& [key, value] : s.submap)
       {
         res[key] = value;
       }
     }
     return res;
  }
private:
  struct Segment
  {
    //Segment () {lock_guard<mutex> g(m_);}
    map <K, V> submap;
    mutex m_;
  };
  vector<Segment> segments;
  size_t GetSegmentByKey(K key) {
    K abs = key >= 0 ? key : -key;
    return abs % segments.size();
  }
};*/

/*
using namespace std;

void RunConcurrentUpdates(ConcurrentMap<int, int>& cm, size_t thread_count, int key_count) {
    auto kernel = [&cm, key_count](int seed) {
        vector<int> updates(key_count);
        iota(begin(updates), end(updates), -key_count / 2);
        shuffle(begin(updates), end(updates), mt19937(seed));

        for (int i = 0; i < 2; ++i) {
            for (auto key : updates) {
                ++cm[key].ref_to_value;
            }
        }
    };

    vector<future<void>> futures;
    for (size_t i = 0; i < thread_count; ++i) {
        futures.push_back(async(kernel, i));
    }
}

void TestConcurrentUpdate() {
    constexpr size_t THREAD_COUNT = 3;
    constexpr size_t KEY_COUNT = 50000;

    ConcurrentMap<int, int> cm(THREAD_COUNT);
    RunConcurrentUpdates(cm, THREAD_COUNT, KEY_COUNT);

    const auto result = cm.BuildOrdinaryMap();
    ASSERT_EQUAL(result.size(), KEY_COUNT);
    for (auto& [k, v] : result) {
        AssertEqual(v, 6, "Key = " + to_string(k));
    }
}

void TestReadAndWrite() {
    ConcurrentMap<size_t, string> cm(5);

    auto updater = [&cm] {
        for (size_t i = 0; i < 50000; ++i) {
            cm[i].ref_to_value.push_back('a');
        }
    };
    auto reader = [&cm] {
        vector<string> result(50000);
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] = cm[i].ref_to_value;
        }
        return result;
    };

    auto u1 = async(updater);
    auto r1 = async(reader);
    auto u2 = async(updater);
    auto r2 = async(reader);

    u1.get();
    u2.get();

    for (auto f : {&r1, &r2}) {
        auto result = f->get();
        ASSERT(all_of(result.begin(), result.end(), [](const string& s) {
            return s.empty() || s == "a" || s == "aa";
        }));
    }
}

void TestSpeedup() {
    {
        ConcurrentMap<int, int> single_lock(1);

        LOG_DURATION("Single lock");
        RunConcurrentUpdates(single_lock, 4, 50000);
    }
    {
        ConcurrentMap<int, int> many_locks(100);

        LOG_DURATION("100 locks");
        RunConcurrentUpdates(many_locks, 4, 50000);
    }
}

int main() {
    TestRunner tr;
    RUN_TEST(tr, TestConcurrentUpdate);
    RUN_TEST(tr, TestReadAndWrite);
    RUN_TEST(tr, TestSpeedup);
}*/
