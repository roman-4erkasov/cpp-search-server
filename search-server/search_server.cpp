#include "search_server.h"
#include <cmath>
#include <numeric>
#include <deque>
#include <stdexcept>
#include <sstream>

using namespace std;
using namespace std::literals;


SearchServer::SearchServer(
    std::string const& stop_words_text
)
:SearchServer(std::string_view(stop_words_text))
{}

SearchServer::SearchServer(
    std::string_view stop_words_text
)
:SearchServer(SplitIntoWords(stop_words_text))
{
}

bool SearchServer::IsValidWord(std::string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const 
{
    vector<string_view> words;
    for (string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument(
                "Word "s + std::string(word) + " is invalid"s
            );
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}


std::vector<Document> SearchServer::FindTopDocuments(
    std::string_view raw_query
) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}


std::vector<Document> SearchServer::FindTopDocuments(
    std::string_view raw_query,
    DocumentStatus status
) const
{
    return FindTopDocuments(
        raw_query, [status](
            int document_id, 
            DocumentStatus document_status, 
            int rating
        )
        { return document_status == status; }
    );
}


MatchType SearchServer::MatchDocument(
    std::string_view raw_query,
    int document_id
) const
{
    const auto query = ParseQuery(raw_query);
    for (const string_view &word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            return {
                vector<string_view>{}, 
                documents_.at(document_id).status
            };
        }
    }
    vector<string_view> matched_words;
    for (string_view word : query.plus_words)
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
    
    return {matched_words, documents_.at(document_id).status};
}


MatchType SearchServer::MatchDocument(
    const std::execution::sequenced_policy& policy,
    std::string_view raw_query,
    int document_id
) const
{
    return MatchDocument(raw_query, document_id);
}


MatchType SearchServer::MatchDocument(
    const std::execution::parallel_policy& policy,
    std::string_view raw_query,
    int document_id
) const
{
    const auto query = ParseQuery(raw_query, false);
    if(
        any_of(
            policy, 
            query.minus_words.begin(), 
            query.minus_words.end(),
            [&](string_view word){
                return word_to_document_freqs_.at(word).count(document_id);
            }
        )
    )
    {
        return {vector<string_view>{}, documents_.at(document_id).status};
    }
    vector<string_view> matched_words(query.plus_words.size());
    {
        auto it_end = copy_if(
            policy,
            query.plus_words.begin(),
            query.plus_words.end(),
            matched_words.begin(),
            [&](string_view word){
                return word_to_document_freqs_.at(word).count(document_id);
            }
        );
        matched_words.erase(it_end, matched_words.end());
    }
    sort(policy, matched_words.begin(),matched_words.end());
    {
        auto it_end = unique(
            policy,
            matched_words.begin(),
            matched_words.end()
        );
        matched_words.erase(it_end, matched_words.end());
    }
    return {
        matched_words, 
        documents_.at(document_id).status
    };
}


void SearchServer::AddDocument(
    int document_id, 
    const std::string& document, 
    DocumentStatus status,
    const std::vector<int>& ratings
) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    auto it = words_.insert(document);
    map<string_view, double> freqs;
    const auto words = SplitIntoWordsNoStop(*(it.first));
    const double inv_word_count = 1.0 / words.size();
    for (string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        freqs[word] += 1;
    }
    documents_.emplace(
        document_id, 
        DocumentData{ComputeAverageRating(ratings), status, freqs}
    );
    document_ids_.insert(document_id);
}


int SearchServer::GetDocumentCount() const {
    return documents_.size();
}


int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(
        ratings.begin(), ratings.end(), 0
    );
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string(text) + " is invalid");
    }
    return {word, is_minus, IsStopWord(word)};
}

template< typename T >
typename std::vector<T>::iterator InsertSorted(
    std::vector<T> & vec,
    T const& item
)
{
    
    auto it = std::upper_bound(vec.begin(), vec.end(), item);
    if(
        it == vec.begin()
        ||
        item != *prev(it)
    )
    {
        return vec.insert(it, item);
    }
    else {
        return {};
    }
}


void make_unique(vector<string_view>& items) {
    sort(items.begin(),items.end());
    auto it_end = unique(items.begin(),items.end());
    items.erase(it_end, items.end());
}

void make_unique(
    const std::execution::parallel_policy& policy,
    vector<string_view>& items
) {
    auto it_end = unique(policy, items.begin(),items.end());
    items.erase(it_end, items.end());
}

SearchServer::Query SearchServer::ParseQuery(
    std::string_view text, 
    bool is_uniq//=true
) const {
    Query result;
    for (string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if(is_uniq) {
        make_unique(result.plus_words);
        make_unique(result.minus_words);
    }
    return result;
}


double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

set<int>::iterator SearchServer::begin() {
    return document_ids_.begin();
}
set<int>::iterator SearchServer::end() {
    return document_ids_.end();
}

set<int>::const_iterator SearchServer::cbegin() {
    return document_ids_.cbegin();
}

set<int>::const_iterator SearchServer::cend() {
    return document_ids_.cend();
}


const map<string_view, double> &SearchServer::GetWordFrequencies(int document_id) const
{
    if (documents_.count(document_id))
    {
        return documents_.at(document_id).freqs;
    }
    else
    {
        static const map<string_view, double> empty_;
        return empty_;
    }
}

void SearchServer::RemoveDocument(int document_id) {
    for ( const auto &[word, _]:  documents_.at(document_id).freqs)
        word_to_document_freqs_[word].erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

    

void SearchServer::RemoveDocument(
    const std::execution::sequenced_policy& policy, 
    int document_id
) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(
    const std::execution::parallel_policy& policy, 
    int document_id
) {
    std::vector<const std::string_view*> words(documents_.at(document_id).freqs.size());
    std::transform(
        policy,
        documents_.at(document_id).freqs.begin(),
        documents_.at(document_id).freqs.end(),
        words.begin(),
        [](const auto &pair){return &pair.first;}
    );
    std::for_each(
        policy,
        words.begin(),
        words.end(),
        [&](const auto *word){
            word_to_document_freqs_.at(*word).erase(document_id);
        }
    );
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

 
