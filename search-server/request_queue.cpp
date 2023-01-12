#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer &search_server)
    : search_server_(search_server) {}

vector<Document> RequestQueue::AddFindRequest(const string &raw_query, DocumentStatus status)
{
    FreeUpPlaceInQueue();
    vector<Document> documents = search_server_.FindTopDocuments(raw_query, status);
    if (documents.empty())
    {
        n_empty_requests_++;
    }
    requests_.push_back({documents, raw_query});
    return documents;
}

vector<Document> RequestQueue::AddFindRequest(const string &raw_query)
{
    FreeUpPlaceInQueue();
    vector<Document> documents = search_server_.FindTopDocuments(raw_query);
    if (documents.empty())
    {
        n_empty_requests_++;
    }
    requests_.push_back({documents, raw_query});
    return documents;
}

int RequestQueue::GetNoResultRequests() const
{
    return n_empty_requests_;
}

void RequestQueue::FreeUpPlaceInQueue()
{
    while (min_in_day_ <= requests_.size())
    {
        if (requests_.front().documents.empty())
        {
            --n_empty_requests_;
        }
        requests_.pop_front();
    }
}
