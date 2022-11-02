#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <tuple>
#include <optional>
#include <iomanip>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ACCURACY_COMPARISON = 1e-6;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text)
{
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            words.push_back(word);
            word.clear();
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
    Document() = default;
    
    Document(int id, double relevance, int rating) : id(id), relevance(relevance), rating(rating)
    {
    }
    
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template<typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings)
{
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus
{
    ACTUAL, IRRELEVANT, BANNED, REMOVED,
};

class SearchServer
{
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    
    explicit SearchServer() {}
    
    template<typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw invalid_argument("Stop-word contains invalid characters.");
        }
    }
    
    explicit SearchServer(const string& stop_words_text) : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw invalid_argument("Stop-word contains invalid characters.");
        }
    }
    
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings)
    {
        if (document_id < 0) { throw invalid_argument("Document ID cannot be less than zero"); }
        if (documents_.count(document_id) != 0) { throw invalid_argument("Document ID cannot be repeated"); }
        
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            if (!IsValidWord(word)) { throw invalid_argument("Word in document contains invalid characters."); }
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        order_documents_id_.push_back(document_id);
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }
    
    template<typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const
    {
        auto query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);
        
        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
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
    
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const
    {
        return FindTopDocuments(raw_query, [status]([[maybe_unused]] int document_id,
                                                    DocumentStatus document_status,
                                                    [[maybe_unused]] int rating) {
            return document_status == status;
        });
    }
    
    vector<Document> FindTopDocuments(const string& raw_query) const
    {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }
    
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const
    {
        auto query = ParseQuery(raw_query);
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
        return tie(matched_words, documents_.at(document_id).status);
    }
    
    int GetDocumentCount() const
    {
        return documents_.size();
    }
    
    int GetDocumentId(int index) const
    {
        int order_documents_id_size = order_documents_id_.size();
        if ((index < 0) || (index > order_documents_id_size - 1)) {
            throw out_of_range("Document index out of the range.");
        }
        return order_documents_id_.at(index);
    }

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> order_documents_id_;
    
    static bool IsValidWord(const string& word)
    {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }
    
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
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }
    
    struct QueryWord
    {
        string data;
        bool is_minus;
        bool is_stop;
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
            if (word.empty()) {
                continue;
            }
            const QueryWord query_word = ParseQueryWord(word);
            
            if (query_word.data[0] == '-') { throw invalid_argument("Query word contains \"minus\" character."); }
            if (query_word.is_minus && query_word.data.empty()) {
                throw invalid_argument("Minus query word is empty.");
            }
            if (!IsValidWord(query_word.data)) { throw invalid_argument("Query word contains invalid characters."); }
            
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
    
    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const
    {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    
    template<typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const
    {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
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

#pragma region ��������� �����

// -------- ������ ��������� ������ ��������� ������� ----------
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

template<typename K, typename T>
ostream& operator<<(ostream& out, const map<K, T>& container)
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
void AssertEqualImpl(const T& t,
                     const U& u,
                     const string& t_str,
                     const string& u_str,
                     const string& file,
                     const string& func,
                     unsigned line,
                     const string& hint)
{
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

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value,
                const string& expr_str,
                const string& file,
                const string& func,
                unsigned line,
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

template<typename T>
void RunTest(T& func, const string& func_name)
{
    func();
    cerr << func_name << " OK" << endl;
}

#define RUN_TEST(func) RunTest(func, #func)

//���������� ����������. ����������� �������� ������ ���������� �� ���������� �������, ������� �������� ����� �� ���������.
void TestAddDocumentMustBeFoundFromQuery()
{
    SearchServer searchServer;
    searchServer.AddDocument(0, "����� ��� � ������ �������"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {1, 2, 3});
    searchServer.AddDocument(2, "��������� �� ������������� �����"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "��������� ������� �������"s, DocumentStatus::ACTUAL, {1});
    
    {
        auto documents = searchServer.FindTopDocuments("�����"s);
        ASSERT_EQUAL(documents.size(), 1);
        ASSERT_EQUAL(documents[0].id, 0);
    }
    {
        auto documents = searchServer.FindTopDocuments("�������"s);
        ASSERT_EQUAL(documents.size(), 1);
        ASSERT_EQUAL(documents[0].id, 3);
    }
}

//��������� ����-����. ����-����� ����������� �� ������ ����������.
void TestSupportStopWordsTheyExcludedDocumentText()
{
    
    {
        SearchServer searchServer;
        searchServer.AddDocument(0, "����� ��� � ������ �������"s, DocumentStatus::ACTUAL, {1, 2});
        searchServer.AddDocument(1, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {1, 2, 3});
        searchServer.AddDocument(2, "��������� �� ������������� �����"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
        searchServer.AddDocument(3, "��������� ������� �������"s, DocumentStatus::ACTUAL, {1});
        auto documents = searchServer.FindTopDocuments("�"s);
        ASSERT_EQUAL(documents.size(), 1);
        ASSERT_EQUAL(documents[0].id, 0);
    }
    {
        SearchServer searchServer("� � ��"s);
        searchServer.AddDocument(0, "����� ��� � ������ �������"s, DocumentStatus::ACTUAL, {1, 2});
        searchServer.AddDocument(1, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {1, 2, 3});
        searchServer.AddDocument(2, "��������� �� ������������� �����"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
        searchServer.AddDocument(3, "��������� ������� �������"s, DocumentStatus::ACTUAL, {1});
        auto documents = searchServer.FindTopDocuments("�"s);
        ASSERT_EQUAL(documents.size(), 0);
    }
    
}

//��������� �����-����. ���������, ���������� �����-����� ���������� �������, �� ������ ���������� � ���������� ������.
void TestSupportMinusWordsTheyExcludedDocumentFromResult()
{
    SearchServer searchServer;
    searchServer.AddDocument(0, "����� ��� � ������ �������"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {1, 2, 3});
    searchServer.AddDocument(2, "��������� �� ������������� �����"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "��������� ������� �������"s, DocumentStatus::ACTUAL, {1});
    
    {
        auto documents = searchServer.FindTopDocuments("�������� ��������� ���"s);
        ASSERT_EQUAL(documents.size(), 4);
    }
    {
        auto documents = searchServer.FindTopDocuments("�������� -��������� ���"s);
        ASSERT_EQUAL(documents.size(), 2);
    }
}

//������� ����������. ��� �������� ��������� �� ���������� ������� ������ ���� ���������� ��� ����� �� ���������� �������,
// �������������� � ���������. ���� ���� ������������ ���� �� �� ������ �����-�����, ������ ������������ ������ ������ ����.
void TestMatchDocumentCheckReturnWords()
{
    SearchServer searchServer;
    searchServer.AddDocument(0, "����� ��� � ������ �������"s, DocumentStatus::ACTUAL, {1, 2});
    set<string> test_word = {"�����", "���"};
    {
        tuple<vector<string>, DocumentStatus> match_document = searchServer.MatchDocument("����� ��� �������� �����"s,
                0);
        const auto& [words, _] = match_document;
        ASSERT_EQUAL(words.size(), 2);
        for (const auto& word : words) {
            ASSERT(test_word.count(word) != 0);
        }
    }
    {
        tuple<vector<string>, DocumentStatus> match_document = searchServer.MatchDocument("����� -��� �������� �����"s,
                0);
        const auto& [words, _] = match_document;
        ASSERT_EQUAL(words.size(), 0);
    }
}

//���������� ��������� ���������� �� �������������. ������������ ��� ������ ���������� ���������� ������ ���� ������������� � ������� �������� �������������.
void TestSortDocumentByRelevance()
{
    SearchServer searchServer;
    searchServer.AddDocument(0, "����� ��� � ������ �������"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {1, 2, 3});
    searchServer.AddDocument(2, "��������� �� ������������� �����"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "��������� ������� �������"s, DocumentStatus::ACTUAL, {1});
    
    auto documents = searchServer.FindTopDocuments("�������� ��������� ���"s);
    ASSERT_EQUAL(documents.size(), 4);
    for (int i = 1; i < static_cast<int>(documents.size()); ++i) {
        ASSERT(documents[i - 1].relevance >= documents[i].relevance);
    }
}

//���������� �������� ����������. ������� ������������ ��������� ����� �������� ���������������, ������ ���������.
void TestComputeRatingDocument()
{
    SearchServer searchServer;
    searchServer.AddDocument(0, "�����"s, DocumentStatus::ACTUAL, {1}); //1
    searchServer.AddDocument(1, "��������"s, DocumentStatus::ACTUAL, {-1, -2}); //1
    searchServer.AddDocument(2, "��"s, DocumentStatus::ACTUAL, {1, 2, 3}); //2
    searchServer.AddDocument(3, "�������"s, DocumentStatus::ACTUAL, {-1, -2, -3, -4}); //2
    
    {
        auto documents = searchServer.FindTopDocuments("�����"s);
        ASSERT_EQUAL(documents[0].rating, 1);
    }
    {
        auto documents = searchServer.FindTopDocuments("��������"s);
        ASSERT_EQUAL(documents[0].rating, -1);
    }
    {
        auto documents = searchServer.FindTopDocuments("��"s);
        ASSERT_EQUAL(documents[0].rating, 2);
    }
    {
        auto documents = searchServer.FindTopDocuments("�������"s);
        ASSERT_EQUAL(documents[0].rating, -2);
    }
}

//���������� ����������� ������ � �������������� ���������, ����������� �������������.
void TestFilteringResultSearch()
{
    SearchServer searchServer;
    searchServer.AddDocument(0, "����� ��� � ������ �������"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {-1, -2, -3});
    searchServer.AddDocument(2, "��������� �� ������������� �����"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "��������� ������� �������"s, DocumentStatus::BANNED, {-1});
    
    {
        auto documents = searchServer.FindTopDocuments("�������� ��������� ���"s, [](int document_id,
                                                                                     [[maybe_unused]] DocumentStatus status,
                                                                                     [[maybe_unused]] int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL(documents.size(), 2);
        ASSERT_EQUAL(documents[0].id, 2);
        ASSERT_EQUAL(documents[1].id, 0);
    }
    {
        auto documents = searchServer.FindTopDocuments("�������� ��������� ���"s, []([[maybe_unused]] int document_id,
                                                                                     [[maybe_unused]] DocumentStatus status,
                                                                                     int rating) { return rating > 0; });
        ASSERT_EQUAL(documents.size(), 2);
        ASSERT(documents[0].rating > 0);
        ASSERT(documents[1].rating > 0);
    }
    
}

//����� ����������, ������� �������� ������.
void TestSearchDocumentByStatus()
{
    SearchServer searchServer;
    searchServer.AddDocument(0, "����� ��� � ������ �������"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "�������� ��� �������� �����"s, DocumentStatus::IRRELEVANT, {-1, -2, -3});
    searchServer.AddDocument(2, "��������� �� ������������� �����"s, DocumentStatus::REMOVED, {1, 2, 3, 4});
    searchServer.AddDocument(3, "��������� ������� �������"s, DocumentStatus::BANNED, {-1});
    
    {
        auto documents = searchServer.FindTopDocuments("�������� ��������� ���"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(documents[0].id, 0);
    }
    {
        auto documents = searchServer.FindTopDocuments("�������� ��������� ���"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(documents[0].id, 1);
    }
    {
        auto documents = searchServer.FindTopDocuments("�������� ��������� ���"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(documents[0].id, 2);
    }
    {
        auto documents = searchServer.FindTopDocuments("�������� ��������� ���"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(documents[0].id, 3);
    }
    
}

//���������� ���������� ������������� ��������� ����������.
void TestComputeRelevanceFoundDocument()
{
    SearchServer searchServer;
    searchServer.AddDocument(0, "����� ��� ������ �������"s, DocumentStatus::ACTUAL, {1, 2});
    searchServer.AddDocument(1, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {-1, -2, -3});
    searchServer.AddDocument(2, "��������� �� ������������� �����"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
    searchServer.AddDocument(3, "��������� ������� �������"s, DocumentStatus::ACTUAL, {-1});
    {
        auto documents = searchServer.FindTopDocuments("����� ��������� ���"s);
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

// ������� TestSearchServer �������� ������ ����� ��� ������� ������
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
// --------- ��������� ��������� ������ ��������� ������� -----------
#pragma endregion

void PrintDocument(const Document& document)
{
    cout << setprecision(12) << "{ "s << "document_id = "s << document.id << ", "s << "relevance = "s
            << document.relevance << ", "s << "rating = "s << document.rating << " }"s << endl;
}

int main()
{
    TestSearchServer();
    // ���� �� ������ ��� ������, ������ ��� ����� ������ �������
    cout << "Search server testing finished"s << endl;
    try {
        SearchServer search_server("� � ��"s);
        // ���� ���������� ��������� ������ AddDocument, ����� �������� ��������������
        // � �������������� ���������� ��� ������
        search_server.AddDocument(1, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(2, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(3, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(4, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(5, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(6, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {7, 2, 7});
        const auto documents = search_server.FindTopDocuments("��� -"s);
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    catch (const invalid_argument& e) {
        cout << "Exception: " << e.what() << endl;
    }
    catch (const out_of_range& e) {
        cout << "Exception: " << e.what() << endl;
    }
    
}

