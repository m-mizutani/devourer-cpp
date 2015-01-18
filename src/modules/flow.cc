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
#include "../swarm/swarm.h"
#include "../devourer.h"
#include "../object.h"
#include "../stream.h"
#include "../debug.h"
#include "./dns.h"

namespace devourer {
  
  const std::vector<std::string> ModFlow::recv_events_{
    "ipv4.packet",
    "ipv6.packet",
  };
  const bool ModFlow::DBG = true;
  
  // ------------------------------------------------------------
  // class ModFlow
  ModFlow::ModFlow(ModDns *mod_dns) :
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
  void ModFlow::bind_event_id(const std::string &ev_name, swarm::ev_id eid) {
    if (ev_name == "ipv4.packet") {
      this->ev_ipv4_ = eid;
    } else if (ev_name == "ipv6.packet") {
      this->ev_ipv6_ = eid;
    }
  }
  
  void ModFlow::recv (swarm::ev_id eid, const swarm::Property &p) {
    static const bool FLOW_DBG = false;

    // Get packet time.
    struct timeval tv;
    p.tv(&tv);

    // Eliminate timeout flow, and re-put flow if it updated.
    if (this->last_ts_ == 0) {
      this->last_ts_ = p.tv_sec();
    } else if (this->last_ts_ < p.tv_sec()) {
      time_t diff = p.tv_sec() - this->last_ts_;
      // debug(FLOW_DBG, "tick: %ld (%ld)", p.tv_sec(), diff);
      this->last_ts_ = p.tv_sec();
      this->flow_table_.prog(diff);

      LRUHash::Node *node;
      while(NULL != (node = this->flow_table_.pop())) {
        Flow *flow = dynamic_cast<Flow*>(node);
        if (flow->remain() > 0) {
          // debug(FLOW_DBG, "updating [%016llX]", flow->hash());
          this->flow_table_.put(flow->remain(), flow);
          flow->refresh(p.tv_sec());
        } else {
          debug(FLOW_DBG, "deleting [%016llX]", flow->hash());

          object::Map *msg = new object::Map();
          flow->build_message(msg);
          struct timeval tv;
          flow->created_at(&tv);
          this->emit("flow.log", msg, &tv);
          delete flow;
        }
      }
    }

    
    // IPv4/IPv6 packets
    if (eid == this->ev_ipv4_ || eid == this->ev_ipv6_) {
      size_t keylen;
      const void *key = p.ssn_label(&keylen); 
      Flow *flow = dynamic_cast<Flow*>
        (this->flow_table_.get(p.hash_value(), key, keylen));
      if (flow == NULL) {
        // TODO: catch bad_alloc
        size_t src_len, dst_len;
        const void *src_addr = p.src_addr(&src_len);
        const void *dst_addr = p.dst_addr(&dst_len);
        const std::string &src =
          this->mod_dns_->resolv_addr(src_addr, src_len);
        const std::string &dst =
          this->mod_dns_->resolv_addr(dst_addr, dst_len);

        flow = new Flow(p, src, dst);

        object::Map *msg = new object::Map();
        msg->put("src_addr", p.src_addr());
        msg->put("dst_addr", p.dst_addr());
        if (!src.empty()) {
          msg->put("src_name", src);
        }
        if (!dst.empty()) {
          msg->put("dst_name", dst);
        }
        msg->put("proto", p.proto());
        if (p.has_port()) {
          msg->put("src_port", p.src_port());
          msg->put("dst_port", p.dst_port());
        }
        debug(FLOW_DBG, "new flow %s(%s)->%s(%s)",
              // p.hash_value(),
              p.src_addr().c_str(), src.c_str(), 
              p.dst_addr().c_str(), dst.c_str());
        this->emit("flow.new", msg, &tv);
        this->flow_table_.put(this->flow_timeout_, flow);
      }

      flow->update(p);

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
  ModFlow::Flow::Flow(const swarm::Property &p, const std::string& src,
                      const std::string& dst) :
    l_port_(0), r_port_(0),
    l_pkt_(0),  r_pkt_(0),
    l_size_(0), r_size_(0)
  {
    this->hv_ = p.hash_value();
    const void *key = p.ssn_label(&this->keylen_);
    this->key_ = malloc(this->keylen_);
    memcpy(this->key_, key, this->keylen_);

    this->created_at_ = p.tv_sec();
    this->updated_at_ = p.tv_sec();
    this->refreshed_at_ = p.tv_sec();
    
    this->init_dir_ = p.dir();
    if (this->init_dir_ == swarm::FlowDir::DIR_L2R) {
      // Left to Right
      this->l_addr_ = p.src_addr(); this->r_addr_ = p.dst_addr();
      this->l_port_ = p.src_port(); this->r_port_ = p.dst_port();
      this->l_name_ = src;          this->r_name_ = dst;
    } else if (this->init_dir_ == swarm::FlowDir::DIR_R2L) {
      // Right to Left
      this->r_addr_ = p.src_addr(); this->l_addr_ = p.dst_addr();
      this->r_port_ = p.src_port(); this->l_port_ = p.dst_port();
      this->r_name_ = src;          this->l_name_ = dst;
    }
    this->proto_ = p.proto();
  }

  ModFlow::Flow::~Flow() {
    free(this->key_);
  }
  
  void ModFlow::Flow::update(const swarm::Property &p) {
    this->updated_at_ = p.tv_sec();
    if (p.dir() == swarm::FlowDir::DIR_L2R) {
      this->l_pkt_  += 1;
      this->l_size_ += p.len();
    } else if (p.dir() == swarm::FlowDir::DIR_R2L) {
      this->r_pkt_  += 1;
      this->r_size_ += p.len();
    }
  }

  void ModFlow::Flow::build_message(object::Map *msg) {
    msg->put("l_addr", this->l_addr_);
    msg->put("r_addr", this->r_addr_);
    msg->put("l_port", this->l_port_);
    msg->put("r_port", this->r_port_);
    msg->put("proto",  this->proto_);
    msg->put("l_size", this->l_size_);
    msg->put("r_size", this->r_size_);
    msg->put("l_pkt",  this->l_pkt_);
    msg->put("r_pkt",  this->r_pkt_);
    msg->put("init_ts", static_cast<int64_t>(this->created_at_));
    msg->put("last_ts", static_cast<int64_t>(this->updated_at_));
      
      
    if (!this->l_name_.empty()) {
      msg->put("l_name", this->l_name_);
    }
    if (!this->r_name_.empty()) {
      msg->put("r_name", this->r_name_);
    }
    switch (this->init_dir_) {
      case swarm::FlowDir::DIR_L2R: msg->put("init", "l"); break;
      case swarm::FlowDir::DIR_R2L: msg->put("init", "r"); break;
      case swarm::FlowDir::DIR_NIL: break; // nothing to do
    }
  }

  
}
