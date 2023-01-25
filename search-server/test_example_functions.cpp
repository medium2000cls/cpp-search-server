#include "test_example_functions.h"
#include "paginator.h"
#include "process_queries.h"

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func,
        unsigned int line, const std::string& hint)
{
    using namespace std;
    
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

void TestAddDocumentMustBeFoundFromQuery()
{
    using namespace std;
    
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {1, 2, 3});
    searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::ACTUAL, {1});
    
    {
        auto documents = searchServer.FindTopDocuments("белый"s);
        ASSERT_EQUAL(documents.size(), 1);
        ASSERT_EQUAL(documents[0].id, 0);
    }
    {
        auto documents = searchServer.FindTopDocuments("скворец"s);
        ASSERT_EQUAL(documents.size(), 1);
        ASSERT_EQUAL(documents[0].id, 3);
    }
}

void TestFindTopDocument()
{
    using namespace std;
    auto PrintDocument = [](const Document& document) -> string {
        string t  = "{ "s + "document_id = "s + to_string(document.id) + ", "s + "relevance = "s + to_string(
                document.relevance) + ", "s + "rating = "s + to_string(document.rating) + " }"s;
        //cout << t << endl;
        return t;
    };
    {
        SearchServer search_server("and with"s);
        int id = 0;
        vector<pair<string, DocumentStatus>> documents_text{{"white cat and yellow hat"s, DocumentStatus::ACTUAL},
                                                            {"curly cat curly tail"s,     DocumentStatus::ACTUAL},
                                                            {"nasty dog with big eyes"s,  DocumentStatus::ACTUAL},
                                                            {"nasty pigeon john"s,        DocumentStatus::ACTUAL},
                                                            {"white1 cat1 and yellow1 hat1"s, DocumentStatus::BANNED},
                                                            {"curly1 cat1 curly1 tail1"s,     DocumentStatus::BANNED},
                                                            {"nasty1 dog1 with big1 eyes1"s,  DocumentStatus::BANNED},
                                                            {"nasty1 pigeon1 john1"s,        DocumentStatus::BANNED},};
        
        for (const auto& [text, status] : documents_text) {
            search_server.AddDocument(++id, text, status, {1, 2});
        }
        vector<string> control_string{"{ document_id = 2, relevance = 1.386294, rating = 1 }",
                                      "{ document_id = 4, relevance = 0.462098, rating = 1 }",
                                      "{ document_id = 1, relevance = 0.346574, rating = 1 }",
                                      "{ document_id = 3, relevance = 0.346574, rating = 1 }",
                                      "{ document_id = 6, relevance = 1.386294, rating = 1 }",
                                      "{ document_id = 8, relevance = 0.462098, rating = 1 }",
                                      "{ document_id = 5, relevance = 0.346574, rating = 1 }",
                                      "{ document_id = 7, relevance = 0.346574, rating = 1 }"};
    
        {
            int test_counter = 0;
            for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
                ASSERT(control_string[test_counter] == PrintDocument(document));
                ++test_counter;
            }
            // последовательная версия
            test_counter = 0;
            for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s)) {
                ASSERT(control_string[test_counter] == PrintDocument(document));
                ++test_counter;
            }
            // параллельная версия
            test_counter = 0;
            for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s)) {
                ASSERT(control_string[test_counter] == PrintDocument(document));
                ++test_counter;
            }
        }
        {
            int test_counter = 4;
            for (const Document& document : search_server.FindTopDocuments("curly1 nasty1 cat1"s, DocumentStatus::BANNED)) {
                ASSERT(control_string[test_counter] == PrintDocument(document));
                ++test_counter;
            }
            // последовательная версия
            test_counter = 4;
            for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly1 nasty1 cat1"s, DocumentStatus::BANNED)) {
                ASSERT(control_string[test_counter] == PrintDocument(document));
                ++test_counter;
            }
            // параллельная версия
            test_counter = 4;
            for (const Document& document : search_server.FindTopDocuments(execution::par, "curly1 nasty1 cat1"s, DocumentStatus::BANNED)) {
                ASSERT(control_string[test_counter] == PrintDocument(document));
                ++test_counter;
            }
        }
        {
            int test_counter = 0;
            for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s,
                    [](int document_id, [[maybe_unused]]DocumentStatus status, [[maybe_unused]]int rating) { return document_id % 2 == 0; })) {
                ASSERT(control_string[test_counter] == PrintDocument(document));
                ++test_counter;
            }
            // последовательная версия
            test_counter = 0;
            for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s,
                    [](int document_id, [[maybe_unused]]DocumentStatus status, [[maybe_unused]]int rating) { return document_id % 2 == 0; })) {
                ASSERT(control_string[test_counter] == PrintDocument(document));
                ++test_counter;
            }
            // параллельная версия
            test_counter = 0;
            for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s,
                    [](int document_id, [[maybe_unused]]DocumentStatus status, [[maybe_unused]]int rating) { return document_id % 2 == 0; })) {
                ASSERT(control_string[test_counter] == PrintDocument(document));
                ++test_counter;
            }
        }
    }
}

void TestSupportStopWordsTheyExcludedDocumentText()
{
    using namespace std;
    
    {
        SearchServer searchServer;
        searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
        searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {1, 2, 3});
        searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
        searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::ACTUAL, {1});
        auto documents = searchServer.FindTopDocuments("и"s);
        ASSERT_EQUAL(documents.size(), 1);
        ASSERT_EQUAL(documents[0].id, 0);
    }
    {
        SearchServer searchServer("и в на"s);
        searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
        searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {1, 2, 3});
        searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
        searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::ACTUAL, {1});
        auto documents = searchServer.FindTopDocuments("и"s);
        ASSERT_EQUAL(documents.size(), 0);
    }
    
}

void TestSupportMinusWordsTheyExcludedDocumentFromResult()
{
    using namespace std;
    
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {1, 2, 3});
    searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::ACTUAL, {1});
    
    {
        auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s);
        ASSERT_EQUAL(documents.size(), 4);
    }
    {
        auto documents = searchServer.FindTopDocuments("пушистый -ухоженный кот"s);
        ASSERT_EQUAL(documents.size(), 2);
    }
}

void TestMatchDocumentCheckReturnWords()
{
    {
        using namespace std;
        using Word = std::string_view;
        
        SearchServer searchServer;
        searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
        searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {1, 2, 3});
        searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
        searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::ACTUAL, {1});
        
        const set<Word> test_word = {"белый", "кот"};
        {
            tuple<vector<Word>, DocumentStatus> match_document = searchServer.MatchDocument(
                    "белый белый  кот пушистый хвост"s, 0);
            const auto& [words, _] = match_document;
            ASSERT_EQUAL(words.size(), 2);
            for (const auto& word : words) {
                ASSERT(test_word.count(word) != 0);
            }
        }
        {
            tuple<vector<Word>, DocumentStatus> match_document = searchServer.MatchDocument(execution::seq,
                    "белый белый  кот пушистый хвост"s, 0);
            const auto& [words, _] = match_document;
            ASSERT_EQUAL(words.size(), 2);
            for (const auto& word : words) {
                ASSERT(test_word.count(word));
            }
        }
        {
            tuple<vector<Word>, DocumentStatus> match_document = searchServer.MatchDocument(execution::par,
                    "белый белый  кот пушистый хвост"s, 0);
            const auto& [words, _] = match_document;
            ASSERT_EQUAL(words.size(), 2);
            for (const auto& word : words) {
                ASSERT(test_word.count(word));
            }
        }
        {
            tuple<vector<Word>, DocumentStatus> match_document = searchServer.MatchDocument(
                    "белый белый  -кот пушистый хвост"s, 0);
            const auto& [words, _] = match_document;
            ASSERT_EQUAL(words.size(), 0);
        }
        {
            tuple<vector<Word>, DocumentStatus> match_document = searchServer.MatchDocument(execution::par,
                    "белый белый  -кот пушистый хвост"s, 0);
            const auto& [words, _] = match_document;
            ASSERT_EQUAL(words.size(), 0);
        }
    }
    {
        using namespace std;
        
        const std::string general_document_text = "cat dog puppy kitty"s;
        constexpr int general_document_id = 1;
        const std::vector<int> general_ratings = {1, 2, 3, 4};
        
        {
            const std::string kQueryWithMinusWords = "cat -puppy"s;
            SearchServer server;
            
            server.AddDocument(1, "cat home"s, DocumentStatus::ACTUAL, general_ratings);
            server.AddDocument(2, "cat puppy home"s, DocumentStatus::ACTUAL, general_ratings);
            
            auto found_documents = server.FindTopDocuments(kQueryWithMinusWords);
            ASSERT_EQUAL_HINT(found_documents.size(), 1, "Server returns ONLY the documents without minus words, which were in query"s);
            
            found_documents = server.FindTopDocuments("home puppy -puppy");
            ASSERT_EQUAL_HINT(found_documents.size(), 1, "Server returns ONLY the documents without minus words, if query has a word and the same word as minus word"s);
            
            auto [matching_words, _] = server.MatchDocument(kQueryWithMinusWords, 1);
            ASSERT_HINT(!matching_words.empty(), "Server matches the words if query has a minus word which absent in the document"s);
            
            std::tie(matching_words, _) = server.MatchDocument(kQueryWithMinusWords, 2);
            ASSERT_HINT(matching_words.empty(), "Server does not match any word for the document if it has at least one minus word"s);
        }
        
        {
            int max_special_symbol_index{31};
            SearchServer server;
            
            server.AddDocument(general_document_id, general_document_text, DocumentStatus::ACTUAL, general_ratings);
            
            try {
                auto matching_result = server.MatchDocument(""s, general_document_id);
            }
            catch (std::invalid_argument& e) {
                ASSERT(true);
                
            }
            catch (...) {
                ASSERT_HINT(false, "Server should throw if matching query is empty"s);
            }
            
            std::string query;
            for (int symbol_index = 0; symbol_index < max_special_symbol_index; ++symbol_index) {
                query = "cat"s + static_cast<char>(symbol_index) + " dog"s;
                try {
                    auto matching_result = server.MatchDocument(query, general_document_id);
                }
                catch (std::invalid_argument& e) {
                    ASSERT(true);
                }
                catch (...) {
                    ASSERT_HINT(false, "Server should throw if at least one special symbol is detected"s);
                }
            }
            
            {
                auto [matching_words, _] = server.MatchDocument("none bug"s, general_document_id);
                ASSERT_HINT(matching_words.empty(), "Server does not match any word if it is not in a document"s);
            }
            {
                auto [matching_words, _] = server.MatchDocument("puppy cat"s, general_document_id);
                ASSERT_EQUAL_HINT(matching_words.size(), 2, "Server matches all words from query in document"s);
                
                std::sort(matching_words.begin(), matching_words.end());
                
                //auto t = matching_words[0];
                ASSERT_HINT(matching_words[0] == "cat"s, "Server match the exact word from the query"s);
                
                ASSERT_HINT(matching_words[1] == "puppy"s, "Server match the exact word from the query"s);
                
                std::tie(matching_words, _) = server.MatchDocument("cat dog -puppy", general_document_id);
                ASSERT_HINT(matching_words.empty(),
                        "Server does not match any word for the document if it has at least on minus word"s);
            }
        }
    }
}

void TestSortDocumentByRelevance()
{
    using namespace std;
    
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {1, 2, 3});
    searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::ACTUAL, {1});
    
    auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(documents.size(), 4);
    
    for (int i = 1; i < static_cast<int>(documents.size()); ++i) {
        ASSERT(documents[i - 1].relevance >= documents[i].relevance);
    }
}

void TestComputeRatingDocument()
{
    using namespace std;
    
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый"s, DocumentStatus::ACTUAL, {1}); //1
    searchServer.AddDocument(1, "пушистый"s, DocumentStatus::ACTUAL, {-1, -2}); //1
    searchServer.AddDocument(2, "пёс"s, DocumentStatus::ACTUAL, {1, 2, 3}); //2
    searchServer.AddDocument(3, "скворец"s, DocumentStatus::ACTUAL, {-1, -2, -3, -4}); //2
    
    {
        auto documents = searchServer.FindTopDocuments("белый"s);
        ASSERT_EQUAL(documents[0].rating, 1);
    }
    {
        auto documents = searchServer.FindTopDocuments("пушистый"s);
        ASSERT_EQUAL(documents[0].rating, -1);
    }
    {
        auto documents = searchServer.FindTopDocuments("пёс"s);
        ASSERT_EQUAL(documents[0].rating, 2);
    }
    {
        auto documents = searchServer.FindTopDocuments("скворец"s);
        ASSERT_EQUAL(documents[0].rating, -2);
    }
}

void TestFilteringResultSearch()
{
    using namespace std;
    
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {-1, -2, -3});
    searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {-1});
    
    {
        auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s,
                [](int document_id, [[maybe_unused]] DocumentStatus status,
                        [[maybe_unused]] int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL(documents.size(), 2);
        ASSERT_EQUAL(documents[0].id, 2);
        ASSERT_EQUAL(documents[1].id, 0);
    }
    {
        auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s,
                []([[maybe_unused]] int document_id, [[maybe_unused]] DocumentStatus status,
                        int rating) { return rating > 0; });
        ASSERT_EQUAL(documents.size(), 2);
        ASSERT(documents[0].rating > 0);
        ASSERT(documents[1].rating > 0);
    }
    
}

void TestSearchDocumentByStatus()
{
    using namespace std;
    
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::IRRELEVANT, {-1, -2, -3});
    searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::REMOVED, {1, 2, 3, 4});
    searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {-1});
    
    {
        auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(documents[0].id, 0);
    }
    {
        auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(documents[0].id, 1);
    }
    {
        auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(documents[0].id, 2);
    }
    {
        auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(documents[0].id, 3);
    }
    
}

void TestComputeRelevanceFoundDocument()
{
    using namespace std;
    
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый кот модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {-1, -2, -3});
    searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::ACTUAL, {-1});
    {
        auto documents = searchServer.FindTopDocuments("белый ухоженный кот"s);
        double relevance0 = 0.51986038542;
        double relevance1 = 0.23104906018;
        double relevance2 = 0.17328679514;
        double relevance3 = 0.17328679514;
        ASSERT(abs(documents[0].relevance - relevance0) < ACCURACY_COMPARISON);
        ASSERT(abs(documents[1].relevance - relevance1) < ACCURACY_COMPARISON);
        ASSERT(abs(documents[2].relevance - relevance2) < ACCURACY_COMPARISON);
        ASSERT(abs(documents[3].relevance - relevance3) < ACCURACY_COMPARISON);
    }
}

void TestCorrectPaginationFoundDocument()
{
    using namespace std;
    
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый кот модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {-1, -2, -3});
    searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::ACTUAL, {-1});
    {
        auto documents = searchServer.FindTopDocuments("белый ухоженный кот"s);
        int page_size = 4;
        const auto pages = Paginate(documents, page_size);
        ASSERT(distance(pages.begin(), pages.end()) == 1);
    }
    {
        auto documents = searchServer.FindTopDocuments("белый ухоженный кот"s);
        int page_size = 3;
        const auto pages = Paginate(documents, page_size);
        ASSERT(distance(pages.begin(), pages.end()) == 2);
    }
    {
        auto documents = searchServer.FindTopDocuments("белый ухоженный кот"s);
        int page_size = 2;
        const auto pages = Paginate(documents, page_size);
        ASSERT(distance(pages.begin(), pages.end()) == 2);
    }
    {
        auto documents = searchServer.FindTopDocuments("белый ухоженный кот"s);
        int page_size = 1;
        const auto pages = Paginate(documents, page_size);
        ASSERT(distance(pages.begin(), pages.end()) == 4);
    }
}

void TestGetWordFrequencies()
{
    using namespace std;
    
    SearchServer searchServer;
    searchServer.AddDocument(0, "white cat fashionable collar"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, {-1, -2, -3});
    searchServer.AddDocument(2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "well-groomed starling eugene"s, DocumentStatus::ACTUAL, {-1});
    {
        map<string_view, double> words_freg_sample{{"white",       0.25},
                                                   {"cat",         0.25},
                                                   {"fashionable", 0.25},
                                                   {"collar",      0.25}};
        const auto result = searchServer.GetWordFrequencies(0);
        ASSERT(result == words_freg_sample);
    }
    {
        map<string_view, double> words_freg_sample{{"cat",    0.25},
                                                   {"fluffy", 0.5},
                                                   {"tail",   0.25}};
        const auto& result = searchServer.GetWordFrequencies(1);
        ASSERT(result == words_freg_sample);
    }
    {
        map<string_view, double> words_freg_sample{{"well-groomed", 0.25},
                                                   {"dog",          0.25},
                                                   {"expressive",   0.25},
                                                   {"eyes",         0.25}};
        const auto& result = searchServer.GetWordFrequencies(2);
        ASSERT(result == words_freg_sample);
    }
    {
        map<string_view, double> words_freg_sample{{"well-groomed", 0.33333333333333331},
                                                   {"starling",     0.33333333333333331},
                                                   {"eugene",       0.33333333333333331}};
        const auto& result = searchServer.GetWordFrequencies(3);
        ASSERT(result == words_freg_sample);
    }
    {
        map<string_view, double> words_freg_sample;
        const auto& result = searchServer.GetWordFrequencies(4);
        ASSERT(result == words_freg_sample);
    }
    
}

void TestRemoveDocument()
{
    using namespace std;
    
    {
        SearchServer searchServer;
        searchServer.AddDocument(0, "white cat fashionable collar"s, DocumentStatus::ACTUAL, {1, 2});
        searchServer.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, {-1, -2, -3});
        searchServer.AddDocument(2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
        searchServer.AddDocument(3, "well-groomed starling eugene"s, DocumentStatus::ACTUAL, {-1});
        {
            map<string_view, double> words_freg_sample{{"cat",    0.25},
                                                       {"fluffy", 0.5},
                                                       {"tail",   0.25}};
            const auto& result = searchServer.GetWordFrequencies(1);
            ASSERT(result == words_freg_sample);
        }
        {
            map<string_view, double> words_freg_sample;
            searchServer.RemoveDocument(1);
            const auto& result = searchServer.GetWordFrequencies(1);
            ASSERT(result == words_freg_sample);
        }
    }
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
        
        const string query = "curly and funny"s;
        
        // однопоточная версия
        search_server.RemoveDocument(5);
        ASSERT(search_server.FindTopDocuments(query).size() == 3);
        // однопоточная версия
        search_server.RemoveDocument(execution::seq, 1);
        ASSERT(search_server.FindTopDocuments(query).size() == 2);
        // многопоточная версия
        search_server.RemoveDocument(execution::par, 2);
        ASSERT(search_server.FindTopDocuments(query).size() == 1);
    }
}

void TestProcessQueries()
{
    using namespace std;
    SearchServer searchServer;
    SearchServer search_server("and with"s);
    int id = 0;
    for (const string& text : {"funny pet and nasty rat"s,
                               "funny pet with curly hair"s,
                               "funny pet and not very nasty rat"s,
                               "pet with rat and rat and rat"s,
                               "nasty rat with curly hair"s,}) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }
    const vector<string> queries = {"nasty rat -not"s, "not very funny nasty pet"s, "curly hair"s};
    id = 0;
    vector<size_t> example{3, 5, 2};
    for (const auto& documents : ProcessQueries(search_server, queries)) {
        ASSERT(documents.size() == example[id++]);
    }
}

void TestIteratorTree()
{
    using namespace std;
    {
        vector<vector<int>> vector_test{{1,  2,  3},
                                        {4,  5,  6},
                                        {7,  8,  9},
                                        {10, 11, 12},
                                        {13, 14, 15},
                                        {16, 17, 18},
                                        {19, 20, 21},
                                        {22, 23, 24},
                                        {25, 26, 27},
                                        {28, 29, 30}};
        
        ListFromVecInDegree<vector<vector<int>>, int> joined(vector_test);
        int i = 0;
        for (auto s : joined) {
            ASSERT(s == ++i);
        }
        auto dist = std::distance(joined.begin(), joined.end());
        ASSERT(dist == 30);
    }
    {
        vector<vector<vector<int>>> vector_test{{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}, {13, 14, 15}, {16, 17, 18}, {19, 20, 21}, {22, 23, 24}, {25, 26, 27}, {28, 29, 30}},
                                        {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}, {13, 14, 15}, {16, 17, 18}, {19, 20, 21}, {22, 23, 24}, {25, 26, 27}, {28, 29, 30}},
                                        {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}, {13, 14, 15}, {16, 17, 18}, {19, 20, 21}, {22, 23, 24}, {25, 26, 27}, {28, 29, 30}}};
        
        ListFromVecInDegree<vector<vector<vector<int>>>, int> joined(vector_test);
        int i = 0;
        for (auto s : joined) {
            ASSERT(s == ++i);
            if (i == 30) { i = 0; }
        }
        auto dist = std::distance(joined.begin(), joined.end());
        ASSERT(dist == 90);
    }
    {
        list<list<list<int>>> vector_test{{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}, {13, 14, 15}, {16, 17, 18}, {19, 20, 21}, {22, 23, 24}, {25, 26, 27}, {28, 29, 30}},
                                        {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}, {13, 14, 15}, {16, 17, 18}, {19, 20, 21}, {22, 23, 24}, {25, 26, 27}, {28, 29, 30}},
                                        {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}, {13, 14, 15}, {16, 17, 18}, {19, 20, 21}, {22, 23, 24}, {25, 26, 27}, {28, 29, 30}}};
    
        ListFromVecInDegree<list<list<list<int>>>, int> joined(vector_test);
        int i = 0;
        for (auto s : joined) {
            ASSERT(s == ++i);
            if (i == 30) { i = 0; }
        }
        auto dist = std::distance(joined.begin(), joined.end());
        ASSERT(dist == 90);
    }
    {
        set<set<set<int>>> vector_test{{{1,  2,  3},  {4,  5,  6},  {7,  8,  9},  {10, 11, 12}, {13, 14, 15}, {16, 17, 18}, {19, 20, 21}, {22, 23, 24}, {25, 26, 27}, {28, 29, 30}},
                                       {{31, 32, 33}, {34, 35, 36}, {37, 38, 39}, {40, 41, 42}, {43, 44, 45}, {46, 47, 48}, {49, 50, 51}, {52, 53, 54}, {55, 56, 57}, {58, 59, 60}, {61, 62, 63}},
                                       {{64, 65, 66}, {67, 68, 69}, {70, 71, 72}, {73, 74, 75}, {76, 77, 78}, {79, 80, 81}, {82, 83, 84}, {85, 86, 87}, {88, 89, 90}, {91, 92, 93}, {94, 95, 96}, {97, 98, 99}}};
        
        ListFromVecInDegree<set<set<set<int>>>, int> joined(vector_test);
        int i = 0;
        for (auto s : joined) {
            ASSERT(s == ++i);
            cout << s << " = " << i << endl;
        }
        auto dist = std::distance(joined.begin(), joined.end());
        cout << "dist " << dist << endl;
        ASSERT(dist == 99);
    }
    {
        set<set<set<int>>> vector_test{{{1,  2,  3},  {2,  5,  6},  {2,  8,  9}},
                                       {{10, 11, 12}, {13, 14, 15}, {16, 17, 18}},
                                       {{19, 20, 21}, {22, 23, 24}, {25, 26, 27}, {28, 29, 30}}};
        vector<int> vector_sample {1,  2,  3,  2,  5,  6,  2,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};
        ListFromVecInDegree<set<set<set<int>>>, int> joined(vector_test);
        int i = 0;
        for (auto s : joined) {
            cout << s << " = " << vector_sample[i] << endl;
            ASSERT(s == vector_sample[i]);
            ++i;
        }
        auto dist = std::distance(joined.begin(), joined.end());
        cout << "dist " << dist << endl;
        ASSERT(dist == 30);
    }
    {
        vector<vector<int>> vector_test{{1,  2,  3},
                                        {},
                                        {7,  8,  9},
                                        {10, 11, 12},
                                        {13},
                                        {16, 17, 18},
                                        {19, 20, 21},
                                        {22, 23},
                                        {25, 26, 27},
                                        {28, 29, 30}};
        
        ListFromVecInDegree<vector<vector<int>>, int> joined(vector_test);
        auto dist = std::distance(joined.begin(), joined.end());
        ASSERT(dist == 24);
    }
    {
        vector<vector<int>> vector_test {};
        ListFromVecInDegree<vector<vector<int>>, int> joined(vector_test);
        int i = 0;
        for (auto s : joined) {
            ASSERT(s == ++i);
        }
        auto dist = std::distance(joined.begin(), joined.end());
        ASSERT(dist == 0);
    }
    {
        vector<int> vector_test2{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        ListFromVecInDegree<vector<int>, int> joined2(vector_test2);
        auto dist2 = std::distance(joined2.begin(), joined2.end());
        ASSERT(dist2 == 10);
    }
}

void TestProcessQueriesJoined()
{
    using namespace std;
    SearchServer searchServer;
    SearchServer search_server("and with"s);
    int id = 0;
    for (const string& text : {"funny pet and nasty rat"s,
                               "funny pet with curly hair"s,
                               "funny pet and not very nasty rat"s,
                               "pet with rat and rat and rat"s,
                               "nasty rat with curly hair"s,}) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }
    const vector<string> queries = {"nasty rat -not"s, "not very funny nasty pet"s, "curly hair"s};
    auto joined = ProcessQueriesJoined(search_server, queries);
    ASSERT(std::distance(joined.begin(), joined.end()) == 10);
    
}

void TestSearchServer()
{
    RUN_TEST (TestAddDocumentMustBeFoundFromQuery);
    RUN_TEST (TestFindTopDocument);
    RUN_TEST (TestSupportStopWordsTheyExcludedDocumentText);
    RUN_TEST (TestSupportMinusWordsTheyExcludedDocumentFromResult);
    RUN_TEST (TestMatchDocumentCheckReturnWords);
    RUN_TEST (TestSortDocumentByRelevance);
    RUN_TEST (TestComputeRatingDocument);
    RUN_TEST (TestFilteringResultSearch);
    RUN_TEST (TestSearchDocumentByStatus);
    RUN_TEST (TestComputeRelevanceFoundDocument);
    RUN_TEST (TestCorrectPaginationFoundDocument);
    RUN_TEST (TestGetWordFrequencies);
    RUN_TEST (TestRemoveDocument);
    RUN_TEST (TestProcessQueries);
    RUN_TEST (TestIteratorTree);
    RUN_TEST (TestProcessQueriesJoined);
}

