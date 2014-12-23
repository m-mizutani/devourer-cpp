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
#include "../object.h"

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
      time_t updated_at_;
    public:
      Flow(const swarm::Property &p);
      ~Flow();
      uint64_t hash() { return this->hv_; }
      bool match(const void *key, size_t len) {
        return (len == this->keylen_ && 0 == memcmp(key, this->key_, len));
      }
      void update(time_t ts) { this->updated_at_ = ts; }
      time_t remain() const {
        assert(this->created_at_ <= this->updated_at_);
        return (this->updated_at_ - this->created_at_);
      }
    };

    static const bool DBG;
    static const std::vector<std::string> recv_events_;
    ModDns *mod_dns_;
    time_t flow_timeout_;
    LRUHash flow_table_;
    swarm::ev_id ev_ipv4_;
    swarm::ev_id ev_ipv6_;
    swarm::ev_id ev_dns_;
    time_t last_ts_;
    
  public:
    ModFlow(ModDns *mod_dns);
    ~ModFlow();
    void set_eid(swarm::ev_id ev_ipv4, swarm::ev_id ev_ipv6,
                 swarm::ev_id ev_dns);
    void recv (swarm::ev_id eid, const  swarm::Property &p);
    void exec (const struct timespec &ts);
    const std::vector<std::string>& recv_event() const;
    int task_interval() const;    
  };

}

#endif   // SRC_MODULES_FLOW_H__
