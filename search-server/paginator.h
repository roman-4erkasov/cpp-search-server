#pragma once
#include <iostream>
#include <vector>

template <typename Iterator>
class Page
{
public:
    Page(Iterator range_begin, Iterator range_end)
        : range_begin_(range_begin), range_end_(range_end)
    {
    }

    auto begin() const
    {
        return range_begin_;
    }

    auto end() const
    {
        return range_end_;
    }

    auto size() const
    {
        return range_end_ - range_begin_;
    }

private:
    Iterator range_begin_;
    Iterator range_end_;
};

template <typename Iterator>
std::ostream &operator<<(
    std::ostream &os, const Page<Iterator> &data)
{
    // std::ostream_iterator<int> out_it (os, "");
    // std::copy(data.begin(), data.end(), out_it );
    for (Iterator it = data.begin(); it != data.end(); ++it)
    {
        os << *it;
    }
    return os;
}

template <typename Iterator>
class Paginator
{
public:
    Paginator(
        Iterator range_begin,
        Iterator range_end,
        int page_size)
    {
        auto ibeg = range_begin;
        auto iend = std::next(ibeg, page_size);
        while (0 < std::distance(iend, range_end))
        {
            pages_.push_back(Page(ibeg, iend));
            ibeg = iend;
            iend = std::next(ibeg, page_size);
        }
        if (0 < std::distance(ibeg, range_end))
            pages_.push_back(Page(ibeg, range_end));
    }

    auto begin() const
    {
        return pages_.begin();
    }
    auto end() const
    {
        return pages_.end();
    }

private:
    std::vector<Page<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container &c, size_t page_size)
{
    return Paginator(begin(c), end(c), page_size);
}
