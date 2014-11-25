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
#include <string.h>
#include <swarm.h>
#include "../devourer.h"
#include "../object.h"
#include "../stream.h"
#include "../debug.h"

namespace devourer {

  std::string v4addr(const void *addr) {
    std::string str;
    char buf[32];
    inet_ntop(AF_INET, addr, buf, sizeof(buf));
    str.assign(buf);
    return str;
  }
  

  // ------------------------------------------------------------
  // class ModDns::Query
  //
  ModDns::Query::Query(uint64_t hv, uint32_t tx_id) : key_(hv, tx_id) {}
  ModDns::Query::~Query() {}
  uint64_t ModDns::Query::hash() {
    return this->key_.hash();
  }
  bool ModDns::Query::match(const void *key, size_t len) {
    return this->key_.match(key, len);
  }
  void ModDns::Query::set_ts(double ts) {
    this->last_ts_ = ts;
    this->ts_ = ts;
  }
  double ModDns::Query::ts() const {
    return this->ts_;
  }
  void ModDns::Query::set_last_ts(double ts) {
    this->ts_ = ts;
  }
  double ModDns::Query::last_ts() const {
    return this->last_ts_;
  }
  void ModDns::Query::set_has_reply(bool has) {
    this->has_reply_ = has;
  }
  bool ModDns::Query::has_reply() const {
    return this->has_reply_;
  }
  void ModDns::Query::set_flow(const std::string &client, 
                              const std::string &server) {
    this->client_ = client;
    this->server_ = server;
  }
  void ModDns::Query::add_question(const std::string &name,
                                  const std::string &type) {
    this->name_.push_back(name);
    this->type_.push_back(type);
  }

  // ------------------------------------------------------------
  // class ModDns::ARecord
  //
  ModDns::ARecord::ARecord(const std::string &name, const void *key,
                           size_t keylen, time_t init_ts) :
    name_(name), keylen_(keylen),
    init_ts_(init_ts), last_ts_(init_ts)
  {
    this->hv_ = ARecord::calc_hash(key, keylen);
    this->key_ = ::malloc(keylen);
    ::memcpy(this->key_, key, keylen);
    /*
    const uint8_t *kp = reinterpret_cast<const uint8_t*>(key);
    debug(true, "Reg: %u.%u.%u.%u, hv=%llu (%s)", kp[0], kp[1], kp[2], kp[3],
          this->hv_, name.c_str());
    */
  }
  ModDns::ARecord::~ARecord() {
    ::free(this->key_);
  }
  void ModDns::ARecord::update(time_t ts) {
    if (this->last_ts_ < ts) {
      this->last_ts_ = ts;
    }
  }
  uint64_t ModDns::ARecord::calc_hash(const void *key, size_t keylen) {
    uint64_t hv = 0;
    if (keylen == 4) {
      // IPv4 addr
      const uint32_t *kp = reinterpret_cast<const uint32_t*>(key);
      hv = static_cast<uint32_t>(*kp);
    } else if (keylen == 16) {
      // IPv6 addr
      const uint64_t *kp = reinterpret_cast<const uint64_t*>(key);
      hv = kp[0] ^ kp[1];
    } else {
      assert(0); // Invalid address length
    }
    return hv;
  }
  uint64_t ModDns::ARecord::hash() {
    return this->hv_;
  }
  bool ModDns::ARecord::match(const void *key, size_t len) {
    /*
    const uint32_t *k1 = reinterpret_cast<const uint32_t*>(key);
    const uint32_t *k2 = reinterpret_cast<const uint32_t*>(this->key_);
    std::string a1 = v4addr(key);
    std::string a2 = v4addr(this->key_);
    debug(true, "%s:%08X, %s:%08X", a1.c_str(), *k1, a2.c_str(), *k2);
    */
    return (this->keylen_ == len && 0 == ::memcmp(key, this->key_, len));
  }

  // ------------------------------------------------------------
  // class ModDns::CNameRecord
  //
  ModDns::CNameRecord::CNameRecord(const std::string &qname,
                                   const std::string &cname, time_t init_ts) :
    qname_(qname), cname_(cname), last_ts_(init_ts)
  {
  }
  ModDns::CNameRecord::~CNameRecord() {
  }
  void ModDns::CNameRecord::update(time_t ts) {
    this->last_ts_ = ts;
  }
  uint64_t ModDns::CNameRecord::calc_hash(const std::string &name) {
    return 0;
  }
  uint64_t ModDns::CNameRecord::hash() {
    // ToDo:
    return 0;
  }
  bool ModDns::CNameRecord::match(const void *key, size_t len) {
    return (this->cname_.length() == len &&
            0 == ::memcmp(key, this->cname_.data(), len));
  }


  // ------------------------------------------------------------
  // class ModDns
  //

  const std::vector<std::string> ModDns::recv_event_{"dns.packet"};
  const bool ModDns::DBG = false;

  ModDns::ModDns() : last_ts_(0), query_table_(600), addr_table_(1200),
                     name_table_(1200)
  {
  }
  ModDns::~ModDns() {
    this->query_table_.purge();
    this->flush_query();
  }
  void ModDns::flush_query() {
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

  void ModDns::recv (swarm::ev_id eid, const swarm::Property &p) {
    uint32_t qflag = p.value("dns.query").uint32();
    uint32_t tx_id = p.value("dns.tx_id").uint32();
    uint64_t hv = p.hash_value();
    ModDns::QueryKey key(hv, tx_id);
    const size_t query_ttl = 120;
    const size_t cache_ttl = 600;

    // Progress tick of LRU hash table for queries.
    const time_t ts = p.tv_sec();
    if (this->last_ts_ > 0 && ts > this->last_ts_) {
      const time_t diff = ts - this->last_ts_;
      // ToDo: Need to prune expired nodes
      this->query_table_.prog(diff);
      this->addr_table_.prog(diff);
      this->name_table_.prog(diff);
    }

    this->last_ts_ = ts;
    
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

          // XXX: Merge A/AAAA record process and CNAME record process
          uint32_t rec_type = p.value("dns.an_type", i).uint32();
          if (rec_type == 1 || rec_type == 28) {
            // A record or AAAA record
            size_t keylen;
            const void *key = p.value("dns.an_data", i).ptr(&keylen);

            const std::string& qname = p.value("dns.an_name", i).repr();
            uint64_t hv = ARecord::calc_hash(key, keylen);
            ARecord *rec =
              dynamic_cast<ARecord*>(this->addr_table_.get(hv, key, keylen));
            if (rec) {
              rec->update(ts);
            } else {
              rec = new ARecord(qname, key, keylen, ts);
              this->addr_table_.put(cache_ttl, rec);
            }
          } else if (rec_type == 5) {
            // CNAME record
            const std::string& qname = p.value("dns.an_name", i).repr();
            const std::string& cname = p.value("dns.an_data", i).repr();
            const size_t keylen = qname.length();
            const void *key = qname.c_str();
            uint64_t hv = CNameRecord::calc_hash(cname);
            CNameRecord *rec = dynamic_cast<CNameRecord*>
              (this->name_table_.get(hv, key, keylen));
              
            if (rec) {
              rec->update(ts);
            } else {
              rec = new CNameRecord(qname, cname, ts);
              this->name_table_.put(cache_ttl, rec);
            }
          }
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

  const std::string ModDns::null_str_;
  const std::string& ModDns::lookup_addr(const void *addr, size_t len) {
    assert(len == 4 || len == 16);
    uint64_t hv = ARecord::calc_hash(addr, len);
    ARecord *rec = dynamic_cast<ARecord*>
      (this->addr_table_.get(hv, addr, len));

    /*
    std::string saddr = v4addr(addr);
    debug(false, "Lookup: %s, hv=%llu", saddr.c_str(), hv);
    */
    
    if (rec) {
      return rec->name();
    } else {
      return ModDns::null_str_;
    }
  }

  const std::string& ModDns::lookup_name(const std::string& name) {
    const void *key = name.c_str();
    const size_t keylen = name.length();
    uint64_t hv = CNameRecord::calc_hash(name);
    CNameRecord *rec = dynamic_cast<CNameRecord*>
      (this->name_table_.get(hv, key, keylen));

    /*
    std::string saddr = v4addr(addr);
    debug(false, "Lookup: %s, hv=%llu", saddr.c_str(), hv);
    */
    
    if (rec) {
      return rec->qname();
    } else {
      return ModDns::null_str_;
    }
  }

  void ModDns::exec (const struct timespec &ts) {
  }
  const std::vector<std::string>& ModDns::recv_event() const {
    return ModDns::recv_event_;
  }
  int ModDns::task_interval() const {
    return 1;
  }
}
