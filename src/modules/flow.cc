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

#include "./flow.h"
#include <iostream>
#include <swarm.h>
#include "../devourer.h"
#include "../object.h"
#include "../stream.h"
#include "../debug.h"
#include "./dns.h"

namespace devourer {
  const std::vector<std::string> ModFlow::recv_events_{"ipv4.packet",
      "ipv6.packet", "dns.packet"};
  const bool ModFlow::DBG = true;
  
  // ------------------------------------------------------------
  // class ModFlow
  ModFlow::ModFlow(DnsTx *mod_dns) :
    mod_dns_(mod_dns),
    flow_timeout_(600), flow_table_(3600), last_ts_(0)
  {
  }
  ModFlow::~ModFlow() {
    LRUHash::Node *node;
    this->flow_table_.purge();
    while(NULL != (node = this->flow_table_.pop())) {
      Flow *flow = dynamic_cast<Flow*>(node);
        delete flow;
    }
  }
      
  void ModFlow::set_eid(swarm::ev_id ev_ipv4, swarm::ev_id ev_ipv6,
                        swarm::ev_id ev_dns) {
    this->ev_ipv4_ = ev_ipv4;
    this->ev_ipv6_ = ev_ipv6;
    this->ev_dns_  = ev_dns;    
  }
  void ModFlow::recv (swarm::ev_id eid, const swarm::Property &p) {
    // Eliminate timeout flow, and re-put flow if it updated.
    if (this->last_ts_ == 0) {
      this->last_ts_ = p.tv_sec();
    } else if (this->last_ts_ < p.tv_sec()) {
      time_t diff = p.tv_sec() - this->last_ts_;
      debug(DBG, "tick: %u (%u)", p.tv_sec(), diff);
      this->last_ts_ = p.tv_sec();
      this->flow_table_.prog(diff);

      LRUHash::Node *node;
      while(NULL != (node = this->flow_table_.pop())) {
        Flow *flow = dynamic_cast<Flow*>(node);
        if (flow->remain() > 0) {
          debug(DBG, "updating [%016llX]", flow->hash());
          this->flow_table_.put(flow->remain(), flow);
        } else {
          debug(DBG, "deleting [%016llX]", flow->hash());          
          delete flow;
        }
      }
    }

    // 
    if (eid == this->ev_ipv4_ || eid == this->ev_ipv6_) {
      size_t keylen;
      const void *key = p.ssn_label(&keylen); 
      Flow *flow = dynamic_cast<Flow*>
        (this->flow_table_.get(p.hash_value(), key, keylen));
      if (flow == NULL) {
        // TODO: catch bad_alloc
        flow = new Flow(p);
        debug(DBG, "new flow [%016llX] %s->%s", p.hash_value(),
              p.src_addr().c_str(), p.dst_addr().c_str());
        this->flow_table_.put(this->flow_timeout_, flow);
      }
      flow->update(p.tv_sec()); 

    } else if (eid == this->ev_dns_) {

      // debug(true, "eid=%lld, dns", eid);
    } 
  }
  void ModFlow::exec (const struct timespec &ts) {
    
  }
  const std::vector<std::string>& ModFlow::recv_event() const {
    return ModFlow::recv_events_;
  }
  int ModFlow::task_interval() const {
    return 1;
  }

  // ------------------------------------------------------------
  // class ModFlow::Flow
  ModFlow::Flow::Flow(const swarm::Property &p) {
    this->hv_ = p.hash_value();
    const void *key = p.ssn_label(&this->keylen_);
    this->key_ = malloc(this->keylen_);
    memcpy(this->key_, key, this->keylen_);
    this->created_at_ = p.tv_sec();
    this->updated_at_ = p.tv_sec();
  }
  ModFlow::Flow::~Flow() {
    free(this->key_);
  }
  
}
