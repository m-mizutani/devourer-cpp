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

#ifndef SRC_MODULES_FLOW_H__
#define SRC_MODULES_FLOW_H__

#include <exception>
#include <vector>
#include <assert.h>

#include "../module.h"
#include "../devourer.h"
#include "../lru-hash.h"

namespace devourer {
  class ModDns;
  class ModFlow : public Module {
  private:
    class Flow : public LRUHash::Node {
    private:
      uint64_t hv_;
      void *key_;
      size_t keylen_;
      time_t created_at_;
      time_t refreshed_at_;
      time_t updated_at_;
      swarm::FlowDir init_dir_;

      std::string l_addr_, r_addr_;
      std::string l_name_, r_name_;
      int l_port_, r_port_;
      int l_pkt_, r_pkt_;
      int l_size_, r_size_;
      std::string proto_;
      std::string hash_;

    public:
      Flow(const swarm::Property &p, const std::string& src_name = "",
           const std::string& dst = "");
      ~Flow();
      uint64_t hash() { return this->hv_; }
      bool match(const void *key, size_t len) {
        return (len == this->keylen_ && 0 == memcmp(key, this->key_, len));
      }
      void update(const swarm::Property &p);
      void refresh(time_t tv_sec) {
        this->refreshed_at_ = tv_sec;
      }
      time_t remain() const {
        if (this->refreshed_at_ <= this->updated_at_) {
          return (this->updated_at_ - this->refreshed_at_);
        } else {
          return 0;
        }
      }
      void set_l_name(const std::string& name) { this->l_name_ = name; }
      void set_r_name(const std::string& name) { this->r_name_ = name; }
      
      void build_message(fluent::Message *msg);
      void created_at(struct timeval *tv) const {
        tv->tv_sec = this->created_at_;
        tv->tv_usec = 0;
      }
    };

    static const bool DBG;
    static const std::vector<std::string> recv_events_;
    ModDns *mod_dns_;
    time_t flow_timeout_;
    LRUHash flow_table_;
    swarm::ev_id ev_ipv4_;
    swarm::ev_id ev_ipv6_;
    time_t last_ts_;
    
  public:
    ModFlow(ModDns *mod_dns);
    ~ModFlow();
    void recv (swarm::ev_id eid, const  swarm::Property &p);
    void exec (const struct timespec &ts);
    const std::vector<std::string>& recv_event() const;
    int task_interval() const;
    void bind_event_id(const std::string &ev_name, swarm::ev_id eid);

  };

}

#endif   // SRC_MODULES_FLOW_H__
