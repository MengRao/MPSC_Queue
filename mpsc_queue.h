#pragma once
#include <deque>

template<class EntryData>
class MPSCQueue
{
public:
    struct Entry
    {
        Entry* next;
        EntryData data;
    };

    MPSCQueue(uint32_t init_size, uint32_t max_size)
        : pending_tail((Entry*)&pending_head)
        , max_entry(max_size) {
        while(init_size--) {
            Entry* cur = _Alloc();
            cur->next = empty_top;
            empty_top = cur;
        }
    }

    Entry* Alloc() {
        Entry* top = FetchAndLock(empty_top, (Entry*)EmptyLock);
        if(top == nullptr) {
            empty_top = nullptr;
            return _Alloc();
        }
        empty_top = top->next;
        return top;
    }

    // A correct and the most efficient implementation that I've tested
    // In normal case pending_tail saves the address of tail entry(if empty it save the address of head)
    // contention winner will temporarily set pending_tail to PendingLock
    void Push(Entry* entry) {
        Entry* tail = FetchAndLock(pending_tail, (Entry*)PendingLock);
        tail->next = entry;
        asm volatile("" : : "m"(tail->next), "m"(pending_tail) :);       // memory fence
        pending_tail = entry;
    }

    // Dont use. An incorrect implemnetation similar to Push, supposed to be faster but indeed slower, who can tell me
    // why? it's incorrect in that when pending_tail is changed to the new entry, its next is not guaranteed to point to
    // it so the consumer could get a corrupt list
    void Push2(Entry* entry) {
        Entry* tail = pending_tail;
        while(true) {
            Entry* cur_tail = __sync_val_compare_and_swap(&pending_tail, tail, entry);
            if(__builtin_expect(cur_tail == tail, 1)) break;
            tail = cur_tail;
        }
        tail->next = entry;
    }

    // Dont use. Another correct implementation of Push, but turned out to be slightly slower
    // In normal case pending_tail's next is PendingLock,
    // and any other entry's next must not be PendingLock regardless of whether it's on the pending list
    // contention winner will set pending_tail's next to the new entry and pending_tail to the new entry
    void Push3(Entry* entry) {
        entry->next = (Entry*)PendingLock;
        while(__builtin_expect(!__sync_bool_compare_and_swap(&pending_tail->next, PendingLock, entry), 0))
            ;
        pending_tail = entry;
    }

    // Similar to Push(), but it directly set pending_tail to the address of pending_head
    Entry* PopAll() {
        if(pending_tail == (Entry*)&pending_head) return nullptr;
        Entry* tail = FetchAndLock(pending_tail, (Entry*)PendingLock);
        Entry* ret = pending_head;
        asm volatile("" : : "m"(pending_tail) :); // memory fence
        pending_tail = (Entry*)&pending_head;
        tail->next = nullptr;
        return ret;
    }

    void Recycle(Entry* first, Entry* last) {
        Entry* top = FetchAndLock(empty_top, (Entry*)EmptyLock);
        last->next = top;
        asm volatile("" : : "m"(last->next), "m"(empty_top) :); // memory fence
        empty_top = first;
    }

private:
    template<class T>
    static T FetchAndLock(volatile T& var, T lock_val) {
        T val = var;
        while(true) {
            while(__builtin_expect(val == lock_val, 0)) val = var;
            T new_val = __sync_val_compare_and_swap(&var, val, lock_val);
            if(__builtin_expect(new_val == val, 1)) break;
            val = new_val;
        }
        return val;
    }

    Entry* _Alloc() {
        if(entry_cnt >= max_entry) return nullptr;
        int cnt = FetchAndLock(entry_cnt, -1);
        if(__builtin_expect(cnt >= max_entry, 0)) {
            entry_cnt = cnt;
            return nullptr;
        }
        deq.emplace_back();
        Entry* ret = &deq.back();
        asm volatile("" : : "m"(entry_cnt) :); // memory fence
        entry_cnt = deq.size();
        return ret;
    }

private:
    static constexpr long PendingLock = 0x8;
    static constexpr long EmptyLock = 0x10;

    alignas(64) Entry* volatile pending_tail;
    Entry* pending_head;

    alignas(64) Entry* volatile empty_top = nullptr;

    alignas(64) volatile int entry_cnt = 0;
    std::deque<Entry> deq;
    const int max_entry;
};
