#include "remove_duplicates.h"
#include <sstream>

using namespace std;

void RemoveDuplicates(SearchServer &search_server)
{
    set<std::string> passed;
    for (auto it : search_server)
    {
        int doc_id = it.second;
        std::stringstream sstream;
        bool is_first = true;
        for (const auto &[word, _] : search_server.GetWordFrequencies(doc_id))
        {
            if (is_first)
                is_first = false;
            else
                sstream << ' ';
            sstream << word;
        }
        std::string key = sstream.str();
        if (not passed.count(key))
        {
            passed.insert(key);
        }
        else
        {
            search_server.RemoveDocument(doc_id);
        }
    }
}
