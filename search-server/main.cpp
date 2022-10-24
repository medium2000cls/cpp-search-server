#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <numeric>
#include <cassert>
#include <iomanip>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ACCURACY_COMPARISON = 1e-6;

vector<string> SplitIntoWords(const string& text)
{
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    
    return words;
}

struct Document
{
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus
{
    ACTUAL, IRRELEVANT, BANNED, REMOVED,
};

class SearchServer
{
public:
    void SetStopWords(const string& text)
    {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }
    
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }
    
    vector<Document> FindTopDocuments(const string& raw_query) const
    {
        //Убрал дублирование перегрузки с предикатом, добавил вызов перегрузки со статусом
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }
    
    //Принято, небольшие типы лучше принимать и захватывать по значению.
    vector<Document> FindTopDocuments(const string& raw_query, const DocumentStatus document_status) const
    {
        return FindTopDocuments(raw_query, [document_status]([[maybe_unused]]int document_id,
                                                             DocumentStatus status,
                                                             [[maybe_unused]]int rating) { return status == document_status; });
    }
    
    template<typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query, const Predicate& predicate) const
    {
        const Query query = ParseQuery(raw_query);
        vector<Document> matched_documents = FindAllDocuments(query, predicate);
        // Принято, сделал перегрузку метода, он теперь принимает статус. Разобрался с constexpr, это спецификатор, который
        // говорит о том, что расчет будет произведен во время компиляции, а если этого сделать нельзя, то блок будет
        // обрабатываться уже во время выполнения программы.
        // В перегрузке, статус захватываю в лямбду.
        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            // Вынес константу в отдельную глобальную константу.
            if (abs(lhs.relevance - rhs.relevance) < ACCURACY_COMPARISON) {
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
    
    int GetDocumentCount() const
    {
        return documents_.size();
    }
    
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& rawQuery, int document_id) const
    {
        const Query query = ParseQuery(rawQuery);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };
    
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    //Принято, буду руководствоваться этим правилом.
    map<int, DocumentData> documents_;
    
    bool IsStopWord(const string& word) const
    {
        return stop_words_.count(word) > 0;
    }
    
    vector<string> SplitIntoWordsNoStop(const string& text) const
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
    
    static int ComputeAverageRating(const vector<int>& ratings)
    {
        if (ratings.empty()) {
            return 0;
        }
        //Принято, переменную оставил для читаемости.
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        
        return rating_sum / static_cast<int>(ratings.size());
    }
    
    struct QueryWord
    {
        string data;
        bool isMinus;
        bool isStop;
    };
    
    QueryWord ParseQueryWord(string text) const
    {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }
    
    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };
    
    Query ParseQuery(const string& text) const
    {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.isStop) {
                if (query_word.isMinus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }
    
    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const
    {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    
    //Исправил стиль, все имена методов и функций с маленькой буквы.
    template<typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate& predicate) const
    {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                //Записал результат в переменную
                auto document = documents_.at(document_id);
                bool filter_predicate = predicate(document_id, document.status, document.rating);
                if (filter_predicate) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        
        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document)
{
    cout << setprecision(12) << "{ "s << "document_id = "s << document.id << ", "s << "relevance = "s << document.relevance << ", "s
            << "rating = "s << document.rating << " }"s << endl;
}

// -------- Начало модульных тестов поисковой системы ----------
template<typename T>
ostream& operator<<(ostream& out, const vector<T>& container)
{
    bool is_first = true;
    out << "["s;
    for (const T& element : container) {
        if (is_first) {
            out << element;
            is_first = false;
        }
        else {
            out << ", "s << element;
        }
    }
    out << "]"s;
    return out;
}

template<typename T>
ostream& operator<<(ostream& out, const set<T>& container)
{
    bool is_first = true;
    out << "{"s;
    for (const T& element : container) {
        if (is_first) {
            out << element;
            is_first = false;
        }
        else {
            out << ", "s << element;
        }
    }
    out << "}"s;
    return out;
}

template<typename K,typename T>
ostream& operator<<(ostream& out, const map<K,T>& container)
{
    bool is_first = true;
    out << "{"s;
    for (const auto& [key, element] : container) {
        if (is_first) {
            out << key << ": " << element;
            is_first = false;
        }
        else {
            out << ", "s << key << ": " << element;
        }
    }
    out << "}"s;
    return out;
}

template<typename T, typename U>
void AssertEqualImpl(const T& t,const U& u,const string& t_str,const string& u_str,const string& file,const string& func,
                     unsigned line,const string& hint)
{
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint)
{
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

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T>
void RunTest(T& func, const string& func_name)
{
    func();
    cerr << func_name << " OK" << endl;
}
#define RUN_TEST(func) RunTest(func, #func)

//Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestAddDocumentMustBeFoundFromQuery() {
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
//Поддержка стоп-слов. Стоп-слова исключаются из текста документов.
void TestSupportStopWordsTheyExcludedDocumentText() {
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {1, 2, 3});
    searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::ACTUAL, {1});
    
    {
        auto documents = searchServer.FindTopDocuments("и"s);
        ASSERT_EQUAL(documents.size(), 1);
        ASSERT_EQUAL(documents[0].id, 0);
    }
    {
        searchServer.SetStopWords("и в на"s);
        auto documents = searchServer.FindTopDocuments("и"s);
        ASSERT_EQUAL(documents.size(), 0);
    }
    
}
//Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void TestSupportMinusWordsTheyExcludedDocumentFromResult() {
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
//Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса,
// присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestMatchDocumentCheckReturnWords() {
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    set<string> test_word = {"белый", "кот"};
    {
        tuple<vector<string>, DocumentStatus> match_document = searchServer.MatchDocument("белый кот пушистый хвост"s, 0);
        const auto& [words, _] = match_document;
        ASSERT_EQUAL(words.size(), 2);
        for(const auto& word : words){
            ASSERT(test_word.count(word) != 0);
        }
    }
    {
        tuple<vector<string>, DocumentStatus> match_document = searchServer.MatchDocument("белый -кот пушистый хвост"s, 0);
        const auto& [words, _] = match_document;
        ASSERT_EQUAL(words.size(), 0);
    }
}
//Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortDocumentByRelevance() {
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {1, 2, 3});
    searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::ACTUAL, {1});
    
    auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(documents.size(), 4);
    for(int i = 1; i < documents.size(); ++i){
        ASSERT(documents[i-1].relevance >= documents[i].relevance);
    }
}
//Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestComputeRatingDocument() {
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
//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestFilteringResultSearch() {
    SearchServer searchServer;
    searchServer.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {-1, -2, -3});
    searchServer.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {-1});
    
    {
        auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s,
                [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL(documents.size(), 2);
        ASSERT_EQUAL(documents[0].id, 2);
        ASSERT_EQUAL(documents[1].id, 0);
    }
    {
        auto documents = searchServer.FindTopDocuments("пушистый ухоженный кот"s,
                [](int document_id, DocumentStatus status, int rating) { return rating > 0; });
        ASSERT_EQUAL(documents.size(), 2);
        ASSERT(documents[0].rating > 0);
        ASSERT(documents[1].rating > 0);
    }
    
}
//Поиск документов, имеющих заданный статус.
void TestSearchDocumentByStatus() {
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
//Корректное вычисление релевантности найденных документов.
void TestComputeRelevanceFoundDocument() {
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
// Функция TestSearchServer является точкой входа для запуска тестов
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
}
// --------- Окончание модульных тестов поисковой системы -----------
int main()
{
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}

