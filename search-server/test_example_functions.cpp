#include "test_example_functions.h"

#include <execution>

#define assertm(exp, msg) assert(((void)msg, exp))

using namespace std;


void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(
            //std::execution::par,
            "in"s
        );
        //ASSERT_EQUAL(found_docs.size(), 1u);
        assert(1u==found_docs.size());
        const Document& doc0 = found_docs[0];
        //ASSERT_EQUAL(doc0.id, doc_id);
        assert(doc0.id==doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        //ASSERT_HINT(
        //    server.FindTopDocuments("in"s).empty(),
        //    "Stop words must be excluded from documents"s
        //);
        assertm(
            server.FindTopDocuments(
                std::execution::par,
                "in"s
            ).empty(),
            "Stop words must be excluded from documents"s
        );
    }
}

void TestExcludeMinusWordsFromAddedDocument(){
    const int doc_id = 1337;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(
            std::execution::par,
            "in"s
        );
        //ASSERT_EQUAL(found_docs.size(), 1);
        assert(found_docs.size()==1);
        const Document& doc0 = found_docs[0];
        //ASSERT_EQUAL(doc0.id, doc_id);
        assert(doc0.id==doc_id);
    }
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(
            std::execution::par,
            "in -cat"s
        );
        //ASSERT_EQUAL(found_docs.size(), 0);
        assert(found_docs.size()==0);
        //ASSERT(server.FindTopDocuments("in -cat"s).empty());
        assert(server.FindTopDocuments("in -cat"s).empty());
    }
}

int ComputeAverageRating(const vector<int> &ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    //int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    int rating_sum = 0;
    for(int rating: ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}


void TestRatingComputation(){
    const int doc_id = 1337;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(
            std::execution::par,
            "in"s
        );
        //ASSERT_EQUAL(found_docs.size(), 1);
        assert(found_docs.size()==1);
        const Document& doc0 = found_docs[0];
        //ASSERT_EQUAL(doc0.rating, ComputeAverageRating(ratings));
        assert(doc0.rating==ComputeAverageRating(ratings));
    }
}

void TestRelevanceOrder(){
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server(""s);
        server.AddDocument(
            1, 
            "white cat in the city", 
            DocumentStatus::ACTUAL, 
            ratings
        );
        server.AddDocument(
            2, 
            "black cat in the village", 
            DocumentStatus::ACTUAL, 
            ratings
        );
        server.AddDocument(
            3, 
            "red cat in the house", 
            DocumentStatus::ACTUAL, 
            ratings
        );
        const vector<Document> found = server.FindTopDocuments(
            std::execution::par,
            "black cat village"s
        );
        string msg = "found.size() = "+to_string(found.size());
        //ASSERT_EQUAL_HINT(found.size(), 3, msg.c_str());
        //ASSERT_EQUAL(found.size(), 3);
        assert(found.size()==3);
        //ASSERT(
        //    is_sorted(
        //        found.begin(),
        //        found.end(),
        //        [](const Document& left, const Document& right) {
        //            return right.relevance < left.relevance;
        //        }
        //    )
        //);
        assert(
            is_sorted(
                found.begin(),
                found.end(),
                [](const Document& left, const Document& right) {
                    return right.relevance < left.relevance;
                }
            )
        );
    }
}


void TestPredicatFiltering() {
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server(""s);
        server.AddDocument(
            1, 
            "white cat in the city", 
            DocumentStatus::ACTUAL, 
            {3, 3, 3}
        );
        server.AddDocument(
            2, 
            "black cat in the village", 
            DocumentStatus::ACTUAL, 
            {1, 1, 1}
        );
        server.AddDocument(
            3, 
            "red cat in the village", 
            DocumentStatus::ACTUAL, 
            {4, 5, 6}
        );
        const vector<Document> found = server.FindTopDocuments(
            //std::execution::par,
            "black cat village"s,
            [](
                [[maybe_unused]] int doc_id,
                [[maybe_unused]] DocumentStatus doc_status,
                int rating
            )
            {
                return 2 < rating;
            }
        );
        //ASSERT_EQUAL(found.size(), 2);
        //ASSERT_EQUAL(found[0].id, 3);
        //ASSERT_EQUAL(found[1].id, 1);
        //assert(found.size()==2);
        assert(found[0].id==3);
        assert(found[1].id==1);
    }
}


void TestStatusFiltering() {
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server(""s);
        server.AddDocument(
            1, 
            "white cat in the city", 
            DocumentStatus::BANNED, 
            {3, 3, 3}
        );
        server.AddDocument(
            2, 
            "black cat in the village", 
            DocumentStatus::ACTUAL, 
            {1, 1, 1}
        );
        server.AddDocument(
            3, 
            "red cat in the village", 
            DocumentStatus::BANNED, 
            {4, 5, 6}
        );
        const vector<Document> found = server.FindTopDocuments(
            std::execution::par,
            "black cat village"s,
            DocumentStatus::BANNED
        );
        //ASSERT_EQUAL(found.size(), 2);
        //ASSERT_EQUAL(found[0].id, 3);
        //ASSERT_EQUAL(found[1].id, 1);
        //assert(found.size()==2);
        assert(found[0].id==3);
        assert(found[1].id==1);
    }
}


vector<string> SplitText(const string &text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

double ComputeWordInverseDocumentFreq(
    const string &word, 
    int doc_count,
    map<string, map<int, double>> word_to_document_freqs_
) {
    return log(doc_count * 1.0 / word_to_document_freqs_.at(word).size());
}


void TestRelevanceComputation() {
    string query = "black cat village"s;
    vector<pair<int, string> > documents = {
        {1, "white cat in the city"s},
        {2, "black cat in the village"s},
        {3, "red cat in the village"s}
    };
    map<string, map<int, double>> word_to_document_freqs_;
    for(const auto& [document_id, doc]: documents) {
        const vector<string> words = SplitText(doc);
        const double inv_word_count = 1.0 / words.size();
        for (const string &word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
    }
    
    map<int, double> document_to_relevance;
    for (const string &word : SplitText(query)) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(
            word, 3, word_to_document_freqs_
        );
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            document_to_relevance[document_id] += term_freq * inverse_document_freq;
        }
    }
    
    SearchServer server(""s);
    for(const auto& [doc_id, text]:documents) {
        server.AddDocument(
            doc_id, 
            text, 
            DocumentStatus::ACTUAL, 
            {1, 1, 1}
        );
    }
    const vector<Document> found = server.FindTopDocuments(
        std::execution::par,
        query, DocumentStatus::ACTUAL
    );
    for(const Document& doc: found) {
        //ASSERT_EQUAL(doc.relevance, document_to_relevance.at(doc.id));
        assert(doc.relevance==document_to_relevance.at(doc.id));
    }
}


void TestAll()
{
    TestExcludeStopWordsFromAddedDocumentContent();
    TestExcludeMinusWordsFromAddedDocument();
    TestRatingComputation();
    TestRelevanceOrder();
    TestPredicatFiltering();
    TestStatusFiltering();
    TestRelevanceComputation();
}

