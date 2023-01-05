#include <iostream>
#include <deque>

#include "request_queue.hpp"
#include "search_server.hpp"


using namespace std;

void FindTopDocuments(SearchServer search_server, const string &raw_query) {
    LOG_DURATION_STREAM("Operation time", cout);
    std::vector<Document> res;
    cout<<"Результаты поиска по запросу: "<<raw_query<<endl;
    res = search_server.FindTopDocuments(raw_query);
    for(Document doc: res) cout << doc << endl;

}

void MatchDocuments(SearchServer search_server, string raw_query) {
    LOG_DURATION_STREAM("Operation time", cout);
    cout<<"Результаты поиска по запросу: "<<raw_query<<endl;
    int n = search_server.GetDocumentCount();
    for(int i=0;i<n;++i){
        bool is_first = true;
        int id = search_server.GetDocumentId(i);
        auto [words, status] = search_server.MatchDocument(raw_query, id);
        cout << "{ document_id = " << id << ", status = "
          << static_cast<std::underlying_type<DocumentStatus>::type>(status) 
          << ", words = ";
        for(const string &word: words){
            if(is_first) is_first=false;
            else cout << ' ';
            cout << word;
        }
        cout<<" }"<<endl;
    }    
}

void profile_test(){
    SearchServer search_server("and in at"s);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

    FindTopDocuments(search_server, "dog sparrow"s);
    MatchDocuments(search_server, "dog sparrow"s);
}

void request_queue_test(){
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
}

int main() {
    profile_test();
    return 0;
} 
