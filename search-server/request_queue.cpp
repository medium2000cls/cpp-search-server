#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {}

void RequestQueue::Add(const std::vector<Document>& documents) {
    bool is_null_result = false;
    if (documents.empty()) { is_null_result = true;}
    requests_.push_back({is_null_result, documents});
    if (requests_.size() > min_in_day_) {
        requests_.pop_front();
    }
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> documents = search_server_.FindTopDocuments(raw_query, status);
    Add(documents);
    return documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> documents = search_server_.FindTopDocuments(raw_query);
    Add(documents);
    return documents;
}

int RequestQueue::GetNoResultRequests() const {
    return count_if(requests_.begin(), requests_.end(), [](auto& query) {return query.is_null_result;});
}

