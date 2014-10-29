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


  class Stream {
  public:
    Stream() {}
    virtual ~Stream() {}
    virtual void setup() = 0;
    virtual void emit(const std::string &tag, object::Object *obj, 
                       const struct timeval &ts) throw(Exception) = 0;
  };

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



  class Plugin : public swarm::Handler, public swarm::Task {
  private:
    Stream *stream_;

  protected:
    void emit(const std::string &tag, object::Object *obj,
              struct timeval *ts = NULL);
  public:
    Plugin() : stream_(NULL) {}
    virtual ~Plugin() {}
    virtual const std::string& recv_event() const = 0;
    virtual int task_interval() const = 0;
    void set_stream(Stream *stream) { this->stream_ = stream; }
  };


  class DnsTx : public Plugin {
  private:
    class QueryKey {
    private:
      uint64_t key_[2];

    public:
      QueryKey(uint64_t hv, uint32_t tx_id) {
        this->key_[0] = hv;
        this->key_[1] = static_cast<uint64_t>(tx_id);
      }
      const uint64_t *ptr() const { return &(this->key_[0]); }
      size_t len() const { return sizeof(this->key_);}
      uint64_t hash() const { return (this->key_[0] ^ this->key_[1]); }
      bool match(const void *key, size_t len) {
        return (len == sizeof(this->key_) && 0 == memcmp(key, this->key_, sizeof(this->key_)));
      }
    };

    class Query : public LRUHash::Node {
    private:
      double last_ts_;
      double ts_;
      QueryKey key_;
      bool has_reply_;
      std::string client_, server_;
      std::vector<std::string> name_;
      std::vector<std::string> type_;

    public:
      Query(uint64_t hv, uint32_t tx_id);
      ~Query();
      uint64_t hash();
      bool match(const void *key, size_t len);
      void set_ts(double ts);
      void set_last_ts(double ts);
      double ts() const;
      double last_ts() const;
      void set_has_reply(bool has);
      bool has_reply() const;
      void set_flow(const std::string &client, const std::string &server);
      void add_question(const std::string &name, const std::string &type);
      size_t q_count() const { return this->name_.size(); }
      const std::string& q_name(size_t i) { return this->name_[i]; }
      const std::string& q_type(size_t i) { return this->type_[i]; }
      const std::string& client() { return this->client_; }
      const std::string& server() { return this->server_; }
    };

    static const bool DBG;
    static const std::string recv_event_;

    time_t last_ts_;
    LRUHash query_table_;
    void flush_query();

  public:
    DnsTx();
    ~DnsTx();
    void recv (swarm::ev_id eid, const  swarm::Property &p);
    void exec (const struct timespec &ts);
    const std::string& recv_event() const;
    int task_interval() const;    
  };

}

class Devourer {
private:
  std::string target_;
  devourer::Source src_;
  swarm::Swarm *sw_;
  std::vector<devourer::Plugin*> plugins_;
  devourer::Stream *stream_;

public:
  Devourer(const std::string &target, devourer::Source src);
  ~Devourer();
  void set_fluentd(const std::string &dst) throw(devourer::Exception);
  void set_logfile(const std::string &fpath) throw(devourer::Exception);
  void enable_verbose();
  void start() throw(devourer::Exception);
};
