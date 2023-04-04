#include "process_queries.h"
#include <list>
#include  <iterator>

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries
) {
    std::vector<std::vector<Document>> documents_lists(queries.size());
    std::transform(
        std::execution::par,
        queries.begin(), queries.end(),
        documents_lists.begin(),
        [&search_server](const std::string &s) {
            return search_server.FindTopDocuments(s);
        }
    );
    return documents_lists;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries
)
{
    std::vector<std::vector<Document>> response = ProcessQueries(search_server,queries);
    std::vector<Document> result;
    result = std::accumulate(
        response.begin(),
        response.end(),
        decltype(response)::value_type{},
        [](auto &acc, auto &items) {
            acc.insert(acc.end(), items.begin(), items.end());
            return acc;
        }
    );
    /*std::for_each(
        std::execution::par,
        response.begin(),
        response.end(),
        [&result](const std::vector<Document>& docs){
            std::copy(
                docs.begin(),
                docs.end(),
                std::back_inserter(result)
            );
            //result.insert(
            //    result.end(), 
            //    docs.begin(), 
            //    docs.end()
            //);
        }
    );*/
    return result;
   // std::list<Document> result;
    /*std::reduce(
        std::execution::par,
        response.begin(), response.end(),
        result.begin(),
        [&search_server](const std::vector<Document> &resp) {
            //return search_server.FindTopDocuments(s);
            a.insert(a.end(), resp.begin(), resp.end());
        }
    );*/
}
