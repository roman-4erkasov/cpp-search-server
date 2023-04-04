#pragma once
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <iostream>
#include <iterator>

using namespace std;

#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

using namespace std;

template<typename T>
std::ostream &operator <<(std::ostream &os, const std::vector<T> &v) {
    using namespace std;
    os << "[";
    bool is_first = true;
    for(const T& elm: v){
        if(is_first) is_first = false;
        else os << ", ";
        os << elm;
    }
    //copy(v.begin(), v.end(), ostream_iterator<T>(os, ", "));
    os << "]";
    return os;
}

template<typename T>
std::ostream &operator <<(std::ostream &os, const std::set<T> &v) {
    using namespace std;
    os << "{";
    bool is_first = true;
    for(const T& elm: v){
        if(is_first) is_first = false;
        else os << ", ";
        os << elm;
    }
   //copy(v.begin(), v.end(), ostream_iterator<T>(os, ", "));
   os << "}";
   return os;
}

template<typename K, typename V>
std::ostream &operator <<(std::ostream &os, const std::map<K, V> &m) {
    using namespace std;
    os << "{";
    bool is_first = true;
    for(const auto& [key, value]: m){
        if(is_first) is_first = false;
        else os << ", ";
        os << key << ": " << value;
    }
    os << "}";
    return os;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

