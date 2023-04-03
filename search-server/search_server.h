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


using namespace std;



class SearchServer {
public:
    static bool duration_logging;
    static bool verbose;
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
        //const std::string& raw_query,
        std::string_view raw_query,
        DocumentPredicate document_predicate
    ) const;
    std::vector<Document> FindTopDocuments(
        //const std::string& raw_query,
        std::string_view raw_query,
        DocumentStatus status
    ) const;
    std::vector<Document> FindTopDocuments(
        //const std::string& raw_query
        std::string_view raw_query
    ) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
        const std::execution::sequenced_policy& policy,
        std::string_view raw_query,
        //const std::string& raw_query,
        DocumentPredicate document_predicate
    ) const;
    std::vector<Document> FindTopDocuments(
        const std::execution::sequenced_policy& policy,
        //const std::string& raw_query,
        std::string_view raw_query,
        DocumentStatus status
    ) const;
    std::vector<Document> FindTopDocuments(
        const std::execution::sequenced_policy& policy,
        //const std::string& raw_query
        std::string_view raw_query
    ) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
        const std::execution::parallel_policy& policy,
        //const std::string& raw_query,
        std::string_view raw_query,
        DocumentPredicate document_predicate
    ) const;
    std::vector<Document> FindTopDocuments(
        const std::execution::parallel_policy& policy,
        //const std::string& raw_query, 
        std::string_view raw_query,
        DocumentStatus status
    ) const;
    std::vector<Document> FindTopDocuments(
        const std::execution::parallel_policy& policy,
        //const std::string& raw_query
        std::string_view raw_query
    ) const;
    
    int GetDocumentCount() const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        std::string_view raw_query, 
        int document_id
    ) const;
    tuple<vector<string_view>, DocumentStatus> MatchDocument(
        const std::execution::sequenced_policy& policy,
        string_view raw_query,
        int document_id
    ) const;
    tuple<vector<string_view>, DocumentStatus> MatchDocument(
        const std::execution::parallel_policy& policy,
        string_view raw_query,
        int document_id
    ) const;
    
    set<int>::iterator begin();
    set<int>::iterator end();
    set<int>::const_iterator cbegin();
    set<int>::const_iterator cend();
    
    //const map<string, double> GetWordFrequencies(int document_id) const;
    const map<string_view, double> &GetWordFrequencies(int document_id) const;
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
        map<string_view,double> freqs;
    };
    std::set<std::string_view> stop_words_;
    
    map<string_view, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(string_view word) const;
    static bool IsValidWord(string_view word);

    vector<string_view> SplitIntoWordsNoStop(string_view text) const;
    static int ComputeAverageRating(const vector<int>& ratings);

    struct QueryWord {
        string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string_view text) const;
     
    // Sequential version
    struct QuerySeq {
        set<string_view> plus_words;
        set<string_view> minus_words;
    };
    
    // Parallel version
    struct Query {
        vector<string_view> plus_words;
        vector<string_view> minus_words;
    };
    
    // Sequential version
    QuerySeq ParseQuerySeq(string_view text) const;
    // Parallel version
    Query ParseQuery(string_view text, bool is_uniq=true) const;
    Query ParseQuery(
        const std::execution::parallel_policy& policy,
        string_view text, 
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
    vector<string_view>& items
);
void make_unique(
    const std::execution::parallel_policy& policy,
    vector<string_view>& items
);

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words)
{
    for(std::string word: MakeUniqueNonEmptyStrings(stop_words)){
        auto it = words_.insert(word);
	stop_words_.insert(string_view(*(it.first)));
    } 
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord))
    {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    const std::execution::sequenced_policy& policy,
    //const std::string& raw_query,
    std::string_view raw_query,
    DocumentPredicate document_predicate
) const
{
    return FindTopDocuments(raw_query,document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    //const std::string& raw_query,
    std::string_view raw_query,
    DocumentPredicate document_predicate
) const
{
    //auto it = words_.insert(raw_query);
    const Query query = ParseQuery(raw_query);
    if(verbose) std::cout << "query=Query(plus=[";
    if(verbose) for(string_view s: query.plus_words) std::cout<<"\""<<s<<"\" ";
    if(verbose) std::cout << "], minus=[";
    if(verbose) for(string_view s: query.minus_words) std::cout<<"\""<<s<<"\" ";
    if(verbose) std::cout << "]"<<std::endl;
    auto matched_documents = FindAllDocuments(query, document_predicate);
    if(verbose) std::cout << "[FindTopDocuments] after FindAllDocuments: matched_documents=";
    if(verbose) for(const auto& item: matched_documents) std::cout << " " << item;
    if(verbose) std::cout << std::endl;
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
std::vector<Document> SearchServer::FindTopDocuments(
    const std::execution::parallel_policy& policy,
    std::string_view raw_query,
    //const std::string& raw_query,
    DocumentPredicate document_predicate
) const
{
    if(duration_logging) LOG_DURATION("[FindTopDocuments]"s);
    //auto it = words_.insert(raw_query);
    Query query;
    {
        if(duration_logging) LOG_DURATION("[FindTopDocuments][ParseQuery]"s);
        query = ParseQuery(raw_query);
    }
    //auto matched_documents = FindAllDocuments(query, document_predicate);
    std::vector<Document> matched_documents;
    {
        if(duration_logging) LOG_DURATION("[FindTopDocuments][FindAllDocuments]"s);
        matched_documents = FindAllDocuments(policy, query, document_predicate);
	if(verbose) std::cout << "[FindTopDocuments] after FindAllDocuments: matched_documents=";
	if(verbose) for(const auto& item: matched_documents) std::cout << " " << item;
	if(verbose) std::cout << std::endl;
    }
    {
        if(duration_logging) LOG_DURATION("[FindTopDocuments][sort matched_documents]"s);
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
            }
        );
    }
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
            if(verbose) std::cout <<"[" <<word<< "][" << document_id << "]"
                <<" rel="<< document_to_relevance[document_id] 
                <<" tf="<<term_freq 
                << " idf="<< inverse_document_freq
                << std::endl;
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
        if(verbose) std::cout << "[FindAllDocuments]["
            << document_id <<"] rel="<< relevance
            << std::endl;
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
    if(duration_logging) LOG_DURATION("[FindAllDocuments]"s);
    //std::map<int, double> document_to_relevance;
    ConcurrentMap<int, double> document_to_relevance;
    std::vector<std::future<void> > tasks;
    {
        if(duration_logging) LOG_DURATION("[FindAllDocuments][proc plus_words]"s);
        for (std::string_view word: query.plus_words){
            if(verbose) std::cout << "[FindAllDocuments] start iter w=\""<<word<<"\"\n";
            tasks.push_back( 
                std::async(
                    [&](std::string word){
                        std::stringstream ss;
                        if(verbose) ss << "[FindAllDocuments][async] start async w=\""<<word<<"\"\n";
                        if(verbose) ss << "[FindAllDocuments][async]word_to_document_freqs_.count(word)="
                            << word_to_document_freqs_.count(word)
                            << "\n";
                        if (word_to_document_freqs_.count(word) == 0)
                        {
                            return;
                        }
                        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                        if(verbose)ss << "[FindAllDocuments][async]idf=" << inverse_document_freq << "\n";
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
                            if(verbose) ss << "[FindAllDocuments][async] doc2rlv=" 
                               << document_to_relevance[document_id].ref_to_value
                               << "\n";
                        }
                        std::cout << ss.str();
                    },
                    string(word)
                )
            );
        }
        //std::for_each(
        //    policy,
        //    query.plus_words.begin(),
        //    query.plus_words.end(),
        //    [&](std::string_view word){
        //        if (word_to_document_freqs_.count(word) == 0)
        //        {
        //            return;
        //        }
        //        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        //        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
        //        {
        //            const auto& document_data = documents_.at(document_id);
        //            if (
        //                document_predicate(
        //                    document_id, document_data.status, document_data.rating
        //                )
        //            )
        //            {
        //                document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
        //            }
        //        }
        //    }
        //);
    }
    
    //for (std::string_view word: query.plus_words)
    //{
    //    if (word_to_document_freqs_.count(word) == 0)
    //    {
    //        continue;
    //    }
    //    const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
    //    for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
    //    {
    //        const auto& document_data = documents_.at(document_id);
    //        if (document_predicate(document_id, document_data.status, document_data.rating))
    //        {
    //            document_to_relevance[document_id] += term_freq * inverse_document_freq;
    //        }
    //    }
    //}
    for(auto& task: tasks) {
        task.wait();
    }
    {
        if(duration_logging) LOG_DURATION("[FindAllDocuments][proc minus_words]"s);
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
    }
    std::vector<Document> matched_documents;
    {
        if(duration_logging) LOG_DURATION("[FindAllDocuments][append matched_documents]"s);
        for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap())
        //for (const auto& [document_id, relevance] : document_to_relevance)
        {
            //if(0<relevance) {
                matched_documents.push_back(
                    {document_id, relevance, documents_.at(document_id).rating}
                );
            //}
        }
    }
    return matched_documents;
}

