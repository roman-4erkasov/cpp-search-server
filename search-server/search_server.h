#pragma once
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <execution>
#include <string_view>
#include <future>
#include <sstream>

#include "document.h"
#include "string_processing.h"
#include "paginator.h"
#include "concurrent_map.h"
#include "log_duration.h"

#define EPS 1e-6
const int MAX_RESULT_DOCUMENT_COUNT = 5;


class SearchServer {
public:
    explicit SearchServer(std::string const& str);
    explicit SearchServer(std::string_view str);
    template <typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);

    void AddDocument(
        int document_id, 
        const std::string& document, 
        DocumentStatus status,
        const std::vector<int>& ratings
    );
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
        std::string_view raw_query,
        DocumentPredicate document_predicate
    ) const;
    std::vector<Document> FindTopDocuments(
        std::string_view raw_query,
        DocumentStatus status
    ) const;
    std::vector<Document> FindTopDocuments(
        std::string_view raw_query
    ) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
        const std::execution::sequenced_policy& policy,
        std::string_view raw_query,
        DocumentPredicate document_predicate
    ) const;
    std::vector<Document> FindTopDocuments(
        const std::execution::sequenced_policy& policy,
        std::string_view raw_query,
        DocumentStatus status
    ) const;
    std::vector<Document> FindTopDocuments(
        const std::execution::sequenced_policy& policy,
        std::string_view raw_query
    ) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
        const std::execution::parallel_policy& policy,
        std::string_view raw_query,
        DocumentPredicate document_predicate
    ) const;
    std::vector<Document> FindTopDocuments(
        const std::execution::parallel_policy& policy,
        std::string_view raw_query,
        DocumentStatus status
    ) const;
    std::vector<Document> FindTopDocuments(
        const std::execution::parallel_policy& policy,
        std::string_view raw_query
    ) const;
    
    int GetDocumentCount() const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> 
    MatchDocument(
        std::string_view raw_query, 
        int document_id
    ) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        const std::execution::sequenced_policy& policy,
        std::string_view raw_query,
        int document_id
    ) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        const std::execution::parallel_policy& policy,
        std::string_view raw_query,
        int document_id
    ) const;
    
    std::set<int>::iterator begin();
    std::set<int>::iterator end();
    std::set<int>::const_iterator cbegin();
    std::set<int>::const_iterator cend();
    
    const std::map<std::string_view, double> 
    &GetWordFrequencies(int document_id) const;
    
    void RemoveDocument(int document_id);
    void RemoveDocument(
        const std::execution::parallel_policy& policy, 
        int document_id
    );
    void RemoveDocument(
        const std::execution::sequenced_policy& policy, 
        int document_id
    );

private:
    std::set<std::string> words_;
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::map<std::string_view,double> freqs;
    };
    std::set<std::string_view> stop_words_;
    
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(std::string_view word) const;
    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;
     
    // Sequential version
    struct QuerySeq {
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };
    
    // Parallel version
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    
    // Sequential version
    QuerySeq ParseQuerySeq(std::string_view text) const;
    // Parallel version
    Query ParseQuery(std::string_view text, bool is_uniq=true) const;
    Query ParseQuery(
        const std::execution::parallel_policy& policy,
        std::string_view text, 
        bool is_uniq=true
    ) const;
    
    double ComputeWordInverseDocumentFreq(std::string_view word) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(
        const Query &query,
        DocumentPredicate document_predicate
    ) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(
        const std::execution::parallel_policy& policy,
        const Query &query,
        DocumentPredicate document_predicate
    ) const;
};


void make_unique(
    std::vector<std::string_view>& items
);
void make_unique(
    const std::execution::parallel_policy& policy,
    std::vector<std::string_view>& items
);

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words)
{
    for(std::string word: MakeUniqueNonEmptyStrings(stop_words)){
        auto it = words_.insert(word);
	stop_words_.insert(std::string_view(*(it.first)));
    } 
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord))
    {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    const std::execution::sequenced_policy& policy,
    std::string_view raw_query,
    DocumentPredicate document_predicate
) const
{
    return FindTopDocuments(raw_query,document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    std::string_view raw_query,
    DocumentPredicate document_predicate
) const
{
    const Query query = ParseQuery(raw_query);
    std::vector<Document> matched_documents = FindAllDocuments(
        query, document_predicate
    );
    std::sort(
        matched_documents.begin(),
        matched_documents.end(),
        [](const Document &lhs, const Document &rhs)
        {
            if (std::abs(lhs.relevance - rhs.relevance) < EPS)
            {
                return lhs.rating > rhs.rating;
            }
            else
            {
                return lhs.relevance > rhs.relevance;
            }
        }
    );
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    const std::execution::parallel_policy& policy,
    std::string_view raw_query,
    DocumentPredicate document_predicate
) const
{
    const Query query = ParseQuery(raw_query);
    std::vector<Document> matched_documents = FindAllDocuments(
        policy, query, document_predicate
    );
    std::sort(
        matched_documents.begin(),
        matched_documents.end(),
        [](const Document &lhs, const Document &rhs)
        {
            if (std::abs(lhs.relevance - rhs.relevance) < EPS)
            {
                return lhs.rating > rhs.rating;
            }
            else
            {
                return lhs.relevance > rhs.relevance;
            }
        }
    );
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
    
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    const Query &query,
    DocumentPredicate document_predicate
) const
{
    std::map<int, double> document_to_relevance;
    for (std::string_view word: query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
        {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating))
            {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (std::string_view word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word))
        {
            document_to_relevance.erase(document_id);
        }
    }
    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance)
    {
        //if(0<relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating}
            );
        //}
    }
    return matched_documents;
}



template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    const std::execution::parallel_policy& policy,
    const Query &query,
    DocumentPredicate document_predicate
) const
{
    ConcurrentMap<int, double> document_to_relevance;
    std::for_each(
        policy,
        query.plus_words.begin(),
        query.plus_words.end(),
        [&](std::string_view word){
            if (word_to_document_freqs_.count(word) == 0)
            {
                return;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                const auto& document_data = documents_.at(document_id);
                if (
                    document_predicate(
                        document_id, document_data.status, document_data.rating
                    )
                )
                {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        }
    );
    for (std::string_view word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word))
        {
            document_to_relevance.erase(document_id);
        }
    }
    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap())
    {
        //if(0<relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating}
            );
        //}
    }
    return matched_documents;
}
