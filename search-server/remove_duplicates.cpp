#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server)
{
    set<int> documents_for_delete;
    
    //Изменил метод, теперь он ищет по словарю какое количество документов было добавлено с одинаковым запросом.
    //Если количество превышает 1, то остальные документы добавляются в коллекцию для удаления.
    //Применил упрощенный for
    for (auto & item : search_server) {
        auto& documents = item.second;
        if (documents.size() > 1){
            for (auto it_doc = ++documents.begin(); it_doc != documents.end(); ++it_doc) {
                documents_for_delete.insert((*it_doc));
            }
        }
    }
    
    for (auto document_id : documents_for_delete) {
        cout << "Found duplicate document id " << document_id << endl;
        search_server.RemoveDocument(document_id);
    }
}
