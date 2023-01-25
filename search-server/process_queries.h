#pragma once

#include "search_server.h"
#include <list>
#include <any>
#include <cassert>

template<typename Container, typename Type>
class ListFromVecInDegree
{
    Container data_;
    
    template<typename TreeContainer, typename LeafType>
    class TreeIterator
    {
        friend class ListFromVecInDegree;
    
        using Iterator = typename TreeContainer::iterator;
    
        template<typename Iter, bool = std::is_same_v<typename Iter::value_type, LeafType>>
        struct SimpleDeepCrawler
        {
            using SimpleValue = typename Iter::value_type;
            using SimpleIteratorOfValue = typename Iter::value_type::iterator;
            using SimpleDeepCrawlerSpecificType = SimpleDeepCrawler<SimpleIteratorOfValue, std::is_same_v<typename SimpleValue::value_type, LeafType>>;
    
            SimpleDeepCrawler() = default;
            SimpleDeepCrawler(Iter start, Iter end) : start_(start), end_(end) {
                if(start == end) return;
                layer_ = SimpleDeepCrawlerSpecificType (start_->begin(), start_->end());
            }
            mutable Iter start_;
            Iter end_;
            mutable SimpleDeepCrawlerSpecificType layer_;
        
            LeafType* IncIterator() const
            {
                if (start_ == end_) { return nullptr; }
                LeafType* value_ptr = layer_.IncIterator();
                while (value_ptr == nullptr) {
                    std::advance(start_, 1);
                    if (start_ == end_) { return nullptr; }
                    layer_ = SimpleDeepCrawlerSpecificType (start_->begin(), start_->end());
                    value_ptr = layer_.GetIterator();
                }
                return value_ptr;
            }
        
            LeafType* GetIterator() const
            {
                if (start_ == end_) { return nullptr; }
                return layer_.GetIterator();
            }
        };
    
        template<typename Iter>
        struct SimpleDeepCrawler<Iter, true>
        {
            mutable Iter start_;
            Iter end_;
        
            SimpleDeepCrawler(Iter start, Iter end) : start_(start), end_(end) {}
            SimpleDeepCrawler() = default;
        
            LeafType* IncIterator() const
            {
                std::advance(start_, 1);
                if (start_ == end_) { return nullptr; }
                return const_cast<LeafType*>(&(*start_));
            }
         
            LeafType* GetIterator() const
            {
                if (start_ == end_) { return nullptr; }
                return const_cast<LeafType*>(&(*start_));
            }
        };
    
        //Приватные поля
        mutable SimpleDeepCrawler<Iterator> crawler;
        
        //Конструкторы
        explicit TreeIterator(TreeContainer& container) : crawler(container.begin(), container.end()) {}
        explicit TreeIterator(Iterator value_ptr_start, Iterator value_ptr_end) : crawler(value_ptr_start, value_ptr_end) {}
    
        LeafType* GetTreeIterator() const
        {
            return crawler.GetIterator();
        }
    
        LeafType* IncTreeIterator() const
        {
            return crawler.IncIterator();
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = LeafType;
        using difference_type = std::size_t;
        using pointer = value_type*;
        using reference = value_type&;
    
        TreeIterator() = default;
        TreeIterator(const TreeIterator& other) noexcept: crawler(other.crawler) {}
    
        [[nodiscard]] bool operator==(const TreeIterator<TreeContainer, LeafType>& rhs) const noexcept
        {
            return GetTreeIterator() == rhs.GetTreeIterator();
        }
        [[nodiscard]] bool operator!=(const TreeIterator<TreeContainer, LeafType>& rhs) const noexcept
        {
            return !(GetTreeIterator() == rhs.GetTreeIterator());
        }
        TreeIterator& operator=(const TreeIterator& other) = default;
        TreeIterator& operator++() noexcept
        {
            IncTreeIterator();
            return *this;
        }
        TreeIterator operator++(int) noexcept
        {
            TreeIterator tree_iterator(*this);
            IncTreeIterator();
            return tree_iterator;
        }
        [[nodiscard]] reference operator*() const noexcept
        {
            return *GetTreeIterator();
        }
        [[nodiscard]] pointer operator->() const noexcept
        {
            return &GetTreeIterator();
        }
    };

public:
    //Constructors and destructor
    
    ListFromVecInDegree() = default;
    
    ListFromVecInDegree(Container& data) : data_(std::forward<Container>(data)) {}
    
    ListFromVecInDegree(Container&& data) : data_(std::forward<Container>(data)) {}
    
    ListFromVecInDegree(const ListFromVecInDegree& other)
    {
        ListFromVecInDegree buffer(other.data_);
        std::swap(buffer.data_, data_);
    }
    
    ~ListFromVecInDegree() = default;
    
    //Using
    using value_type = Type;
    using reference = value_type&;
    using const_reference = const value_type&;
    using Iterator = TreeIterator<Container, Type>;
    using ConstIterator = TreeIterator<Container, const Type>;
    
    //Iterators
    [[nodiscard]] Iterator begin() noexcept
    {
        return Iterator(data_.begin(), data_.end());
    }
    
    [[nodiscard]] Iterator end() noexcept
    {
        return Iterator{};
    }
    
    [[nodiscard]] ConstIterator begin() const noexcept
    {
        return cbegin();
    }
    
    [[nodiscard]] ConstIterator end() const noexcept
    {
        return cend();
    }
    
    [[nodiscard]] ConstIterator cbegin() const noexcept
    {
        return ConstIterator(data_.begin(), data_.end());
    }
    
    [[nodiscard]] ConstIterator cend() const noexcept
    {
        return ConstIterator{};
    }
    
    //Operators
    ListFromVecInDegree& operator=(const ListFromVecInDegree& rhs)
    {
        ListFromVecInDegree other(rhs);
        std::swap(data_, other.data_);
        return *this;
    }
};

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server,
        const std::vector<std::string>& queries);

ListFromVecInDegree<std::vector<std::vector<Document>>, Document> ProcessQueriesJoined(
        const SearchServer& search_server, const std::vector<std::string>& queries);

std::vector<Document> ProcessQueriesJoinedInVector(const SearchServer& search_server,
        const std::vector<std::string>& queries);