#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ACCURACY_COMPARISON = 1e-6;

string readLine()
{
    string s;
    getline(cin, s);
    return s;
}

int readLineWithNumber()
{
    int result;
    cin >> result;
    readLine();
    return result;
}

vector<string> splitIntoWords(const string& text)
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
    void setStopWords(const string& text)
    {
        for (const string& word : splitIntoWords(text)) {
            stopWords_.insert(word);
        }
    }
    
    void addDocument(int documentId, const string& document, DocumentStatus status, const vector<int>& ratings)
    {
        const vector<string> words = splitIntoWordsNoStop(document);
        const double invWordCount = 1.0 / words.size();
        
        for (const string& word : words) {
            wordToDocumentFreqs_[word][documentId] += invWordCount;
        }
        documents_.emplace(documentId, DocumentData{computeAverageRating(ratings), status});
    }
    
    vector<Document> findTopDocuments(const string& raw_query) const
    {
        return findTopDocuments(raw_query, [](int documentId, DocumentStatus status, int rating) {
            return status == DocumentStatus::ACTUAL;
        });
    }
    
    vector<Document> findTopDocuments(const string& raw_query, const DocumentStatus& documentStatus) const
    {
        return findTopDocuments(raw_query, [&documentStatus](int documentId,
                                                             DocumentStatus status,
                                                             int rating) { return status == documentStatus; });
    }
    
    template<typename Predicate>
    vector<Document> findTopDocuments(const string& rawQuery, const Predicate& predicate) const
    {
        const Query query = parseQuery(rawQuery);
        vector<Document> matchedDocuments = findAllDocuments(query, predicate);
        // Принято, сделал перегрузку метода, он теперь принимает статус. Разобрался с constexpr, это спецификатор, который
        // говорит о том, что расчет будет произведен во время компиляции, а если этого сделать нельзя, то блок будет
        // обрабатываться уже во время выполнения программы.
        // В перегрузке, статус захватываю в лямбду.
        sort(matchedDocuments.begin(), matchedDocuments.end(), [](const Document& lhs, const Document& rhs) {
            // Вынес константу в отдельную глобальную константу.
            if (abs(lhs.relevance - rhs.relevance) < ACCURACY_COMPARISON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matchedDocuments.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matchedDocuments.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matchedDocuments;
    }
    
    int getDocumentCount() const
    {
        return documents_.size();
    }
    
    tuple<vector<string>, DocumentStatus> matchDocument(const string& rawQuery, int documentId) const
    {
        const Query query = parseQuery(rawQuery);
        vector<string> matchedWords;
        for (const string& word : query.plusWords) {
            if (wordToDocumentFreqs_.count(word) == 0) {
                continue;
            }
            if (wordToDocumentFreqs_.at(word).count(documentId)) {
                matchedWords.push_back(word);
            }
        }
        for (const string& word : query.minusWords) {
            if (wordToDocumentFreqs_.count(word) == 0) {
                continue;
            }
            if (wordToDocumentFreqs_.at(word).count(documentId)) {
                matchedWords.clear();
                break;
            }
        }
        return {matchedWords, documents_.at(documentId).status};
    }

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };
    
    set<string> stopWords_;
    map<string, map<int, double>> wordToDocumentFreqs_;
    //Принято, буду руководствоваться этим правилом.
    map<int, DocumentData> documents_;
    
    bool isStopWord(const string& word) const
    {
        return stopWords_.count(word) > 0;
    }
    
    vector<string> splitIntoWordsNoStop(const string& text) const
    {
        vector<string> words;
        for (const string& word : splitIntoWords(text)) {
            if (!isStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
    
    static int computeAverageRating(const vector<int>& ratings)
    {
        if (ratings.empty()) {
            return 0;
        }
        //Принято, переменную оставил для читаемости.
        int ratingSum = accumulate(ratings.begin(), ratings.end(), 0);
        
        return ratingSum / static_cast<int>(ratings.size());
    }
    
    struct QueryWord
    {
        string data;
        bool isMinus;
        bool isStop;
    };
    
    QueryWord parseQueryWord(string text) const
    {
        bool isMinus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            isMinus = true;
            text = text.substr(1);
        }
        return {text, isMinus, isStopWord(text)};
    }
    
    struct Query
    {
        set<string> plusWords;
        set<string> minusWords;
    };
    
    Query parseQuery(const string& text) const
    {
        Query query;
        for (const string& word : splitIntoWords(text)) {
            const QueryWord queryWord = parseQueryWord(word);
            if (!queryWord.isStop) {
                if (queryWord.isMinus) {
                    query.minusWords.insert(queryWord.data);
                }
                else {
                    query.plusWords.insert(queryWord.data);
                }
            }
        }
        return query;
    }
    
    // Existence required
    double computeWordInverseDocumentFreq(const string& word) const
    {
        return log(getDocumentCount() * 1.0 / wordToDocumentFreqs_.at(word).size());
    }
    
    //Исправил стиль, все имена методов и функций с маленькой буквы.
    template<typename Predicate>
    vector<Document> findAllDocuments(const Query& query, Predicate& predicate) const
    {
        map<int, double> documentToRelevance;
        for (const string& word : query.plusWords) {
            if (wordToDocumentFreqs_.count(word) == 0) {
                continue;
            }
            const double inverseDocumentFreq = computeWordInverseDocumentFreq(word);
            for (const auto [documentId, termFreq] : wordToDocumentFreqs_.at(word)) {
                //Записал результат в переменную
                auto document = documents_.at(documentId);
                bool filterPredicate = predicate(documentId, document.status, document.rating);
                if (filterPredicate) {
                    documentToRelevance[documentId] += termFreq * inverseDocumentFreq;
                }
            }
        }
        
        for (const string& word : query.minusWords) {
            if (wordToDocumentFreqs_.count(word) == 0) {
                continue;
            }
            for (const auto [documentId, _] : wordToDocumentFreqs_.at(word)) {
                documentToRelevance.erase(documentId);
            }
        }
        
        vector<Document> matchedDocuments;
        for (const auto [documentId, relevance] : documentToRelevance) {
            matchedDocuments.push_back({documentId, relevance, documents_.at(documentId).rating});
        }
        return matchedDocuments;
    }
};

// ==================== для примера =========================
void printDocument(const Document& document)
{
    cout << "{ "s << "document_id = "s << document.id << ", "s << "relevance = "s << document.relevance << ", "s
            << "rating = "s << document.rating << " }"s << endl;
}

int main()
{
    SearchServer searchServer;
    searchServer.setStopWords("и в на"s);
    searchServer.addDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    searchServer.addDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    searchServer.addDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    searchServer.addDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : searchServer.findTopDocuments("пушистый ухоженный кот"s)) {
        printDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : searchServer.findTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        printDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : searchServer.findTopDocuments("пушистый ухоженный кот"s,
            [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        printDocument(document);
    }
    return 0;
}