#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    set<int> buff;
    for (auto it1=search_server.begin();it1!=search_server.end();++it1) {
        int id1 = *it1;
        if(buff.count(id1)) continue;
        map<string,double> freqs1=search_server.GetWordFrequencies(id1);
        for(auto it2=next(it1);it2!=search_server.end();++it2){
            int id2 = *it2;
            map<string,double> freqs2=search_server.GetWordFrequencies(id2);
            if(freqs1.size()!=freqs2.size()) continue;
            bool is_equal = equal(
                freqs1.begin(),
                freqs1.end(),
                freqs2.begin(),
                [](const auto l,const auto r){
                    return l.first==r.first;
                }
            );
            if(is_equal) buff.insert(id2);
        }
    }
    for(int id: buff) search_server.RemoveDocument(id);
}
