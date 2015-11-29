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

#include "./local.h"

namespace devourer {
  const std::vector<std::string> ModLocal::recv_events_{
    "arp.request",
    "mdns.packet",
  };
  enum EvItemArray {
    ARP_REQUEST = 0,
    MDNS_PACKET,
  };
  
  ModLocal::ModLocal() {
    this->recv_events_id_.resize(this->recv_events_.size(), 0);
  }
  
  ModLocal::~ModLocal() {
  }
  
  void ModLocal::recv(swarm::ev_id eid, const swarm::Property &p) {
    if (eid == this->recv_events_id_[ARP_REQUEST]) {
      fluent::Message *msg = this->logger_->retain_message("arp.request");
      msg->set_ts(p.tv_sec());
      msg->set("src_hw", p.value("arp.src_hw").repr());
      msg->set("dst_hw", p.value("arp.dst_hw").repr());
      msg->set("src_pr", p.value("arp.src_pr").repr());
      msg->set("dst_pr", p.value("arp.dst_pr").repr());
      this->logger_->emit(msg);
    }
    
    if (eid == this->recv_events_id_[MDNS_PACKET]) {
      size_t answer_num = p.value_size("mdns.an_type");

      if (answer_num > 0) {
        fluent::Message *msg = this->logger_->retain_message("mdns.answer");
        msg->set_ts(p.tv_sec());
        fluent::Message::Array *arr = msg->retain_array("answer");

        for (size_t i = 0; i < answer_num; i++) {
          fluent::Message::Map *m = arr->retain_map();
          m->set("type", p.value("mdns.an_type", i).repr());
          m->set("name", p.value("mdns.an_name", i).repr());
          m->set("data", p.value("mdns.an_data", i).repr());
        }
        
        this->logger_->emit(msg);
      }
    }
  }
  
  void ModLocal::exec(const struct timespec &ts) {
    // TODO
  }
  
  const std::vector<std::string>& ModLocal::recv_event() const {
    return this->recv_events_;
  }
  
  int ModLocal::task_interval() const {
    return 1;
  }
  void ModLocal::bind_event_id(const std::string &ev_name, swarm::ev_id eid) {
    
    for (size_t i = 0; i < this->recv_events_.size(); i++) {
      if (this->recv_events_[i] == ev_name) {
        this->recv_events_id_[i] = eid;
        break;
      }
    }
  }

}