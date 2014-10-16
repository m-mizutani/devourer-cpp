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

#include <exception>
#include <vector>
#include <msgpack.hpp>
#include <swarm.h>

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


  class Stream {
  public:
    Stream() {}
    virtual ~Stream() {}
    virtual void write(const msgpack::sbuffer &sbuf) throw(Exception) = 0;
  };

  class FileStream : public Stream {
  public:
    FileStream(const std::string &fpath);
    virtual ~FileStream();
    void write(const msgpack::sbuffer &sbuf) throw(Exception) = 0;
  };

  class FluentdStream : public Stream {
  public:
    FluentdStream(const std::string &host, int port);
    virtual ~FluentdStream();
    void write(const msgpack::sbuffer &sbuf) throw(Exception) = 0;
  };



  class Plugin : public swarm::Handler {
  private:
    Stream *stream_;

  protected:
    void write_stream(const msgpack::sbuffer &sbuf);

  public:
    Plugin() : stream_(NULL) {}
    virtual ~Plugin() {}
    virtual const std::string& recv_event() const = 0;
    void set_stream(Stream *stream) { this->stream_ = stream; }

  };

  class DnsTx : public Plugin {
  private:
    static const std::string recv_event_;

  public:
    DnsTx();
    ~DnsTx();
    void recv (swarm::ev_id eid, const  swarm::Property &p);
    const std::string& recv_event() const;
  };


  class Proc : public swarm::Task {
  private:
    swarm::Swarm *sw_;
    std::vector<Plugin*> plugins_;

  public:
    Proc(swarm::Swarm *sw);
    ~Proc();
    void exec (const struct timespec &ts);
    void load_plugin(Plugin *plugin);
  };

}

class Devourer {
private:
  std::string target_;
  devourer::Source src_;
  swarm::Swarm *sw_;

public:
  Devourer(const std::string &target, devourer::Source src);
  ~Devourer();
  void set_fluentd(const std::string &dst) throw(devourer::Exception);
  void set_logfile(const std::string &fpath) throw(devourer::Exception);
  void enable_verbose();
  void start() throw(devourer::Exception);
};
