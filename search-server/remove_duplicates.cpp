#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server)
{
    using namespace std;
    set<int> documents_for_delete;
    set<set<string>> words_catalog;
    for (auto& document_id : search_server) {
        const auto& word_fregs = search_server.GetWordFrequencies(document_id);
        set<string> words;
        for (const auto& [word, _] : word_fregs) {
            words.insert(word);
        }
        if (words_catalog.count(words)) {
            documents_for_delete.insert(document_id);
        }
        else { words_catalog.insert(words); }
    }
    
    for (auto document_id : documents_for_delete) {
        cout << "Found duplicate document id " << document_id << endl;
        search_server.RemoveDocument(document_id);
    }
}
