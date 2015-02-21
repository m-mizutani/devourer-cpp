/*
 * Copyright (c) 2014 Masayoshi Mizutani <muret@haeena.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SRC_STREAM_H__
#define SRC_STREAM_H__

#include <regex>
#include "./object.h"
#include "./devourer.h"

namespace devourer {
  class Stream {
  private:
    std::regex *filter_;
    virtual void output(const std::string &tag, object::Object *obj,
                        const struct timeval &ts) throw(Exception) = 0;
  public:
    Stream() : filter_(nullptr) {}
    virtual ~Stream() {}
    virtual void setup() = 0;
    void emit(const std::string &tag, object::Object *obj,
              const struct timeval &ts) throw(Exception);
    void set_filter(const std::string &filter) throw(Exception);
  };

  // -----------------------------------
  class FileStream : public Stream {
  private:
    std::string fpath_;
    int fd_;
    void output(const std::string &tag, object::Object *obj,
                const struct timeval &ts) throw(Exception);

  public:
    FileStream(const std::string &fpath);
    virtual ~FileStream();
    void setup();
  };

  // -----------------------------------
  class FluentdStream : public Stream {
  private:
    std::string host_;
    std::string port_;
    int sock_;
    const static bool DBG;

  public:
    FluentdStream(const std::string &host, const std::string &port);
    virtual ~FluentdStream();
    void setup();
    void output(const std::string &tag, object::Object *obj,
                const struct timeval &ts) throw(Exception);
    
  };

  // -----------------------------------
  class BufferStream : public Stream {
  private:
    Buffer *buf_;
    const static bool DBG;

  public:
    BufferStream(Buffer *buf);
    virtual ~BufferStream();
    void setup();
    void output(const std::string &tag, object::Object *obj,
                const struct timeval &ts) throw(Exception);
    
  };

}

#endif   // SRC_STREAM_H__
