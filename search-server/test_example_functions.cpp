#include "test_example_functions.h"
#include "paginator.h"

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned int line,
                const std::string& hint)
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
    using namespace std;
    
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    set<string> test_word = {"белый", "кот"};
    {
        tuple<vector<string>, DocumentStatus> match_document = searchServer.MatchDocument("белый кот пушистый хвост"s,
                0);
        const auto& [words, _] = match_document;
        ASSERT_EQUAL(words.size(), 2);
        for (const auto& word : words) {
            ASSERT(test_word.count(word) != 0);
        }
    }
    {
        tuple<vector<string>, DocumentStatus> match_document = searchServer.MatchDocument("белый -кот пушистый хвост"s,
                0);
        const auto& [words, _] = match_document;
        ASSERT_EQUAL(words.size(), 0);
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
        auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id,
                                                                                     [[maybe_unused]] DocumentStatus status,
                                                                                     [[maybe_unused]] int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL(documents.size(), 2);
        ASSERT_EQUAL(documents[0].id, 2);
        ASSERT_EQUAL(documents[1].id, 0);
    }
    {
        auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s, []([[maybe_unused]] int document_id,
                                                                                     [[maybe_unused]] DocumentStatus status,
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

void TestSearchServer()
{
    RUN_TEST (TestAddDocumentMustBeFoundFromQuery);
    RUN_TEST (TestSupportStopWordsTheyExcludedDocumentText);
    RUN_TEST (TestSupportMinusWordsTheyExcludedDocumentFromResult);
    RUN_TEST (TestMatchDocumentCheckReturnWords);
    RUN_TEST (TestSortDocumentByRelevance);
    RUN_TEST (TestComputeRatingDocument);
    RUN_TEST (TestFilteringResultSearch);
    RUN_TEST (TestSearchDocumentByStatus);
    RUN_TEST (TestComputeRelevanceFoundDocument);
    RUN_TEST (TestCorrectPaginationFoundDocument);
}

