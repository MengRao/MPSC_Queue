#include <bits/stdc++.h>
#include "../mpsc_queue.h"
#include "rdtsc.h"
#include "cpupin.h"
using namespace std;

MPSCQueue<int> q(100, 200);
int64_t produce_total = 0;
int64_t produce_alloc_miss = 0;
volatile int ready = 0;
volatile int done = 0;
const int nthread = 4;
const int num_per_thr = 1000000;

void produce(int nth) {
    cpupin(nth);

    random_device rd;
    mt19937 generator(rd());
    uniform_int_distribution<int> dis(0, 100000);

    __sync_fetch_and_add(&ready, 1);
    while(ready != nthread)
        ;
    int64_t sum = 0;
    int cnt = num_per_thr;
    int alloc_miss_cnt = 0;
    while(cnt--) {
        MPSCQueue<int>::Entry* entry;
        while((entry = q.Alloc()) == nullptr) alloc_miss_cnt++;
        int newval = dis(generator);
        entry->data = newval;
        q.Push(entry);
        sum += newval;
    }

    __sync_fetch_and_add(&produce_total, sum);
    __sync_fetch_and_add(&produce_alloc_miss, alloc_miss_cnt);
    __sync_fetch_and_add(&done, 1);
}


int64_t doConsume() {
    MPSCQueue<int>::Entry* list = q.PopAll();
    if(!list) return 0;
    int64_t sum = 0;
    auto cur = list;
    while(true) {
        sum += cur->data;
        if(!cur->next) break;
        cur = cur->next;
    }
    q.Recycle(list, cur);
    return sum;
}

void consume() {
    cpupin(nthread);
    while(ready != nthread)
        ;
    auto before = rdtsc();
    cout << "started" << endl;

    int64_t sum = 0;
    while(done != nthread) {
        sum += doConsume();
        // std::this_thread::yield();
    }
    sum += doConsume();
    auto after = rdtsc();
    cout << "latency: " << (after - before) << " produce total: " << produce_total << " consume total: " << sum
         << " alloc miss: " << produce_alloc_miss << endl;
}

int main() {
    vector<thread> thr;
    for(int i = 0; i < nthread; i++) {
        thr.emplace_back(produce, i);
    }
    consume();
    for(auto& cur : thr) {
        cur.join();
    }
    return 0;
}


