#include "read_input_functions.h"
#include <iostream>
#include <iomanip>
#include <string>

std::string ReadLine()
{
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}

void PrintDocument(const Document& document)
{
    using namespace std;
    std::cout << std::setprecision(12) << "{ "s << "document_id = "s << document.id << ", "s << "relevance = "s
            << document.relevance << ", "s << "rating = "s << document.rating << " }"s << std::endl;
}
