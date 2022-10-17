#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>


using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

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
            _stopWords.insert(word);
        }
    }
    
    void addDocument(int documentId, const string& document, DocumentStatus status, const vector<int>& ratings)
    {
        const vector<string> words = splitIntoWordsNoStop(document);
        const double invWordCount = 1.0 / words.size();
        for (const string& word : words) {
            _wordToDocumentFreqs[word][documentId] += invWordCount;
        }
        _documents.emplace(documentId, DocumentData{computeAverageRating(ratings), status});
    }
    
    vector<Document> findTopDocuments(const string& raw_query) const
    {
        return findTopDocuments(raw_query, [](int documentId, DocumentStatus status, int rating) {
            return status == DocumentStatus::ACTUAL;
        });
    }
    
    template<typename Predicate>
    vector<Document> findTopDocuments(const string& rawQuery, Predicate predicate) const
    {
        vector<Document> matchedDocuments;
        const Query query = parseQuery(rawQuery);
    
        if constexpr (is_same_v<Predicate, DocumentStatus>) {
            function<bool(int, DocumentStatus, int )> resultPredicate;
    
            if (predicate == DocumentStatus::ACTUAL) {
                resultPredicate = [](int documentId,
                                     DocumentStatus status,
                                     int rating) { return status == DocumentStatus::ACTUAL; };
            }
            else if (predicate == DocumentStatus::IRRELEVANT) {
                resultPredicate = [](int documentId,
                                     DocumentStatus status,
                                     int rating) { return status == DocumentStatus::IRRELEVANT; };
            }
            else if (predicate == DocumentStatus::BANNED) {
                resultPredicate = [](int documentId,
                                     DocumentStatus status,
                                     int rating) { return status == DocumentStatus::BANNED; };
            }
            else if (predicate == DocumentStatus::REMOVED) {
                resultPredicate = [](int documentId,
                                     DocumentStatus status,
                                     int rating) { return status == DocumentStatus::REMOVED; };
            }
            matchedDocuments = FindAllDocuments(query, resultPredicate);
        }
        else{
            matchedDocuments = FindAllDocuments(query, predicate);
        }
    
        sort(matchedDocuments.begin(), matchedDocuments.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
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
        return _documents.size();
    }
    
    tuple<vector<string>, DocumentStatus> matchDocument(const string& rawQuery, int documentId) const
    {
        const Query query = parseQuery(rawQuery);
        vector<string> matchedWords;
        for (const string& word : query.plusWords) {
            if (_wordToDocumentFreqs.count(word) == 0) {
                continue;
            }
            if (_wordToDocumentFreqs.at(word).count(documentId)) {
                matchedWords.push_back(word);
            }
        }
        for (const string& word : query.minusWords) {
            if (_wordToDocumentFreqs.count(word) == 0) {
                continue;
            }
            if (_wordToDocumentFreqs.at(word).count(documentId)) {
                matchedWords.clear();
                break;
            }
        }
        return {matchedWords, _documents.at(documentId).status};
    }

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };
    
    set<string> _stopWords;
    map<string, map<int, double>> _wordToDocumentFreqs;
    map<int, DocumentData> _documents;
    
    bool isStopWord(const string& word) const
    {
        return _stopWords.count(word) > 0;
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
        int ratingSum = 0;
        for (const int rating : ratings) {
            ratingSum += rating;
        }
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
    double ComputeWordInverseDocumentFreq(const string& word) const
    {
        return log(getDocumentCount() * 1.0 / _wordToDocumentFreqs.at(word).size());
    }
    
    template<typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const
    {
        map<int, double> documentToRelevance;
        for (const string& word : query.plusWords) {
            if (_wordToDocumentFreqs.count(word) == 0) {
                continue;
            }
            const double inverseDocumentFreq = ComputeWordInverseDocumentFreq(word);
            for (const auto [documentId, termFreq] : _wordToDocumentFreqs.at(word)) {
                auto filterPredicate = predicate(documentId, _documents.at(documentId).status,
                        _documents.at(documentId).rating);
                if (filterPredicate) {
                    documentToRelevance[documentId] += termFreq * inverseDocumentFreq;
                }
            }
        }
        
        for (const string& word : query.minusWords) {
            if (_wordToDocumentFreqs.count(word) == 0) {
                continue;
            }
            for (const auto [documentId, _] : _wordToDocumentFreqs.at(word)) {
                documentToRelevance.erase(documentId);
            }
        }
        
        vector<Document> matchedDocuments;
        for (const auto [documentId, relevance] : documentToRelevance) {
            matchedDocuments.push_back({documentId, relevance, _documents.at(documentId).rating});
        }
        return matchedDocuments;
    }
};

// ==================== для примера =========================
void PrintDocument(const Document& document)
{
    cout << "{ "s << "document_id = "s << document.id << ", "s << "relevance = "s << document.relevance << ", "s
            << "rating = "s << document.rating << " }"s << endl;
}

int main() {
    SearchServer searchServer;
    searchServer.setStopWords("и в на"s);
    searchServer.addDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    searchServer.addDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    searchServer.addDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    searchServer.addDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : searchServer.findTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : searchServer.findTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : searchServer.findTopDocuments("пушистый ухоженный кот"s,
            [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}