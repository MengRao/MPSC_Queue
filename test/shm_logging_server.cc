#include <bits/stdc++.h>
#include "Logging.h"
#include "AppendFile.h"
#include "cpupin.h"
#include "shmmap.h"
#include <unistd.h>
#include <sys/resource.h>
using namespace std;

LogQueue* g_logq = nullptr;
bool g_delay_format_ts = true;

volatile bool running = true;

void logServer(const string& logfile) {
    AppendFile file(logfile.c_str());
    while(running) {
        LogQueue::Entry* list = g_logq->PopAll();
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
            LogQueue::Entry* next = g_logq->NextEntry(cur);
            if(!next) break;
            cur = next;
        }
        g_logq->Recycle(list, cur);
        file.flush();
    }
}

int main(int argc, char* argv[]) {
    // cpupin(2);
    g_logq = shmmap<LogQueue>("/LogQueue.shm");
    if(!g_logq) return 1;
    {
        // set max virtual memory to 2GB.
        size_t kOneGB = 1000 * 1024 * 1024;
        rlimit rl = {2 * kOneGB, 2 * kOneGB};
        setrlimit(RLIMIT_AS, &rl);
    }
    printf("pid = %d\n", getpid());
    char name[256] = {0};
    strncpy(name, argv[0], sizeof name - 1);
    logServer(string(::basename(name)) + ".log");
}



