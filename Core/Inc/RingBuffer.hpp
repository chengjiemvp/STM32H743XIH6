/// @file    RingBuffer.hpp
#pragma once

#include <cstdint>
#include <array>
#include <atomic>

template<typename T, std::size_t Size>
class RingBuffer {
    public:
        RingBuffer() : head(0), tail(0) {}

        /// @brief  pushes an item to the buffer
        /// @param  item the item to push
        /// @return true if the item was pushed sucessfully
        bool push(const T& item) {
            size_t next_head = (head +1) % Size;
            if (next_head == tail) {
                return false; // buffer full
            }
            buffer[head] = item;
            head = next_head;
            return true;
        }
        
        /// @brief  pops an item from the buffer
        /// @param  item reference to store the popped item
        /// @return true if an item was popped successfully
        bool pop(T& item) {
            if (is_empty()) {
                return false;
            }
            item = buffer[tail];
            tail = (tail +1) % Size;
            return true;
        }

        /// @brief  checks if the buffer is empty
        /// @return true if buffer is empty
        bool is_empty() const {
            return head == tail;
        }

        /// @brief  checks if the buffer is full
        /// @return true if buffer is full
        bool is_full() const {
            return ((head + 1) % Size) == tail;
        }

        /// @brief  gets the number of items currently in the buffer
        /// @return number of items
        size_t count() const {
            if (head >= tail) {
                return head - tail;
            }
            return Size + head - tail;
        }

    private:
        std::array<T, Size> buffer;
        volatile uint32_t head; // modified by ISR
        volatile uint32_t tail;
};