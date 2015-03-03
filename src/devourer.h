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
#include <deque>
#include <string>

namespace devourer {
  class Exception : public std::exception {
  private:
    std::string errmsg_;
  public:
    Exception(const std::string &errmsg) : errmsg_(errmsg) {}
    ~Exception() {}
    virtual const char* what() const throw() { return this->errmsg_.c_str(); }
  };

  class Module;
  enum Source {
    PCAP_FILE = 1,
    INTERFACE = 2,
  };
}

namespace swarm {
  class NetCap;
  class NetDec;
}

namespace fluent {
  class Logger;
  class MsgQueue;
}

class Devourer {
private:
  std::string target_;
  devourer::Source src_;
  swarm::NetDec *netdec_;
  swarm::NetCap *netcap_;
  fluent::Logger *logger_;
  std::vector<devourer::Module*> modules_;

  void install_module(devourer::Module *module) throw(devourer::Exception);

public:
  Devourer(const std::string &target, devourer::Source src);
  ~Devourer();
  void setdst_fluentd(const std::string &dst);
  void setdst_filestream(const std::string &fpath);
  fluent::MsgQueue* setdst_msgqueue();

  void set_filter(const std::string &filter) throw(devourer::Exception);
  void enable_verbose();

  // to capture
  void start() throw(devourer::Exception);

  // only decoding
  bool input (const uint8_t *data, const size_t len,
              const struct timeval &tv, const size_t cap_len = 0);
};


#endif   // SRC_DEVOURER_H__
