#pragma once
#include "search_server.h"
#include <vector>
#include <deque>

using namespace std;

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        // напишите реализацию
        FreeUpPlaceInQueue();
        vector<Document> documents = search_server_.FindTopDocuments(raw_query, document_predicate);
        if(documents.empty()) n_empty_requests_++;
        requests_.push_back({documents, raw_query});
        return documents;
    }
    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);
    vector<Document> AddFindRequest(const string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        // определите, что должно быть в структуре
        vector<Document> documents;
        string raw_query;
    };
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    // возможно, здесь вам понадобится что-то ещё
    int n_empty_requests_ = 0;
    const SearchServer& search_server_;
    
    void FreeUpPlaceInQueue();
};
