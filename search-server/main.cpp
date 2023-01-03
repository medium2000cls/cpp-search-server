#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "search_server.h"
#include "process_queries.h"
#include "log_duration.h"
#include "test_example_functions.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "paginator.h"
#include "remove_duplicates.h"


using namespace std;

string GenerateWord(mt19937& generator, int max_length)
{
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
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
string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int max_word_count) {
    const int word_count = uniform_int_distribution(1, max_word_count)(generator);
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
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

int main() {
    //Unit тесты
    TestSearchServer();
    //Тестирование удаления дубликатов
    {
        using namespace std;
        TestSearchServer();
        // Если вы видите эту строку, значит все тесты прошли успешно
        cout << "Search server testing finished"s << endl;
        SearchServer search_server("and with"s);
        
        search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
        
        // дубликат документа 2, будет удалён
        search_server.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
        
        // отличие только в стоп-словах, считаем дубликатом
        search_server.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});
        
        // множество слов такое же, считаем дубликатом документа 1
        search_server.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
        
        // добавились новые слова, дубликатом не является
        search_server.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
        
        // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
        search_server.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});
        
        // есть не все слова, не является дубликатом
        search_server.AddDocument(8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});
        
        // слова из разных документов, не является дубликатом
        search_server.AddDocument(9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
        
        cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
        RemoveDuplicates(search_server);
        cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;
    }
    //Фреймворк параллельных запросов
    {
        mt19937 generator;
        const auto dictionary = GenerateDictionary(generator, 2'000, 25);
        const auto documents = GenerateQueries(generator, dictionary, 20'000, 10);
        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
        }
        const auto queries = GenerateQueries(generator, dictionary, 2'000, 7);
        {
            LOG_DURATION("ProcessQueries");
            const auto documents_lists = ProcessQueries(search_server, queries);
        }
        {
            LOG_DURATION("ProcessQueriesJoined");
            const auto documents_lists = ProcessQueriesJoined(search_server, queries);
        }
    }
}

