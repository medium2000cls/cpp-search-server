#include <execution>
#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries)
{
    std::vector<std::vector<Document>> results(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), results.begin(),
            [&search_server](const std::string& str) -> std::vector<Document> {
                return search_server.FindTopDocuments(str);
            });
    return results;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
    std::vector<Document> result;
    for (std::vector<Document> documents : ProcessQueries(search_server, queries)) {
        result.insert(result.end(), documents.begin(), documents.end());
    }
    return result;
}
