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

#include "./dns.h"
#include <iostream>
#include <swarm.h>
#include "../devourer.h"
#include "../object.h"
#include "../stream.h"
#include "../debug.h"

namespace devourer {
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


  const std::vector<std::string> DnsTx::recv_event_{"dns.packet"};
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
      Query *q = dynamic_cast<Query*>(n);

      if(!(q->has_reply())) {
        struct timeval tv = {0, 0};
        tv.tv_sec = q->last_ts();
        object::Map *map = new object::Map();
        map->put("ts", q->last_ts());
        map->put("client", q->client());
        map->put("server", q->server());
        map->put("q_name", q->q_name(0));
        map->put("status", "timeout");
        this->emit("dns.tx", map, &tv);
      }

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

      if (q) {
        tv.tv_sec = q->last_ts();
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
        map->put("status", "success");
        map->put("latency", ts);
        this->emit("dns.tx", map, &tv);
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
        tv.tv_sec = p.ts();
        map->put("ts", p.ts());
        map->put("client", p.dst_addr());
        map->put("server", p.src_addr());
        map->put("q_name", p.value("dns.qd_name").repr());
        map->put("status", "miss");
        this->emit("dns.tx", map, &tv);
      }
    }
  }
  void DnsTx::exec (const struct timespec &ts) {
  }
  const std::vector<std::string>& DnsTx::recv_event() const {
    return DnsTx::recv_event_;
  }
  int DnsTx::task_interval() const {
    return 1;
  }
}
