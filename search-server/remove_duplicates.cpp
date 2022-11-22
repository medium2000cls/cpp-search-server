#include "remove_duplicates.h"

template <typename T>
static bool CompareMapsKeys (const map<string, T>& lhs, const map<string, T>& rhs) {
    if (lhs.size() != rhs.size()) { return false; }
    for(const auto& [key_lhs, _] : lhs ) {
        if (!rhs.count(key_lhs)) { return false; }
    }
    return true;
}

void RemoveDuplicates(SearchServer& search_server)
{
    set<int> documents_for_delete;
    
    for (auto it = search_server.begin(); it != search_server.end(); ++it) {
        for(auto it_2 = it + 1; it_2 != search_server.end(); ++it_2) {
            if (CompareMapsKeys(search_server.GetWordFrequencies(*it),search_server.GetWordFrequencies(*it_2))){
                documents_for_delete.insert(*it_2);
            }
        }
    }
    
    for (auto document_id : documents_for_delete) {
        cout << "Found duplicate document id " << document_id << endl;
        search_server.RemoveDocument(document_id);
    }
}
