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

#include <sys/stat.h> 
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


#include <iostream>
#include <swarm.h>
#include "./devourer.h"
#include "./object.h"
#include "./debug.h"

namespace devourer {
  FileStream::FileStream(const std::string &fpath) : fpath_(fpath) {
  }
  FileStream::~FileStream() {
  }
  void FileStream::setup() {
    this->fd_ = ::open(this->fpath_.c_str(), O_WRONLY|O_CREAT, 0644);
    if (this->fd_ < 0) {
      std::string err(strerror(errno));
      throw Exception("FileStream Error: " + err);
    }
  }
  void FileStream::emit(const std::string &tag, object::Object *obj, 
                        const struct timeval &ts) throw(Exception) {
    msgpack::sbuffer buf;
    msgpack::packer <msgpack::sbuffer> pk (&buf);
    obj->to_msgpack(&pk);
    ::write(this->fd_, buf.data(), buf.size());
  }


  const bool FluentdStream::DBG = false;

  FluentdStream::FluentdStream(const std::string &host, 
                               const std::string &port) : 
    host_(host), port_(port) {
  }
  FluentdStream::~FluentdStream() {
  }
  void FluentdStream::setup() {
    debug(DBG, "host=%s, port=%s", this->host_.c_str(), this->port_.c_str());

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    
    int r;
    if (0 != (r = getaddrinfo(this->host_.c_str(), this->port_.c_str(), 
                              &hints, &result))) {
      std::string errmsg(gai_strerror(r));
      throw Exception("getaddrinfo error: " + errmsg);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
      this->sock_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (this->sock_ == -1) {
        continue;
      }

      if (connect(this->sock_, rp->ai_addr, rp->ai_addrlen) != -1) {
        char buf[INET6_ADDRSTRLEN];
        struct sockaddr_in *addr_in = (struct sockaddr_in *) rp->ai_addr;
        inet_ntop(rp->ai_family, &addr_in->sin_addr.s_addr, buf, 
                  rp->ai_addrlen);

        debug(DBG, "connected to %s", buf);
        break;
      }
    }

    if (rp == NULL) {
      throw Exception("no avaiable address for " + this->host_);
    }
    freeaddrinfo(result);

  }
  void FluentdStream::emit(const std::string &tag, object::Object *obj,
                           const struct timeval &ts) throw(Exception) {
    msgpack::sbuffer buf;
    msgpack::packer <msgpack::sbuffer> pk (&buf);
    object::Array arr;
    object::Array *msg_set = new object::Array();
    object::Array *msg = new object::Array();
    std::string tag_prefix("devourer.");

    arr.push(tag_prefix + tag);
    arr.push(msg_set);
    msg_set->push(msg);
    msg->push(static_cast<int64_t>(ts.tv_sec));
    msg->push(obj);

    arr.to_msgpack(&pk);
    ::write(this->sock_, buf.data(), buf.size());
  }


  void Plugin::emit(const std::string &tag, object::Object *obj,
                            struct timeval *ts) {
    if (this->stream_) {
      if (ts) {
        this->stream_->emit(tag, obj, *ts);
      } else {
        struct timeval now_ts;
        gettimeofday(&now_ts, NULL);
        this->stream_->emit(tag, obj, now_ts);
      }
    }
  }


  DnsTx::Query::Query(uint64_t hv, uint32_t tx_id) : key_(hv, tx_id) {}
  DnsTx::Query::~Query() {}
  uint64_t DnsTx::Query::hash() {
    return this->key_.hash();
  }
  bool DnsTx::Query::match(const void *key, size_t len) {
    return this->key_.match(key, len);
  }
  void DnsTx::Query::set_ts(double ts) {
    this->last_ts_ = ts;
    this->ts_ = ts;
  }
  double DnsTx::Query::ts() const {
    return this->ts_;
  }
  void DnsTx::Query::set_last_ts(double ts) {
    this->ts_ = ts;
  }
  double DnsTx::Query::last_ts() const {
    return this->last_ts_;
  }
  void DnsTx::Query::set_has_reply(bool has) {
    this->has_reply_ = has;
  }
  bool DnsTx::Query::has_reply() const {
    return this->has_reply_;
  }
  void DnsTx::Query::set_flow(const std::string &client, 
                              const std::string &server) {
    this->client_ = client;
    this->server_ = server;
  }
  void DnsTx::Query::add_question(const std::string &name,
                                  const std::string &type) {
    this->name_.push_back(name);
    this->type_.push_back(type);
  }


  const std::string DnsTx::recv_event_ = "dns.packet";
  const bool DnsTx::DBG = false;

  DnsTx::DnsTx() : last_ts_(0), query_table_(600) {
  }
  DnsTx::~DnsTx() {
    this->query_table_.purge();
    this->flush_query();
  }
  void DnsTx::flush_query() {
    LRUHash::Node *n;
    while(NULL != (n = this->query_table_.pop())) {
      /*
      Query *q = dynamic_cast<Query*>(n);

      printf("TIMEOUT %f %s -> %s [", q->last_ts(), q->client().c_str(), q->server().c_str());
      for(size_t i = 0; i < q->q_count(); i++) {
        printf("%s(%s), ", q->q_name(i).c_str(), q->q_type(i).c_str());
      }
      printf("]: \n");
      */

      delete n;
    }
  }
  void DnsTx::recv (swarm::ev_id eid, const swarm::Property &p) {
    uint32_t qflag = p.value("dns.query").uint32();
    uint32_t tx_id = p.value("dns.tx_id").uint32();
    uint64_t hv = p.hash_value();
    DnsTx::QueryKey key(hv, tx_id);
    const size_t query_ttl = 60;

    // Progress tick of LRU hash table for queries.
    time_t ts = p.tv_sec();
    if (this->last_ts_ == 0) {
      this->last_ts_ = ts; // Initialize
    }

    if (ts > this->last_ts_) {
      this->query_table_.prog(ts - this->last_ts_);
      this->last_ts_ = ts;
    }

    Query *q = dynamic_cast<Query*>
      (this->query_table_.get(key.hash(), key.ptr(), key.len()));

    debug(DBG, "flag:%d, query %p", qflag, q);
    if (qflag == 0) {
      // Handling DNS query.
      // debug(true, "%s -> %s", p.src_addr().c_str(), p.dst_addr().c_str());
      if (!q) {
        // Query is not found.
        q = new Query(hv, tx_id);
        q->set_flow(p.src_addr(), p.dst_addr());
        q->set_ts(p.ts());
        size_t max = p.value_size("dns.qd_name");
        for(size_t i = 0; i < max; i++) {
          q->add_question(p.value("dns.qd_name", i).repr(), 
                          p.value("dns.qd_type", i).repr());
        }
        this->query_table_.put(query_ttl, q);
      } else {
        q->set_last_ts(p.ts());
      }

    } else {
      // Handling DNS response.
      struct timeval tv = {0, 0};
      tv.tv_sec = q->last_ts();

      if (q) {
        double ts = p.ts() - q->last_ts();
        object::Map *map = new object::Map();
        map->put("ts", q->last_ts());
        map->put("client", q->client());
        map->put("server", q->server());
        map->put("q_name", q->q_name(0));
        /*
          object::Array *array = new object::Array();
          for(size_t i = 0; i < q->q_count(); i++) {
          array->push(q->q_name(i));
          // printf("%s(%s), ", , q->q_type(i).c_str());
          }
          map->put("q_name", array);
        */
        map->put("latency", ts);
        this->emit("dns.latency", map, &tv);
        q->set_has_reply(true);

        size_t an_max = p.value_size("dns.an_name");
        for(size_t i = 0; i < an_max; i++) {
          object::Map *m = new object::Map();
          m->put("client", p.dst_addr());
          m->put("server", p.src_addr());
          m->put("name", p.value("dns.an_name", i).repr());
          m->put("type", p.value("dns.an_type", i).repr());
          m->put("data", p.value("dns.an_data", i).repr());
          this->emit("dns.log", m, &tv);
        }

      } else {
        object::Map *map = new object::Map();
        map->put("ts", p.ts());
        map->put("client", p.dst_addr());
        map->put("server", p.src_addr());
        map->put("q_name", p.value("dns.qd_name").repr());
        this->emit("dns.invalid", map, &tv);
      }
    }
  }
  void DnsTx::exec (const struct timespec &ts) {
  }
  const std::string& DnsTx::recv_event() const {
    return DnsTx::recv_event_;
  }
  int DnsTx::task_interval() const {
    return 1;
  }
}

Devourer::Devourer(const std::string &target, devourer::Source src) :
  target_(target), src_(src), sw_(NULL), stream_(NULL)
{
}

Devourer::~Devourer(){
  delete this->sw_;
}

void Devourer::set_fluentd(const std::string &dst) throw(devourer::Exception) {
  size_t pos;
  if(std::string::npos != (pos = dst.find(":", 0))) {
    std::string host = dst.substr(0, pos);
    std::string port = dst.substr(pos + 1);
    this->stream_ = new devourer::FluentdStream(host, port);
  } else {
    throw new devourer::Exception("Fluentd option format must be 'hostname:port'");
  }
}
void Devourer::set_logfile(const std::string &fpath) throw(devourer::Exception) {
  this->stream_ = new devourer::FileStream(fpath);
}

void Devourer::start() throw(devourer::Exception) {
  // Create a new Swarm instance.
  switch(this->src_) {
  case devourer::PCAP_FILE:
    this->sw_ = new swarm::SwarmFile(this->target_);
    break;
  case devourer::INTERFACE:
    this->sw_ = new swarm::SwarmDev(this->target_);
    break;
  }

  if (!this->sw_) {
    throw devourer::Exception("Fatal error");
  }
  if (!this->sw_->ready()) {
    throw devourer::Exception(this->sw_->errmsg());
  }

  // Setup output stream.
  if (this->stream_) {
    this->stream_->setup();
  }

  
  devourer::Plugin *plugin = new devourer::DnsTx();
  {
    std::string ev = plugin->recv_event();
    if (!ev.empty()) {
      swarm::hdlr_id hid = this->sw_->set_handler(ev, plugin);
      if (hid == swarm::HDLR_NULL) {
        throw devourer::Exception(this->sw_->errmsg());
      }
    }

    int interval = plugin->task_interval();
    if (interval > 0) {
      swarm::task_id tid =
        this->sw_->set_periodic_task(plugin, static_cast<float>(interval));
      if (tid == swarm::TASK_NULL) {
        throw devourer::Exception(this->sw_->errmsg());
      }
    }

    plugin->set_stream(this->stream_);
  }

  this->sw_->start();
  delete plugin;
  return;
}
