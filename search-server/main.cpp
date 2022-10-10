#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string readLine() {
    string s;
    getline(cin, s);
    return s;
}


int readLineWithNumber() {
    int result = 0;
    cin >> result;
    readLine();
    return result;
}


vector<string> splitIntoWords(const string &text) {
    vector<string> words;
    string         word;

    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}


struct Document {
    int    id;
    double relevance;
};


class SearchServer {
public:
    void setStopWords(const string &text) {
        const vector<string> &intoWords = splitIntoWords(text);
        for (const string    &word : intoWords) {
            _stopWords.insert(word);
        }
    }


    void addDocument(int documentId, const string &document) {
        const vector<string> words     = splitIntoWordsNoStop(document);
        int                  wordsSize = words.size();
// Принято, я убрал лишнюю переменную, но не уверен, что правильно понял, как это сделать.
// Так как получается, что я храню промежуточные результаты вычислений в члене типа.

        for (int i = 0; i < wordsSize; ++i) {
            auto &idRelevanceDoc = _idDocumentsInWord[words[i]];
            idRelevanceDoc[documentId] += 1.0;
        }

        for (auto &[word, idRelevanceDoc] : _idDocumentsInWord) {
            if (idRelevanceDoc.count(documentId) != 0) {
                idRelevanceDoc[documentId] /= wordsSize;
            }
        }
        _documentCount++;
    }


    vector<Document> findTopDocuments(const string &rawQuery) const {
        const vector<string> &intoWordsNoStop = splitIntoWordsNoStop(rawQuery);
        const set<string>    &queryWords      = parseQuery(intoWordsNoStop);
        const set<string>    &minusQueryWords = parseMinusQuery(intoWordsNoStop);
        vector<Document>     matchedDocuments = findAllDocuments(queryWords, minusQueryWords);

        sort(matchedDocuments.begin(), matchedDocuments.end(),
             [](const Document &lhs, const Document &rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matchedDocuments.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matchedDocuments.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matchedDocuments;
    }

private:
// Все переменные привел к одному стилю, кроме того, всю программу привел в соответствии с рекомендациями
// по стилю написания кода на С++
// Отошел в рекомендации, при именовании приватных членов типа, если так не принято, то исправлю.
// https://habr.com/ru/post/172091/
    map<string, map<int, double>> _idDocumentsInWord;
    int                           _documentCount = 0;
    set<string>                   _stopWords;


    bool isStopWord(const string &word) const {
        return _stopWords.count(word) > 0;
    }


    vector<string> splitIntoWordsNoStop(const string &text) const {
        vector<string> words;

        const vector<string> &intoWords = splitIntoWords(text);
        for (const string    &word : intoWords) {
            if (!isStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

// Перенес контейнер intoWordsNoStop в метод findTopDocuments,
// что бы не вызывать метод splitIntoWordsNoStop два раза
    set<string> parseQuery(const vector<string> &intoWordsNoStop) const {
        set<string> queryWords;

        for (const string &word : intoWordsNoStop) {
            if (word[0] != '-') {
                queryWords.insert(word);
            }
        }
        return queryWords;
    }


    set<string> parseMinusQuery(const vector<string> &intoWordsNoStop) const {
        set<string> minusQueryWords;

        for (string word : intoWordsNoStop) {
            if (word[0] == '-') {
                //Убрал лишнюю переменную
                word.erase(0, 1); //Удаляю минус перед словом
                //"Можно сразу отбросить стоп слова, их нет в индексе." это замечание не понял
                minusQueryWords.insert(word);
            }
        }
        return minusQueryWords;
    }


    vector<Document> findAllDocuments(const set<string> &queryWords, const set<string> &minusQueryWords) const {
        vector<Document> matchedDocuments;
        map<int, double> docIdRelevance;

        for (auto word : queryWords) {
            if (_idDocumentsInWord.count(word) != 0) {
                double wordDocCount = _idDocumentsInWord.at(word).size();
                double idf          = log(_documentCount / wordDocCount);
                for (auto [docId, tf] : _idDocumentsInWord.at(word)) {
                    docIdRelevance[docId] += (idf * tf);
                }
            }
        }

        for (auto word : minusQueryWords) {
            if (_idDocumentsInWord.count(word) != 0) {
                for (auto [docId, tf] : _idDocumentsInWord.at(word)) {
                    if (docIdRelevance.count(docId) != 0) {
                        docIdRelevance.erase(docId);
                    }
                }
            }
        }

        for (const auto &[documentId, relevance] : docIdRelevance) {
            matchedDocuments.push_back({documentId, relevance});
        }

        return matchedDocuments;
    }
};


SearchServer createSearchServer() {
    SearchServer searchServer;
    searchServer.setStopWords(readLine());

    const int documentCount = readLineWithNumber();
    for (int  documentId    = 0; documentId < documentCount; ++documentId) {
        searchServer.addDocument(documentId, readLine());
    }

    return searchServer;
}


int main() {
    const SearchServer searchServer = createSearchServer();

    const string query = readLine();
    for (const auto &[documentId, relevance] : searchServer.findTopDocuments(query)) {
        cout << "{ documentId = "s << documentId << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}