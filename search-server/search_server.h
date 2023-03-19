#pragma once
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <execution>

#include "document.h"
#include "string_processing.h"
#include "paginator.h"

#define EPS 1e-6

const int MAX_RESULT_DOCUMENT_COUNT = 5;


using namespace std;



class SearchServer {
public:
    explicit SearchServer(std::string const& str);
    explicit SearchServer(std::string_view str);
    template <typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);
    //explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(
        int document_id, 
        const std::string& document, 
        DocumentStatus status,
        const std::vector<int>& ratings
    );
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
        const std::string &raw_query,
        DocumentPredicate document_predicate
    ) const;

    
    std::vector<Document> FindTopDocuments(
        const std::string &raw_query, DocumentStatus status
    ) const;
    std::vector<Document> FindTopDocuments(const std::string &raw_query) const;
    
    int GetDocumentCount() const;
    
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(
        const std::string &raw_query, int document_id
    ) const;
    tuple<vector<string>, DocumentStatus> MatchDocument(
        const std::execution::sequenced_policy& policy,
        const string &raw_query,
        int document_id
    ) const;
    tuple<vector<string>, DocumentStatus> MatchDocument(
        const std::execution::parallel_policy& policy,
        const string &raw_query,
        int document_id
    ) const;
    
    set<int>::iterator begin();
    set<int>::iterator end();
    set<int>::const_iterator cbegin();
    set<int>::const_iterator cend();
    
    //const map<string, double> GetWordFrequencies(int document_id) const;
    const map<string, double> &GetWordFrequencies(int document_id) const;
    void RemoveDocument(int document_id);
    //template<class ExecutionPolicy>
    //void RemoveDocument(ExecutionPolicy policy, int document_id);
    void RemoveDocument(
        const std::execution::parallel_policy& policy, 
        int document_id
    );
    void RemoveDocument(
        const std::execution::sequenced_policy& policy, 
        int document_id
    );

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        map<string,double> freqs;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    //vector<int> document_ids_;
    std::set<int> document_ids_;

    bool IsStopWord(const string_view& word) const;
    static bool IsValidWord(const string_view& word);

    vector<string> SplitIntoWordsNoStop(const string& text) const;
    static int ComputeAverageRating(const vector<int>& ratings);

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const string& text) const;
     
    // Sequential version
    struct QuerySeq {
        set<string> plus_words;
        set<string> minus_words;
    };
    
    // Parallel version
    struct Query {
        vector<string> plus_words;
        vector<string> minus_words;
    };
    
    // Sequential version
    QuerySeq ParseQuerySeq(const string& text) const;
    // Parallel version
    Query ParseQuery(const string& text,bool is_uniq=true) const;
    Query ParseQuery(
        const std::execution::parallel_policy& policy,
        const string& text, 
        bool is_uniq=true
    ) const;
    
    double ComputeWordInverseDocumentFreq(const string& word) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(
        const Query &query,
        DocumentPredicate document_predicate
    ) const;
};

void make_unique(
    vector<string>& items
);
void make_unique(
    const std::execution::parallel_policy& policy,
    vector<string>& items
);

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words)
:stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord))
    {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    const std::string &raw_query,
    DocumentPredicate document_predicate) const
{
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(
        matched_documents.begin(),
        matched_documents.end(),
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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    const Query &query,
    DocumentPredicate document_predicate) const
{
    std::map<int, double> document_to_relevance;
    for (const std::string &word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
        {
            const auto &document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating))
            {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string &word : query.minus_words)
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

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance)
    {
        matched_documents.push_back(
            {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}