#include "remove_duplicates.h"
#include <sstream>

using namespace std;

void RemoveDuplicates(SearchServer &search_server)
{
    set<set<std::string> > passed;
    set<int> duplicates;
    for (int doc_id : search_server)
    {
        set<string> bag_of_words;
        for (const auto &[word, _] : search_server.GetWordFrequencies(doc_id))
        {
            bag_of_words.insert(word);
        }
        if (passed.find(bag_of_words) == passed.end())
        {
            passed.insert(bag_of_words);
        }
        else
        {
            duplicates.insert(doc_id);
        }
    }
    for(int doc_id: duplicates){
        search_server.RemoveDocument(doc_id);
    }
}
