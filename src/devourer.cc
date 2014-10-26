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
      throw new Exception("FileStream Error: " + err);
    }
  }
  void FileStream::write(const object::Object &obj) throw(Exception) {
    msgpack::sbuffer buf;
    msgpack::packer <msgpack::sbuffer> pk (&buf);
    obj.to_msgpack(&pk);
    ::write(this->fd_, buf.data(), buf.size());
  }

  FluentdStream::FluentdStream(const std::string &host, int port) {
  }
  FluentdStream::~FluentdStream() {
  }
  void FluentdStream::setup() {
  }
  void FluentdStream::write(const object::Object &obj) throw(Exception) {
    msgpack::sbuffer buf;
    msgpack::packer <msgpack::sbuffer> pk (&buf);
    obj.to_msgpack(&pk);
  }


  void Plugin::write_stream(const object::Object &obj) {
    if (this->stream_) {
      this->stream_->write(obj);
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
  void DnsTx::Query::set_flow(const std::string &client, const std::string &server) {
    this->client_ = client;
    this->server_ = server;
  }
  void DnsTx::Query::add_question(const std::string &name, const std::string &type) {
    this->name_.push_back(name);
    this->type_.push_back(type);
  }


  const std::string DnsTx::recv_event_ = "dns.packet";

  DnsTx::DnsTx() : last_ts_(0), query_table_(600) {
  }
  DnsTx::~DnsTx() {
    this->query_table_.purge();
    this->flush_query();
  }
  void DnsTx::flush_query() {
    LRUHash::Node *n;
    while(NULL != (n = this->query_table_.pop())) {
      Query *q = dynamic_cast<Query*>(n);

      printf("TIMEOUT %f %s -> %s [", q->last_ts(), q->client().c_str(), q->server().c_str());
      for(size_t i = 0; i < q->q_count(); i++) {
        printf("%s(%s), ", q->q_name(i).c_str(), q->q_type(i).c_str());
      }
      printf("]: \n");

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

    // const uint8_t *ptr = reinterpret_cast<const uint8_t*>(key.ptr());
    // for(size_t i = 0; i < key.len(); i++) { debug(true, "%d) %02x", i, ptr[i]); }

    LRUHash::Node *n = this->query_table_.get(key.hash(), key.ptr(), key.len());
    Query *q = dynamic_cast<Query*>(n);
    if (qflag == 0) {
      // Handling DNS query.
      if (!q) {
        // Query is not found.
        q = new Query(hv, tx_id);
        q->set_flow(p.src_addr(), p.dst_addr());
        q->set_ts(p.ts());
        size_t max = p.value_size("dns.qd_name");
        for(size_t i = 0; i < max; i++) {
          q->add_question(p.value("dns.qd_name", i).repr(), p.value("dns.qd_type", i).repr());
        }
        this->query_table_.put(query_ttl, q);
      } else {
        q->set_last_ts(p.ts());
      }

    } else {
      // Handling DNS response.
      if (q) {
        double ts = p.ts() - q->last_ts();

        object::Map map;
        map.put("ts", q->last_ts());
        map.put("client", q->client());
        map.put("server", q->server());
        /*
        for(size_t i = 0; i < q->q_count(); i++) {
            printf("%s(%s), ", q->q_name(i).c_str(), q->q_type(i).c_str());
          }
        */
        map.put("latency", ts);
        this->write_stream(map);
        q->set_has_reply(true);
      } else {
        debug(1, "not found");
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
    std::string s_port = dst.substr(pos + 1);
    char *e;
    int port = strtol(s_port.c_str(), &e, 0);
    if (*e != '\0') {
      throw new devourer::Exception("Fluentd option format must be 'hostname:port'");
    }
    this->stream_ = new devourer::FluentdStream(host, port);

  } else {
    throw new devourer::Exception("Fluentd option format must be 'hostname:port'");
  }
}
void Devourer::set_logfile(const std::string &fpath) throw(devourer::Exception) {
  this->stream_ = new devourer::FileStream(fpath);
}

void Devourer::start() throw(devourer::Exception) {
  switch(this->src_) {
  case devourer::PCAP_FILE: this->sw_ = new swarm::SwarmFile(this->target_); break;
  case devourer::INTERFACE: this->sw_ = new swarm::SwarmDev(this->target_); break;
  }

  if (!this->sw_) {
    throw new devourer::Exception("Fatal error");
  }

  if (!this->sw_->ready()) {
    throw new devourer::Exception(this->sw_->errmsg());
  }


  if (this->stream_) {
    this->stream_->setup();
  }

  devourer::Plugin *plugin = new devourer::DnsTx();
  {
    std::string ev = plugin->recv_event();
    if (!ev.empty()) {
      swarm::hdlr_id hid = this->sw_->set_handler(ev, plugin);
      if (hid == swarm::HDLR_NULL) {
        throw new devourer::Exception(this->sw_->errmsg());
      }
    }

    int interval = plugin->task_interval();
    if (interval > 0) {
      swarm::task_id tid =
        this->sw_->set_periodic_task(plugin, static_cast<float>(interval));
      if (tid == swarm::TASK_NULL) {
        throw new devourer::Exception(this->sw_->errmsg());
      }
    }

    plugin->set_stream(this->stream_);
  }

  this->sw_->start();
  delete plugin;
  return;
}
