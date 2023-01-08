#pragma once

#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <execution>
#include "document.h"
#include "string_processing.h"
#include "log_duration.h"


const double ACCURACY_COMPARISON = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer
{
public:
    explicit SearchServer();
    
    explicit SearchServer(const std::string& stop_words_text);
    
    explicit SearchServer(const std::string_view& stop_words_text);
    
    template<typename StringContainer>
    SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Stop-word contains invalid characters.");
        }
    }
    
    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
            const std::vector<int>& ratings);
    
    template<typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const
    {
        //LOG_DURATION;
        auto query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);
        
        std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            //Добавил глобальную константу
            if (std::abs(lhs.relevance - rhs.relevance) < ACCURACY_COMPARISON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const;
    
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy,
            const std::string_view& raw_query, int document_id) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy,
            const std::string_view& raw_query, int document_id) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query,
            int document_id) const;
    
    int GetDocumentCount() const;
    
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    
    void RemoveDocument(int document_id);
    
    template<typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id)
    {
        std::map<std::string_view, double>& words_freqs_to_erase = document_to_word_freqs_[document_id];
        std::vector<std::string> words(words_freqs_to_erase.size());
        std::transform(policy, words_freqs_to_erase.begin(), words_freqs_to_erase.end(), words.begin(),
                [](const auto& el) { return el.first; });
        
        std::for_each(policy, words.begin(), words.end(), [document_id, this](auto& word) {
            word_to_document_freqs_[word].erase(document_id);
        });
        document_to_word_freqs_.erase(document_id);
        documents_.erase(document_id);
        order_documents_id_.erase(document_id);
    }
    
    auto begin()
    {
        return order_documents_id_.begin();
    }
    
    auto end()
    {
        return order_documents_id_.end();
    }

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord
    {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    struct Query
    {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    
    const std::set<std::string, std::less<>> stop_words_;
    
    std::set<std::string, std::less<>> words_;
    
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    //Изменил тип контейнера
    std::set<int> order_documents_id_;
    
    static bool IsValidWord(const std::string_view& word);
    
    bool IsStopWord(const std::string_view& word) const;
    
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;
    
    static int ComputeAverageRating(const std::vector<int>& ratings);
    
    QueryWord ParseQueryWord(std::string_view text_view) const;
    
    Query ParseQuery(const std::string_view& text) const;
    
    Query ParseQueryDuplicate(const std::string_view& text) const;
    
    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;
    
    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const SearchServer::Query& query, DocumentPredicate document_predicate) const
    {
        std::map<int, double> document_to_relevance;
        for (const std::string_view& word_view : query.plus_words) {
            if (word_to_document_freqs_.count(word_view) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word_view);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word_view)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        
        for (const std::string_view& word_view : query.minus_words) {
            if (word_to_document_freqs_.count(word_view) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word_view)) {
                document_to_relevance.erase(document_id);
            }
        }
        
        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};
