#include <iostream>
#include <string>
#include "document.h"
#include "search_server.h"
#include "test_example_functions.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "paginator.h"
#include "remove_duplicates.h"

using namespace std;


int main()
{
    using namespace std;
    TestSearchServer();
    // ���� �� ������ ��� ������, ������ ��� ����� ������ �������
    cout << "Search server testing finished"s << endl;
    SearchServer search_server("and with"s);
    
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    
    // �������� ��������� 2, ����� �����
    search_server.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    
    // ������� ������ � ����-������, ������� ����������
    search_server.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    
    // ��������� ���� ����� ��, ������� ���������� ��������� 1
    search_server.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    
    // ���������� ����� �����, ���������� �� ��������
    search_server.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    
    // ��������� ���� ����� ��, ��� � id 6, �������� �� ������ �������, ������� ����������
    search_server.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});
    
    // ���� �� ��� �����, �� �������� ����������
    search_server.AddDocument(8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});
    
    // ����� �� ������ ����������, �� �������� ����������
    search_server.AddDocument(9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    
    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
    RemoveDuplicates(search_server);
    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;
    return 0;
}
