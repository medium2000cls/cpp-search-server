#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text) {
    vector<string> words;
    string word;
    for (const char c: text) {
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
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string &text) {
        for (const string &word: SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string &document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        map<string, int> words_count;
        int words_size = words.size();
        for (int i = 0; i < words_size; ++i) {
            words_count[words[i]]++;
        }
        for (auto [word, count]: words_count) {
            idDocumentsInWord_[word][document_id] = (1.0 * count / words_size);
        }
        document_count_++;
    }

    vector<Document> FindTopDocuments(const string &raw_query) const {
        const set<string> query_words = ParseQuery(raw_query);
        const set<string> minus_query_words = ParseMinusQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words, minus_query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    int document_count_ = 0;
    map<string, map<int, double>> idDocumentsInWord_;
    set<string> stop_words_;


    bool IsStopWord(const string &word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string &text) const {
        vector<string> words;
        for (const string &word: SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    set<string> ParseQuery(const string &text) const {
        set<string> query_words;
        for (const string &word: SplitIntoWordsNoStop(text)) {
            if (word[0] != '-') {
                query_words.insert(word);
            }
        }
        return query_words;
    }

    const set<string> ParseMinusQuery(const string &text) const {
        set<string> minus_query_words;
        for (const string &word: SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
                string wordWithoutMinus = word;
                wordWithoutMinus.erase(0, 1);
                minus_query_words.insert(wordWithoutMinus);
            }
        }
        return minus_query_words;
    }

    vector<Document> FindAllDocuments(const set<string> &query_words, const set<string> &minus_query_words) const {
        vector<Document> matched_documents;
        map<int, double> relevance;
        for (auto word: query_words) {
            if (idDocumentsInWord_.count(word) != 0) {
                double word_doc_count = idDocumentsInWord_.at(word).size();
                double idf = log(document_count_ / word_doc_count);
                for (auto [doc_id, tf]: idDocumentsInWord_.at(word)) {
                    relevance[doc_id] += (idf * tf);
                }
            }
        }
        for (auto word: minus_query_words) {
            if (idDocumentsInWord_.count(word) != 0) {
                for (auto [doc_id, tf]: idDocumentsInWord_.at(word)) {
                    if (relevance.count(doc_id) != 0) {
                        relevance.erase(doc_id);
                    }
                }
            }
        }
        for (const auto &[document_id, relevance]: relevance) {
            matched_documents.push_back({document_id, relevance});
        }
        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto &[document_id, relevance]: search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}