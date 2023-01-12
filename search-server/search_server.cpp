#include "search_server.h"
#include <cmath>
#include <numeric>
#include <deque>

using namespace std;

std::vector<Document> SearchServer::FindTopDocuments(
    const std::string &raw_query,
    DocumentStatus status) const
{
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating)
        { return document_status == status; });
};

std::vector<Document> SearchServer::FindTopDocuments(const string &raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
};

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(
    const string &raw_query,
    int document_id) const
{
    const auto query = ParseQuery(raw_query);

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
};

SearchServer::SearchServer(const std::string &stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(
    int document_id,
    const std::string &document,
    DocumentStatus status,
    const std::vector<int> &ratings)
{
    if ((document_id < 0) || (documents_.count(document_id) > 0))
    {
        throw invalid_argument("Invalid document_id"s);
    }
    map<string, double> freqs;
    const auto words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const string &word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        freqs[word] = 1;
    }
    documents_.emplace(
        document_id,
        DocumentData{ComputeAverageRating(ratings), status, freqs});
    document_ids_[next_index] = document_id;
    id2index[document_id] = next_index;
    ++next_index;
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

int SearchServer::GetDocumentId(int index) const
{
    return document_ids_.at(index);
}

bool SearchServer::IsStopWord(const string &word) const
{
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string &word)
{
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c)
                   { return c >= '\0' && c < ' '; });
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string &text) const
{
    vector<string> words;
    for (const string &word : SplitIntoWords(text))
    {
        if (!IsValidWord(word))
        {
            throw invalid_argument("Word "s + word + " is invalid"s);
        }
        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int> &ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    int rating_sum = std::accumulate(
        ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string &text) const
{
    if (text.empty())
    {
        throw invalid_argument("Query word is empty"s);
    }
    string word = text;
    bool is_minus = false;
    if (word[0] == '-')
    {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word))
    {
        throw invalid_argument("Query word "s + text + " is invalid");
    }
    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const string &text) const
{
    Query result;
    for (const string &word : SplitIntoWords(text))
    {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                result.minus_words.insert(query_word.data);
            }
            else
            {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string &word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

map<int, int>::iterator SearchServer::begin()
{
    return document_ids_.begin();
}
map<int, int>::iterator SearchServer::end()
{
    return document_ids_.end();
}

map<int, int>::const_iterator SearchServer::cbegin()
{
    return document_ids_.cbegin();
}

map<int, int>::const_iterator SearchServer::cend()
{
    return document_ids_.cend();
}

const map<string, double> &SearchServer::GetWordFrequencies(int document_id) const
{
    if (documents_.count(document_id))
    {
        return documents_.at(document_id).freqs;
    }
    else
    {
        static const map<string, double> empty_;
        return empty_;
    }
}

void SearchServer::RemoveDocument(int document_id)
{
    for (const auto &[word, _] : documents_.at(document_id).freqs)
    {
        word_to_document_freqs_[word].erase(document_id);
    }
    documents_.erase(document_id);
    int doc_index = id2index.at(document_id);
    document_ids_.erase(doc_index);
    id2index.erase(document_id);
}
