#pragma once
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
#include <sys/types.h>

class AppendFile
{
public:
    explicit AppendFile(const char* filename)
        : fp_(::fopen(filename, "ae"))
        , writtenBytes_(0) {
        ::setbuffer(fp_, buffer_, sizeof buffer_);
    }

    ~AppendFile() {
        ::fclose(fp_);
    }

    void append(const char* logline, const size_t len) {
        size_t n = write(logline, len);
        size_t remain = len - n;
        while(remain > 0) {
            size_t x = write(logline + n, remain);
            if(x == 0) {
                int err = ferror(fp_);
                if(err) {
                    fprintf(stderr, "AppendFile::append() failed %d\n", err);
                }
                break;
            }
            n += x;
            remain = len - n; // remain -= x
        }

        writtenBytes_ += len;
    }

    void flush() {
        ::fflush(fp_);
    }

    off_t writtenBytes() const {
        return writtenBytes_;
    }

private:
    size_t write(const char* logline, size_t len) {
        return ::fwrite_unlocked(logline, 1, len, fp_);
    }

    FILE* fp_;
    char buffer_[64 * 1024];
    off_t writtenBytes_;
};
