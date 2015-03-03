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

#include <iostream>
#include <fluent.hpp>
#include "./swarm/swarm.h"
#include "./devourer.h"
#include "./debug.h"

#include "./module.h"
#include "./modules/dns.h"
#include "./modules/flow.h"

Devourer::Devourer(const std::string &target, devourer::Source src) :
  target_(target), src_(src), netcap_(NULL), logger_(new fluent::Logger())
{
  this->netdec_ = new swarm::NetDec();
  
  devourer::ModDns *mod_dns = new devourer::ModDns();
  devourer::ModFlow *mod_flow = new devourer::ModFlow(mod_dns);
  this->install_module(mod_dns);
  this->install_module(mod_flow);
}

Devourer::~Devourer(){
  delete this->netcap_;
  for(size_t i = 0; i < this->modules_.size(); i++) {
    delete this->modules_[i];
  }
}

void Devourer::setdst_fluentd(const std::string &dst) {
  size_t pos;
  if(std::string::npos == (pos = dst.find(":", 0))) {
    // Specify only host, use default port
    this->logger_->new_forward(dst);
  } else {
    std::string host = dst.substr(0, pos);
    std::string str_port = dst.substr(pos + 1);

    char *e;
    unsigned int port = strtoul(str_port.c_str(), &e, 0);
    if (*e != '\0') {
      throw devourer::Exception("Invalid port number: " + str_port);
    }
    
    this->logger_->new_forward(host, port);
  }
}

void Devourer::setdst_filestream(const std::string &fpath) {
  this->logger_->new_dumpfile(fpath);
}

fluent::MsgQueue* Devourer::setdst_msgqueue() {
  return this->logger_->new_msgqueue();
}


void Devourer::install_module(devourer::Module *module)
  throw(devourer::Exception) {
  assert(this->netdec_);
  
  const std::vector<std::string> &ev_set = module->recv_event();
  for(size_t i = 0; i < ev_set.size(); i++) {
    swarm::ev_id eid = this->netdec_->lookup_event_id(ev_set[i]);
    swarm::hdlr_id hid = this->netdec_->set_handler(eid, module);
    if (hid == swarm::HDLR_NULL) {
      throw devourer::Exception(this->netdec_->errmsg());
    }
    module->bind_event_id(ev_set[i], eid);
  }

  this->modules_.push_back(module);
}

void Devourer::start() throw(devourer::Exception) {
  // Create a new Swarm instance.
  switch(this->src_) {
  case devourer::PCAP_FILE:
    this->netcap_ = new swarm::CapPcapFile(this->target_);
    break;
  case devourer::INTERFACE:
    this->netcap_ = new swarm::CapPcapDev(this->target_);
    break;
  }

  if (!this->netcap_) {
    throw devourer::Exception("Fatal error");
  }
  if (!this->netcap_->ready()) {
    throw devourer::Exception(this->netcap_->errmsg());
  }

  this->netcap_->bind_netdec(this->netdec_);

  // Setup modules
  for(size_t i = 0; i < this->modules_.size(); i++) {
    devourer::Module *module = this->modules_[i];
    int interval = module->task_interval();
    if (interval > 0) {
      swarm::task_id tid =
        this->netcap_->set_periodic_task(module,
                                         static_cast<float>(interval));
      if (tid == swarm::TASK_NULL) {
        throw devourer::Exception(this->netcap_->errmsg());
      }
    }
    module->set_logger(this->logger_);
  }    
  
  this->netcap_->start();
  return;
}


bool Devourer::input (const uint8_t *data, const size_t len,
                      const struct timeval &tv, const size_t cap_len) {
  return this->netdec_->input(data, len, tv, cap_len);
}
