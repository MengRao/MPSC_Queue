#pragma once

template<class EntryData, uint32_t SIZE>
class SHMMPSCQueue
{
public:
    struct Entry
    {
        uint32_t next;
        EntryData data;
    };

    SHMMPSCQueue() {
        for(uint32_t i = 1; i < SIZE; i++) {
            entries[i].next = i + 1;
        }
    }

    Entry* Alloc() {
        uint32_t top = FetchAndLock(empty_top, EmptyLock);
        if(top == SIZE) {
            empty_top = SIZE;
            return nullptr;
        }
        empty_top = entries[top].next;
        return &entries[top];
    }

    void Push(Entry* entry) {
        uint32_t entry_idx = entry - entries;
        uint32_t tail = FetchAndLock(pending_tail, PendingLock);
        entries[tail].next = entry_idx;
        asm volatile("" : : "m"(entries[tail].next), "m"(pending_tail) :); // memory fence
        pending_tail = entry_idx;
    }

    Entry* PopAll() {
        if(pending_tail == 0) return nullptr;
        uint32_t tail = FetchAndLock(pending_tail, PendingLock);
        Entry* ret = &entries[entries[0].next];
        asm volatile("" : : "m"(pending_tail) :); // memory fence
        pending_tail = 0;
        entries[tail].next = SIZE;
        return ret;
    }

    Entry* NextEntry(Entry* entry) {
        if(entry->next == SIZE) return nullptr;
        return &entries[entry->next];
    }

    void Recycle(Entry* first, Entry* last) {
        uint32_t top = FetchAndLock(empty_top, EmptyLock);
        last->next = top;
        asm volatile("" : : "m"(last->next), "m"(empty_top) :); // memory fence
        empty_top = first - entries;
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

private:
    static constexpr uint32_t PendingLock = SIZE + 1;
    static constexpr uint32_t EmptyLock = SIZE + 2;

    alignas(64) uint32_t volatile pending_tail = 0;

    alignas(64) uint32_t volatile empty_top = 1;

    alignas(64) Entry entries[SIZE];
};

