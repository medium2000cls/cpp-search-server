#pragma once

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <tuple>
#include "document.h"
#include "search_server.h"


// -------- Начало модульных тестов поисковой системы ----------
template<typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& container);
template<typename T>
std::ostream& operator<<(std::ostream& out, const std::set<T>& container);
template<typename K, typename T>
std::ostream& operator<<(std::ostream& out, const std::map<K, T>& container);
template<typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned line, const std::string& hint);

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
                const std::string& hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template<typename T>
void RunTest(T& func, const std::string& func_name);

#define RUN_TEST(func) RunTest(func, #func)

template<typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& container)
{
    using namespace std;
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
std::ostream& operator<<(std::ostream& out, const std::set<T>& container)
{
    using namespace std;
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

template<typename K, typename T>
std::ostream& operator<<(std::ostream& out, const std::map<K, T>& container)
{
    using namespace std;
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
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned int line, const std::string& hint)
{
    using namespace std;
    
    if (static_cast<int>(t) != u) {
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

//Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestAddDocumentMustBeFoundFromQuery();
//Поиск документа
void TestFindTopDocument();
//Поддержка стоп-слов. Стоп-слова исключаются из текста документов.
void TestSupportStopWordsTheyExcludedDocumentText();
//Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void TestSupportMinusWordsTheyExcludedDocumentFromResult();
//Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса,
// присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestMatchDocumentCheckReturnWords();
//Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortDocumentByRelevance();
//Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому, оценок документа.
void TestComputeRatingDocument();
//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestFilteringResultSearch();
////Поиск документов, имеющих заданный статус.
void TestSearchDocumentByStatus();
//Корректное вычисление релевантности найденных документов.
void TestComputeRelevanceFoundDocument();
//Корректное разделение на страницы.
void TestCorrectPaginationFoundDocument();

//Корректный возврат слова и частоты слов по ID.
void TestGetWordFrequencies();
//Корректное удаление документа по ID.
void TestRemoveDocument();

void TestProcessQueries();

void TestIteratorTree();

void TestProcessQueriesJoined();
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();
// --------- Окончание модульных тестов поисковой системы -----------
template<typename T>
void RunTest(T& func, const std::string& func_name)
{
    using namespace std;
    
    func();
    cerr << func_name << " OK" << endl;
}
