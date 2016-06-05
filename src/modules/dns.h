/*
 * Copyright (c) 2014 Masayoshi Mizutani <mizutani@sfc.wide.ad.jp>
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

#ifndef SRC_MODULES_DNS_H__
#define SRC_MODULES_DNS_H__

#include <exception>
#include <vector>
#include <msgpack.hpp>

#include "../module.h"
#include "../devourer.h"
#include "../lru-hash.h"

namespace devourer {
  class ModDns : public Module {
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

    class ARecord : public LRUHash::Node {
    private:
      const std::string name_;
      void *key_;
      const size_t keylen_;
      const time_t init_ts_;
      time_t last_ts_;
      uint64_t hv_;
      
    public:
      ARecord(const std::string &name, const void *key, size_t keylen,
              time_t init_ts);
      ~ARecord();
      void update(time_t ts);
      const std::string &name() { return this->name_; }
      static uint64_t calc_hash(const void *key, size_t keylen);
      uint64_t hash();
      bool match(const void *key, size_t len);
    };


    class CNameRecord : public LRUHash::Node {
    private:
      const std::string qname_;
      const std::string cname_;
      uint64_t hv_;
      time_t last_ts_;
      
    public:
      CNameRecord(const std::string &qname, const std::string &cname,
                  time_t init_ts);
      ~CNameRecord();
      void update(time_t ts);
      const std::string& qname() { return this->qname_; }
      static uint64_t calc_hash(const std::string &name);
      uint64_t hash();
      bool match(const void *key, size_t len);
    };

    
    static const bool DBG;
    static const std::vector<std::string> recv_event_;
    static const std::string null_str_;
    
    time_t last_ts_;
    LRUHash query_table_;
    LRUHash addr_table_;
    LRUHash name_table_;
    void flush_query();

  public:
    ModDns();
    ~ModDns();
    void recv (swarm::ev_id eid, const  swarm::Property &p);
    void exec (const struct timespec &ts);
    const std::vector<std::string>& recv_event() const;
    int task_interval() const;
    // ToDo: add const to lookup functions
    const std::string& resolv_addr(const void *addr, size_t len,
                                   size_t recur_max=32);
  };

}

#endif   // SRC_MODULES_DNS_H__
