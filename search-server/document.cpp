#include "document.h"

std::ostream& operator<<(std::ostream& os, const Document& document)
{
    using namespace std;
    os << "{ document_id = "s << document.id << ", relevance = "s << document.relevance << ", rating = "s
            << document.rating << " }"s;
    return os;
}
