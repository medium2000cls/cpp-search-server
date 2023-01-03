#include <numeric>
#include "search_server.h"

SearchServer::SearchServer() {}

SearchServer::SearchServer(const std::string& stop_words_text) : SearchServer(
        SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Stop-word contains invalid characters.");
    }
}

void SearchServer::AddDocument(int document_id,
                               const std::string& document,
                               DocumentStatus status,
                               const std::vector<int>& ratings)
{
    if (document_id < 0) { throw std::invalid_argument("Document ID cannot be less than zero"); }
    if (documents_.count(document_id) != 0) { throw std::invalid_argument("Document ID cannot be repeated"); }
    
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        if (!IsValidWord(word)) { throw std::invalid_argument("Word in document contains invalid characters."); }
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    order_documents_id_.insert(document_id);
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const
{
    return FindTopDocuments(raw_query,
            [status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]] int rating) {
                return document_status == status;
            });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
                                                                                 int document_id) const
{
    //LOG_DURATION;
    auto query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return std::tie(matched_words, documents_.at(document_id).status);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    if(!document_to_word_freqs_.count(document_id)) {
        const static std::map<std::string, double> word_frequencies_empty;
        return word_frequencies_empty;
    }
    return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id)
{
    SearchServer::RemoveDocument(std::execution::seq, document_id);
}

bool SearchServer::IsValidWord(const std::string& word)
{
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

bool SearchServer::IsStopWord(const std::string& word) const
{
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const
{
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    //Изменил цикл на accumulate
    rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const
{
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const
{
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        if (word.empty()) {
            continue;
        }
        const QueryWord query_word = ParseQueryWord(word);
        
        if (query_word.data[0] == '-') { throw std::invalid_argument("Query word contains \"minus\" character."); }
        if (query_word.is_minus && query_word.data.empty()) {
            throw std::invalid_argument("Minus query word is empty.");
        }
        if (!IsValidWord(query_word.data)) {
            throw std::invalid_argument("Query word contains invalid characters.");
        }
        
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            }
            else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}


