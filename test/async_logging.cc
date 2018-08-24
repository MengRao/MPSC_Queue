#include <bits/stdc++.h>
#include "Logging.h"
#include "AppendFile.h"
#include "cpupin.h"
#include <unistd.h>
#include <sys/resource.h>
using namespace std;

volatile bool running = true;

void bench(bool longLog) {
    int cnt = 0;
    const int kBatch = 1000;
    string empty = " ";
    string longStr(3000, 'X');
    longStr += " ";

    for(int t = 0; t < 30; ++t) {
        int64_t start = now();
        for(int i = 0; i < kBatch; ++i) {
            LOG_INFO << "Hello 0123456789"
                     << " abcdefghijklmnopqrstuvwxyz " << (longLog ? longStr : empty) << cnt;
            ++cnt;
        }
        int64_t end = now();
        printf("%f\n", static_cast<double>(end - start) / kBatch);
        struct timespec ts = {0, 500 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
}

void logServer(const string& logfile) {
    AppendFile file(logfile.c_str());
    while(running) {
        LogQueue::Entry* list = g_logq.PopAll();
        if(!list) {
            this_thread::yield();
            continue;
        }
        auto cur = list;
        while(true) {
            if(g_delay_format_ts) {
                cur->data.formatTime();
            }
            file.append(cur->data.buf, cur->data.buflen);
            if(!cur->next) break;
            cur = cur->next;
        }
        g_logq.Recycle(list, cur);
        file.flush();
    }
}

int main(int argc, char* argv[]) {
    cpupin(1);
    // g_delay_format_ts = false;
    {
        // set max virtual memory to 2GB.
        size_t kOneGB = 1000 * 1024 * 1024;
        rlimit rl = {2 * kOneGB, 2 * kOneGB};
        setrlimit(RLIMIT_AS, &rl);
    }
    printf("pid = %d\n", getpid());
    char name[256] = {0};
    strncpy(name, argv[0], sizeof name - 1);
    thread srv_thr(logServer, string(::basename(name)) + ".log");

    bool longLog = argc > 1;
    bench(longLog);
    running = false;
    srv_thr.join();
}


