#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H
// Muduo - A reactor-based C++ network library for Linux
// Copyright (c) 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//   * Neither the name of Shuo Chen nor the names of other contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "LogStream.h"
#include <sys/time.h>

extern __thread char t_time[64];
extern __thread time_t t_lastSecond;

static const int kMicroSecondsPerSecond = 1000 * 1000;

inline int64_t now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return seconds * kMicroSecondsPerSecond + tv.tv_usec;
}

struct LogEntry
{
    static const int kFormatTimeLen = 17 + 9;
    LogEntry() {
        // memset(buf, 0, sizeof(buf));
    }
    void stampTime() {
        timestamp = now();
    }

    void formatTime() {
        int64_t microSecondsSinceEpoch = timestamp;
        time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / kMicroSecondsPerSecond);
        int microseconds = static_cast<int>(microSecondsSinceEpoch % kMicroSecondsPerSecond);
        if(seconds != t_lastSecond) {
            t_lastSecond = seconds;
            struct tm tm_time;
            {
                ::gmtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime
            }

            int len = snprintf(t_time,
                               sizeof(t_time),
                               "%4d%02d%02d %02d:%02d:%02d",
                               tm_time.tm_year + 1900,
                               tm_time.tm_mon + 1,
                               tm_time.tm_mday,
                               tm_time.tm_hour,
                               tm_time.tm_min,
                               tm_time.tm_sec);
            assert(len == 17);
            (void)len;
        }

        {
            muduo::Fmt us(".%06dZ ", microseconds);
            assert(us.length() == 9);
            memcpy(buf, t_time, 17);
            memcpy(buf + 17, us.data(), 9);
        }
    }

    int64_t timestamp;
    uint32_t buflen;
    char buf[muduo::detail::kSmallBuffer];
};

#ifdef USE_SHM
#include "../shm_mpsc_queue.h"
typedef SHMMPSCQueue<LogEntry, 2000> LogQueue;
#else
#include "../mpsc_queue.h"
typedef MPSCQueue<LogEntry> LogQueue;
#endif

extern LogQueue* g_logq;
extern bool g_delay_format_ts;

namespace muduo {

class Logger
{
 public:
  enum LogLevel
  {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  // compile time calculation of basename of source file
  class SourceFile
  {
   public:
    template<int N>
    inline SourceFile(const char (&arr)[N])
      : data_(arr),
        size_(N-1)
    {
      const char* slash = strrchr(data_, '/'); // builtin function
      if (slash)
      {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    explicit SourceFile(const char* filename)
      : data_(filename)
    {
      const char* slash = strrchr(filename, '/');
      if (slash)
      {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }

    const char* data_;
    int size_;
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream& stream() { return impl_.stream_; }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  typedef void (*OutputFunc)(const char* msg, int len);
  typedef void (*FlushFunc)();
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);

 private:

class Impl
{
 public:
  typedef Logger::LogLevel LogLevel;
  Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
  // void formatTime();
  void finish();

  LogStream stream_;
  LogLevel level_;
  int line_;
  SourceFile basename_;
  LogQueue::Entry* entry_;
};

  Impl impl_;

};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel()
{
  return g_logLevel;
}

//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//
#define LOG_TRACE if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
#define LOG_INFO if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
  muduo::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
#define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
#define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
#define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()

const char* strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
  ::muduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char *names, T* ptr)
{
  if (ptr == NULL)
  {
   Logger(file, line, Logger::FATAL).stream() << names;
  }
  return ptr;
}

}

#endif  // MUDUO_BASE_LOGGING_H
