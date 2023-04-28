#pragma once

#include <vector>

// push elements until FixVector is full, then every push_back will push the first element out of the FixVector
template<class T>
class FixVector
{
public:
    FixVector(uint32_t size, T initial_value) : content(size * 2, initial_value), current_idx(0), capacity(size)
    {
        // size must be power of 2
        assert((size & (size - 1)) == 0);
    }

    FixVector(uint32_t size) : FixVector(size, T())
    {}

    void push_back(T element)
    {
        // write every element two times to build two copies of the data
        // if current_idx reaches the capacity reset current_idx to 0
        content[current_idx] = element;
        content[current_idx + capacity] = element;
        current_idx = (current_idx + 1) & (capacity - 1);
    }

    T* data()
    {
        // the actual data always starts at current_idx and ends after capacity elements
        return content.data() + current_idx;
    }

    uint32_t size()
    {
        return capacity;
    }

private:
    std::vector<T> content;
    uint32_t current_idx;
    const uint32_t capacity;
};
