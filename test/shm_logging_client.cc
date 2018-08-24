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

int main(int argc, char* argv[]) {
    // cpupin(1);
#ifdef USE_SHM
    cout << "use shm" << endl;
#else
    cout << "not use shm" << endl;
#endif
    g_logq = shmmap<LogQueue>("/LogQueue.shm");
    if(!g_logq) return 1;

    {
        // set max virtual memory to 2GB.
        size_t kOneGB = 1000 * 1024 * 1024;
        rlimit rl = {2 * kOneGB, 2 * kOneGB};
        setrlimit(RLIMIT_AS, &rl);
    }
    printf("pid = %d\n", getpid());

    bool longLog = argc > 1;
    bench(longLog);
}



