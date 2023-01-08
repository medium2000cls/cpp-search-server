#include <numeric>
#include <cmath>
#include "search_server.h"

SearchServer::SearchServer() {}

SearchServer::SearchServer(const std::string& stop_words_text) : SearchServer(std::string_view(stop_words_text)){}

SearchServer::SearchServer(const std::string_view& stop_words_text) : SearchServer(
        SplitIntoWordsView(stop_words_text))  // Invoke delegating constructor from string container
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Stop-word contains invalid characters.");
    }
}

void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
        const std::vector<int>& ratings)
{
    if (document_id < 0) { throw std::invalid_argument("Document ID cannot be less than zero"); }
    if (documents_.count(document_id) != 0) { throw std::invalid_argument("Document ID cannot be repeated"); }
    
    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view& word_view : words) {
        if (!IsValidWord(word_view)) { throw std::invalid_argument("Word in document contains invalid characters."); }
        auto [it_word, _] = words_.emplace(word_view);
        word_to_document_freqs_[*it_word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][*it_word] += inv_word_count;
    }
    order_documents_id_.insert(document_id);
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const
{
    return FindTopDocuments(raw_query,
            [status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]] int rating) {
                return document_status == status;
            });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query, int document_id) const
{
    //LOG_DURATION;
    auto query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;
    for (const std::string_view& word_view : query.minus_words) {
        if (document_to_word_freqs_.at(document_id).count(word_view)) {
            std::vector<std::string_view> v;
            return std::tie(v, documents_.at(document_id).status);
        }
    }
    for (const std::string_view& word_view : query.plus_words) {
        if (document_to_word_freqs_.at(document_id).count(word_view)) {
            auto it = words_.find(word_view);
            matched_words.emplace_back(*it);
        }
    }
    return std::tie(matched_words, documents_.at(document_id).status);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy,
        const std::string_view& raw_query, int document_id) const
{
    //LOG_DURATION;
    Query query = ParseQueryDuplicate(raw_query);
    
    bool minus_word_is_find = std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
            [document_id, this](const std::string_view& word_view) -> bool {
                const auto it = word_to_document_freqs_.find(word_view);
                return it != word_to_document_freqs_.end() && it->second.count(document_id);
            });
    if (minus_word_is_find) {
        std::vector<std::string_view> v;
        return std::tie(v, documents_.at(document_id).status);
    }
    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto end_it = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(),matched_words.begin(),
            [document_id, this](std::string_view word_view) -> bool {
                return document_to_word_freqs_.at(document_id).count(word_view);
            });
    
    std::sort(std::execution::par, matched_words.begin(), end_it);
    end_it = std::unique(std::execution::par, matched_words.begin(), end_it);
    matched_words.erase(end_it, matched_words.end());
    
    std::transform(std::execution::par, matched_words.begin(), matched_words.end(), matched_words.begin(),
            [this](std::string_view word_view) -> std::string_view {
                auto it = words_.find(word_view);
                if(it != words_.end()) { word_view = *it; }
                else{ throw std::invalid_argument("Word \"" + std::string(word_view) + "\" is not find in words container."); }
                return word_view;
            });
    
    return std::tie(matched_words, documents_.at(document_id).status);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy, const std::string_view& raw_query,
        int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    if (!document_to_word_freqs_.count(document_id)) {
        const static std::map<std::string_view, double> word_frequencies_empty;
        return word_frequencies_empty;
    }
    return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id)
{
    SearchServer::RemoveDocument(std::execution::seq, document_id);
}

bool SearchServer::IsValidWord(const std::string_view& word)
{
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

bool SearchServer::IsStopWord(const std::string_view& word) const
{
    return stop_words_.count(std::string(word)) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const
{
    std::vector<std::string_view> words;
    for (const std::string_view& word : SplitIntoWordsView(text)) {
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text_view) const
{
    bool is_minus = false;
    // Word shouldn't be empty
    if (text_view[0] == '-') {
        is_minus = true;
        text_view = text_view.substr(1);
    }
    if (text_view[0] == '-') { throw std::invalid_argument("Query word contains \"minus\" character."); }
    if (is_minus && text_view.empty()) { throw std::invalid_argument("Minus query word is empty."); }
    if (!IsValidWord(text_view)) { throw std::invalid_argument("Query word contains invalid characters."); }
    
    return {text_view, is_minus, IsStopWord(text_view)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const
{
    Query query(std::forward<Query>(ParseQueryDuplicate(text)));
    auto sort_unique_erase = [](std::vector<std::string_view>& words) {
        std::sort(words.begin(), words.end());
        auto it_unique = std::unique(words.begin(), words.end());
        words.erase(it_unique, words.end());
    };
    sort_unique_erase(query.plus_words);
    sort_unique_erase(query.minus_words);
    return query;
}


SearchServer::Query SearchServer::ParseQueryDuplicate(const std::string_view& text) const
{
    Query query;
    for (const std::string_view& word : SplitIntoWordsView(text)) {
        if (word.empty()) { continue; }
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.emplace_back(query_word.data);
            }
            else {
                query.plus_words.emplace_back(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const
{
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}


