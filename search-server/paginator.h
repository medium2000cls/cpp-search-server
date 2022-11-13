#pragma once

#include <iostream>
#include <vector>

template<typename T_it>
struct IteratorRange {
    const T_it begin_it;
    const T_it end_it;
};

template <typename T_it>
std::ostream& operator<<(std::ostream& out, const IteratorRange<T_it>& range);

template<typename T_it>
class Paginator
{
public:
    Paginator(T_it begin_it, T_it end_it, int page_size);
    auto begin() const;
    auto end() const;
    
private:
    
    int page_size_;
    std::vector<IteratorRange<T_it>> iterators_;
    
};

template<typename T_it>
std::ostream& operator<<(std::ostream& out, const IteratorRange<T_it>& range)
{
    for (T_it it = range.begin_it; it != range.end_it; ++it) {
        out << *it;
    }
    return out;
}

template<typename T_it>
Paginator<T_it>::Paginator(T_it begin_it, T_it end_it, int page_size) : page_size_(page_size)
{
    int container_size = count_if(begin_it, end_it, [](auto) { return true; });
    if (container_size <= page_size_) {
        iterators_.push_back(IteratorRange<T_it>{begin_it, end_it});
    }
    else if (page_size_ > 0) {
        int i = page_size_;
        while (i < container_size) {
            T_it it_start = begin_it + i - page_size_;
            T_it it_end = begin_it + i;
            IteratorRange<T_it> iterator_range{it_start, it_end};
            iterators_.push_back(iterator_range);
            i += page_size_;
        }
        if (i > container_size - 1) {
            i -= page_size_;
            T_it it_start = begin_it + i;
            T_it it_end = end_it;
            IteratorRange<T_it> iterator_range{it_start, it_end};
            iterators_.push_back(iterator_range);
        }
    }
    else {
        throw std::invalid_argument("Page size must be greater than zero.");
    }
}

template<typename T_it>
auto Paginator<T_it>::begin() const {
    return iterators_.begin();
}

template<typename T_it>
auto Paginator<T_it>::end() const {
    return iterators_.end();
}

template<typename Container>
auto Paginate(const Container& c, size_t page_size)
{
    return Paginator(begin(c), end(c), page_size);
}