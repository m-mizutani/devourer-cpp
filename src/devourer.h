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

#ifndef SRC_DEVOURER_H__
#define SRC_DEVOURER_H__

#include <exception>
#include <vector>
#include <msgpack.hpp>
#include <swarm.h>

#include "./lru-hash.h"
#include "./object.h"

namespace devourer {
  class Exception : public std::exception {
  private:
    std::string errmsg_;
  public:
    Exception(const std::string &errmsg) : errmsg_(errmsg) {}
    ~Exception() {}
    virtual const char* what() const throw() { return this->errmsg_.c_str(); }
  };

  enum Source {
    PCAP_FILE = 1,
    INTERFACE = 2,
  };

  class Stream;

  class Module : public swarm::Handler, public swarm::Task {
  private:
    Stream *stream_;

  protected:
    void emit(const std::string &tag, object::Object *obj,
              struct timeval *ts = NULL);
  public:
    Module() : stream_(NULL) {}
    virtual ~Module() {}
    virtual const std::string& recv_event() const = 0;
    virtual int task_interval() const = 0;
    void set_stream(Stream *stream) { this->stream_ = stream; }
  };


}

class Devourer {
private:
  std::string target_;
  devourer::Source src_;
  swarm::Swarm *sw_;
  std::vector<devourer::Module*> modules_;
  devourer::Stream *stream_;

  void install_module(devourer::Module *module) throw(devourer::Exception);

public:
  Devourer(const std::string &target, devourer::Source src);
  ~Devourer();
  void set_fluentd(const std::string &dst) throw(devourer::Exception);
  void set_logfile(const std::string &fpath) throw(devourer::Exception);
  void enable_verbose();
  void start() throw(devourer::Exception);
};


#endif   // SRC_DEVOURER_H__
