#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPS = 1e-6;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }
    return words;
}

struct Document
{
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer
{
public:
    void SetStopWords(const string &text)
    {
        for (const string &word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    void AddDocument(
        int document_id,
        const string &document,
        DocumentStatus status,
        const vector<int> &ratings)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string &word : words)
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    vector<Document> FindTopDocuments(
        const string &raw_query,
        DocumentStatus status) const
    {
        return FindTopDocuments(
            raw_query,
            [status](
                int doc_id,
                DocumentStatus doc_status,
                int rating)
            {
                return doc_status == status;
            });
    }

    vector<Document> FindTopDocuments(
        const string &raw_query) const
    {
        return FindTopDocuments(
            raw_query,
            [](
                int doc_id,
                DocumentStatus doc_status,
                int rating)
            {
                return doc_status == DocumentStatus::ACTUAL;
            });
    }

    template <typename Predicat>
    vector<Document> FindTopDocuments(
        const string &raw_query,
        Predicat predicat) const
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(
            query, predicat);
        sort(
            matched_documents.begin(), matched_documents.end(),
            [](const Document &lhs, const Document &rhs)
            {
                if (abs(lhs.relevance - rhs.relevance) < EPS)
                {
                    return lhs.rating > rhs.rating;
                }
                else
                {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    int GetDocumentCount() const
    {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(
        const string &raw_query,
        int document_id) const
    {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string &word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.push_back(word);
            }
        }
        for (const string &word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string &word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string &text) const
    {
        vector<string> words;
        for (const string &word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int> &ratings)
    {
        if (ratings.empty())
        {
            return 0;
        }
        int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord
    {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const
    {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-')
        {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string &text) const
    {
        Query query;
        for (const string &word : SplitIntoWords(text))
        {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop)
            {
                if (query_word.is_minus)
                {
                    query.minus_words.insert(query_word.data);
                }
                else
                {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string &word) const
    {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Predicat>
    vector<Document> FindAllDocuments(
        const Query &query,
        Predicat predicat) const
    {
        map<int, double> document_to_relevance;
        for (const string &word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                const DocumentData doc = documents_.at(document_id);
                if (
                    predicat(document_id, doc.status, doc.rating))
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string &word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word))
            {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

// ==================== для примера =========================

void PrintDocument(const Document &document)
{
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

/////////////////// TESTS //////////////////////////

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v)
{
    using namespace std;
    os << "[";
    bool is_first = true;
    for (const T &elm : v)
    {
        if (is_first)
            is_first = false;
        else
            os << ", ";
        os << elm;
    }
    // copy(v.begin(), v.end(), ostream_iterator<T>(os, ", "));
    os << "]";
    return os;
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::set<T> &v)
{
    using namespace std;
    os << "{";
    bool is_first = true;
    for (const T &elm : v)
    {
        if (is_first)
            is_first = false;
        else
            os << ", ";
        os << elm;
    }
    // copy(v.begin(), v.end(), ostream_iterator<T>(os, ", "));
    os << "}";
    return os;
}

template <typename K, typename V>
std::ostream &operator<<(std::ostream &os, const std::map<K, V> &m)
{
    using namespace std;
    os << "{";
    bool is_first = true;
    for (const auto &[key, value] : m)
    {
        if (is_first)
            is_first = false;
        else
            os << ", ";
        os << key << ": " << value;
    }
    os << "}";
    return os;
}

template <typename T, typename U>
void AssertEqualImpl(const T &t, const U &u, const string &t_str, const string &u_str, const string &file,
                     const string &func, unsigned line, const string &hint)
{
    if (t != u)
    {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty())
        {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string &expr_str, const string &file, const string &func, unsigned line,
                const string &hint)
{
    if (!value)
    {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty())
        {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

vector<int> TakeEvens(const vector<int> &numbers)
{
    vector<int> evens;
    for (int x : numbers)
    {
        if (x % 2 == 0)
        {
            evens.push_back(x);
        }
    }
    return evens;
}

template <typename F>
void RunTestImpl(F func, string func_name)
{
    func();
    cerr << func_name << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document &doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/

void TestExcludeMinusWordsFromAddedDocument()
{
    const int doc_id = 1337;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document &doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in -cat"s);
        ASSERT_EQUAL(found_docs.size(), 0);
        ASSERT(server.FindTopDocuments("in -cat"s).empty());
    }
}

int ComputeAverageRating(const vector<int> &ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    // int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    int rating_sum = 0;
    for (int rating : ratings)
    {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

void TestRatingComputation()
{
    const int doc_id = 1337;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document &doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.rating, ComputeAverageRating(ratings));
    }
}

void TestRelevanceOrder()
{
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(
            1,
            "white cat in the city",
            DocumentStatus::ACTUAL,
            ratings);
        server.AddDocument(
            2,
            "black cat in the village",
            DocumentStatus::ACTUAL,
            ratings);
        server.AddDocument(
            3,
            "red cat in the house",
            DocumentStatus::ACTUAL,
            ratings);
        const vector<Document> found = server.FindTopDocuments(
            "black cat village"s);
        string msg = "found.size() = " + to_string(found.size());
        // ASSERT_EQUAL_HINT(found.size(), 3, msg.c_str());
        ASSERT_EQUAL(found.size(), 3);
        ASSERT(
            is_sorted(
                found.begin(),
                found.end(),
                [](const Document &left, const Document &right)
                {
                    return right.relevance < left.relevance;
                }));
    }
}

void TestPredicatFiltering()
{
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(
            1,
            "white cat in the city",
            DocumentStatus::ACTUAL,
            {3, 3, 3});
        server.AddDocument(
            2,
            "black cat in the village",
            DocumentStatus::ACTUAL,
            {1, 1, 1});
        server.AddDocument(
            3,
            "red cat in the village",
            DocumentStatus::ACTUAL,
            {4, 5, 6});
        const vector<Document> found = server.FindTopDocuments(
            "black cat village"s,
            [](
                [[maybe_unused]] int doc_id,
                [[maybe_unused]] DocumentStatus doc_status,
                int rating)
            {
                return 2 < rating;
            });
        ASSERT_EQUAL(found.size(), 2);
        ASSERT_EQUAL(found[0].id, 3);
        ASSERT_EQUAL(found[1].id, 1);
    }
}

void TestStatusFiltering()
{
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(
            1,
            "white cat in the city",
            DocumentStatus::BANNED,
            {3, 3, 3});
        server.AddDocument(
            2,
            "black cat in the village",
            DocumentStatus::ACTUAL,
            {1, 1, 1});
        server.AddDocument(
            3,
            "red cat in the village",
            DocumentStatus::BANNED,
            {4, 5, 6});
        const vector<Document> found = server.FindTopDocuments(
            "black cat village"s,
            DocumentStatus::BANNED);
        ASSERT_EQUAL(found.size(), 2);
        ASSERT_EQUAL(found[0].id, 3);
        ASSERT_EQUAL(found[1].id, 1);
    }
}

vector<string> SplitText(const string &text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }
    return words;
}

double ComputeWordInverseDocumentFreq(
    const string &word,
    int doc_count,
    map<string, map<int, double>> word_to_document_freqs_)
{
    return log(doc_count * 1.0 / word_to_document_freqs_.at(word).size());
}

void TestRelevanceComputation()
{
    string query = "black cat village"s;
    vector<pair<int, string>> documents = {
        {1, "white cat in the city"s},
        {2, "black cat in the village"s},
        {3, "red cat in the village"s}};
    map<string, map<int, double>> word_to_document_freqs_;
    for (const auto &[document_id, doc] : documents)
    {
        const vector<string> words = SplitText(doc);
        const double inv_word_count = 1.0 / words.size();
        for (const string &word : words)
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
    }

    map<int, double> document_to_relevance;
    for (const string &word : SplitText(query))
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(
            word, 3, word_to_document_freqs_);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
        {
            document_to_relevance[document_id] += term_freq * inverse_document_freq;
        }
    }

    SearchServer server;
    for (const auto &[doc_id, text] : documents)
    {
        server.AddDocument(
            doc_id,
            text,
            DocumentStatus::ACTUAL,
            {1, 1, 1});
    }
    const vector<Document> found = server.FindTopDocuments(
        query, DocumentStatus::ACTUAL);
    for (const Document &doc : found)
    {
        ASSERT_EQUAL(doc.relevance, document_to_relevance.at(doc.id));
    }
}

/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer()
{
    TestExcludeStopWordsFromAddedDocumentContent();
    // Не забудьте вызывать остальные тесты здесь
    TestExcludeMinusWordsFromAddedDocument();
    TestRatingComputation();
    TestRelevanceOrder();
    TestPredicatFiltering();
    TestStatusFiltering();
    TestRelevanceComputation();
}

int main()
{
    TestSearchServer();
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL,
                              {5, -12, 2, 1});

    for (const Document &document : search_server.FindTopDocuments("ухоженный кот"s))
    {
        PrintDocument(document);
    }

    return 0;
}
