



#include "search_server.h"
#include "process_queries.h"

#include <iostream>
#include <string>
#include <vector>
#include <execution>

using namespace std;

void PrintMatchDocumentResultUTest(int document_id,  std::vector<std::string> words,
                                   DocumentStatus status) {
    std::cout << "{ "
              << "document_id = " << document_id << ", "
              << "status = " << static_cast<int>(status) << ", "
              << "words =";
    for (const string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}" << std::endl;
}
 
void PrintDocumentUTest(const Document& document) {
    std::cout << "{ "
              << "document_id = " << document.id << ", "
              << "relevance = " << document.relevance << ", "
              << "rating = " << document.rating << " }" << std::endl;
}
 
void TestMatch() {
    const std::vector<int> ratings1 = {1, 2, 3, 4, 5};
    const std::vector<int> ratings2 = {-1, -2, 30, -3, 44, 5};
    const std::vector<int> ratings3 = {12, -20, 80, 0, 8, 0, 0, 9, 67};
    const std::vector<int> ratings4 = {7, 0, 3, -49, 5};
    const std::vector<int> ratings5 = {81, -6, 7, 94, -7};
    const std::vector<int> ratings6 = {41, 8, -7, 897, 5};
    const std::vector<int> ratings7 = {543, 0, 43, 4, -5};
    const std::vector<int> ratings8 = {91, 7, 3, -88, 56};
    const std::vector<int> ratings9 = {0, -87, 93, 66, 5};
    const std::vector<int> ratings10 = {11, 2, -43, 4, 895};
    std::string stop_words = "и в на";
    SearchServer search_server(stop_words);
 
    search_server.AddDocument(0, "белый кот и модный ошейник", DocumentStatus::ACTUAL, ratings1);
    search_server.AddDocument(1, "пушистый кот пушистый хвост", DocumentStatus::ACTUAL, ratings2);
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза", DocumentStatus::ACTUAL,
                              ratings3);
    search_server.AddDocument(3, "белый модный кот", DocumentStatus::IRRELEVANT, ratings1);
    search_server.AddDocument(4, "пушистый кот пёс", DocumentStatus::IRRELEVANT, ratings2);
    search_server.AddDocument(5, "ухоженный ошейник выразительные глаза",
                              DocumentStatus::IRRELEVANT, ratings3);
    search_server.AddDocument(6, "кот и ошейник", DocumentStatus::BANNED, ratings1);
    search_server.AddDocument(7, "пёс и хвост", DocumentStatus::BANNED, ratings2);
    search_server.AddDocument(8, "модный пёс пушистый хвост", DocumentStatus::BANNED, ratings3);
    search_server.AddDocument(9, "кот пушистый ошейник", DocumentStatus::REMOVED, ratings1);
    search_server.AddDocument(10, "ухоженный кот и пёс", DocumentStatus::REMOVED, ratings2);
    search_server.AddDocument(11, "хвост и выразительные глаза", DocumentStatus::REMOVED, ratings3);
 
    const std::string query = "пушистый ухоженный кот -ошейник";
    const auto documents = search_server.FindTopDocuments(query);
 
    std::cout << "Top documents for query:" << std::endl;
    for (const Document& document : documents) {
        PrintDocumentUTest(document);
    }
 
    std::cout << "Documents' statuses:" << std::endl;
    const int document_count = search_server.GetDocumentCount();
    
    for (int document_id = 0; document_id < document_count; ++document_id) {
        //tuple<vector<string>, DocumentStatus> p = search_server.MatchDocument(query, document_id);
        //tuple<vector<string>, DocumentStatus> tpl = search_server.MatchDocument(query, document_id);
        //PrintMatchDocumentResultUTest(document_id, get<0>(tpl), get<1>(tpl));
        const auto [words, status] = search_server.MatchDocument(query, document_id);
        vector<string> words2;
        for(string_view s: words) words2.push_back(string(s));
        PrintMatchDocumentResultUTest(document_id, words2, status);
    }
}

void Test2(){

    //SearchServer search_server("and with"s);
    string stop_words = "and with"s;
    SearchServer search_server(stop_words);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny curly -not -not"s;

    {
        //std::cout << "ex1:start"<<std::endl;
        const auto [words, status] = search_server.MatchDocument(query, 1);
        cout << words.size() << " words for document 1"s << endl;
        // 1 words for document 1
        //std::cout << "ex1:end"<<std::endl;
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 2);
        cout << words.size() << " words for document 2"s << endl;
        // 2 words for document 2
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
        cout << words.size() << " words for document 3"s << endl;
        // 0 words for document 3
    }
}

int main() {
    //std::cout << "main: start"<<std::endl;
    //Test2();
    TestMatch();
    return 0;
}

/*#include "search_server.h"
#include "process_queries.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny"s;

    auto report = [&search_server, &query] {
        cout << search_server.GetDocumentCount() << " documents total, "s
            << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
    };

    report();
    // однопоточная версия
    search_server.RemoveDocument(5);
    report();
    // однопоточная версия
    search_server.RemoveDocument(execution::seq, 1);
    report();
    // многопоточная версия
    search_server.RemoveDocument(execution::par, 2);
    report();

    return 0;
}*/

/*#include "process_queries.h"
#include "search_server.h"
#include <iostream>
#include <string>
#include <vector>
using namespace std;
int main() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }
    const vector<string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
        cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
    }
    return 0;
} */

/*#include "process_queries.h"
#include "search_server.h"
#include <iostream>
#include <string>
#include <vector>
using namespace std;
int main() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }
    const vector<string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    id = 0;
    for (
        const auto& documents : ProcessQueries(search_server, queries)
    ) {
        cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << endl;
    }
    return 0;
}*/
