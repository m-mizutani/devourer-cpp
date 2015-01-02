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

#include "./object.h"
#include "./devourer.h"

namespace devourer {
  class Stream {
  public:
    Stream() {}
    virtual ~Stream() {}
    virtual void setup() = 0;
    virtual void emit(const std::string &tag, object::Object *obj, 
                       const struct timeval &ts) throw(Exception) = 0;
  };

  // -----------------------------------
  class FileStream : public Stream {
  private:
    std::string fpath_;
    int fd_;

  public:
    FileStream(const std::string &fpath);
    virtual ~FileStream();
    void setup();
    void emit(const std::string &tag, object::Object *obj, 
               const struct timeval &ts) throw(Exception);
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
    void emit(const std::string &tag, object::Object *obj, 
               const struct timeval &ts) throw(Exception);
  };

}

#endif   // SRC_STREAM_H__
