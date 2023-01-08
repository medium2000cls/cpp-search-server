#include "search_server.h"

#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <thread>

#include "log_duration.h"
#include "process_queries.h"
#include "test_example_functions.h"

using namespace std;

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution(static_cast<int>('a'), static_cast<int>('z'))(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename ExecutionPolicy>
void Test(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();
    int word_count = 0;
    for (int id = 0; id < document_count; ++id) {
        const auto [words, status] = search_server.MatchDocument(policy, query, id);
        word_count += words.size();
    }
    cout << word_count << endl;
}

#define TEST(policy) Test(#policy, search_server, query, execution::policy)







void PrintMatchDocumentResultUTest(int document_id, const std::vector<std::string_view>& words,DocumentStatus status) {
    std::cout << "{ "
            << "document_id = " << document_id << ", "
            << "status = " << static_cast<int>(status) << ", "
            << "words =";
    for (const string_view & word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}" << std::endl;
}

void PrintDocumentUTest(const Document& document) {
    std::cout << "{ "
            << "document_id = " << document.id << ", "
            << "relevance = " << document.relevance << ", "
            << "rating = " << document.rating << " }" << std::endl;
}

void Test() {
    const std::vector<int> ratings1 = {1, 2, 3, 4, 5};
    const std::vector<int> ratings2 = {-1, -2, 30, -3, 44, 5};
    const std::vector<int> ratings3 = {12, -20, 80, 0, 8, 0, 0, 9, 67};
    std::string stop_words = "and in on";
    SearchServer search_server(stop_words);
    
    search_server.AddDocument(0, "white cat and fashion collar", DocumentStatus::ACTUAL, ratings1);
    search_server.AddDocument(1, "fluffy cat fluffy tail", DocumentStatus::ACTUAL, ratings2);
    search_server.AddDocument(2, "groomed dog expressive eye", DocumentStatus::ACTUAL,
            ratings3);
    search_server.AddDocument(3, "white fashion cat", DocumentStatus::IRRELEVANT, ratings1);
    search_server.AddDocument(4, "fluffy cat dog", DocumentStatus::IRRELEVANT, ratings2);
    search_server.AddDocument(5, "groomed collar expressive eye",
            DocumentStatus::IRRELEVANT, ratings3);
    search_server.AddDocument(6, "cat and collar", DocumentStatus::BANNED, ratings1);
    search_server.AddDocument(7, "dog and tail", DocumentStatus::BANNED, ratings2);
    search_server.AddDocument(8, "fashion dog fluffy tail", DocumentStatus::BANNED, ratings3);
    search_server.AddDocument(9, "cat fluffy collar", DocumentStatus::REMOVED, ratings1);
    search_server.AddDocument(10, "groomed cat and dog", DocumentStatus::REMOVED, ratings2);
    search_server.AddDocument(11, "tail and expressive eye", DocumentStatus::REMOVED, ratings3);
    
    const std::string query = "fluffy groomed cat -collar";
    const auto documents = search_server.FindTopDocuments(query);
    
    std::cout << "Top documents for query:" << std::endl;
    for (const Document& document : documents) {
        PrintDocumentUTest(document);
    }
    
    std::cout << "Documents' statuses:" << std::endl;
    const int document_count = search_server.GetDocumentCount();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        const auto [words, status] = search_server.MatchDocument( query, document_id);
        
        PrintMatchDocumentResultUTest(document_id, words, status);
    }
}





int main()
{
    
    TestSearchServer();
    {
        SearchServer search_server("and with"s);

        int id = 0;
        for (const string& text : {"funny pet and nasty rat"s,
                                   "funny pet with curly hair"s,
                                   "funny pet and not very nasty rat"s,
                                   "pet with rat and rat and rat"s,
                                   "nasty rat with curly hair"s,}) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
        }

        const string query = "curly and funny -not"s;

        {
            const auto [words, status] = search_server.MatchDocument(query, 1);
            cout << words.size() << " words for document 1"s << endl;
            // 1 words for document 1
        }

        {
            const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
            cout << words.size() << " words for document 2"s << endl;
            // 2 words for document 2
        }

        {
            const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
            cout << words.size() << " words for document 3"s << endl;
            // 0 words for document 3
        }
    }
    {
        mt19937 generator;

        const auto dictionary = GenerateDictionary(generator, 1000, 10);
        const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

        const string query = GenerateQuery(generator, dictionary, 500, 0.1);

        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
        }
        {
            LOG_DURATION("seq");
            const int document_count = search_server.GetDocumentCount();
            int word_count = 0;
            for (int id = 0; id < document_count; ++id) {
                const auto [words, status] = search_server.MatchDocument(execution::seq, query, id);
                word_count += words.size();
            }
            cout << word_count << endl;
        }
        {
            LOG_DURATION("par");
            const int document_count_1 = search_server.GetDocumentCount();
            int word_count_1 = 0;
            for (int id_1 = 0; id_1 < document_count_1; ++id_1) {
                const auto [words_1, status_1] = search_server.MatchDocument(execution::par, query, id_1);
                word_count_1 += words_1.size();
            }
            cout << word_count_1 << endl;
        }
    }
}