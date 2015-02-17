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
#include "./swarm/swarm.h"
#include "./devourer.h"
#include "./object.h"
#include "./stream.h"
#include "./debug.h"

#include "./module.h"
#include "./modules/dns.h"
#include "./modules/flow.h"

namespace devourer {
  void Module::emit(const std::string &tag, object::Object *obj,
                    struct timeval *ts) {
    if (this->stream_) {
        
      if (ts) {
        this->stream_->emit(tag, obj, *ts);
      } else {
        struct timeval now_ts;
        gettimeofday(&now_ts, NULL);
        this->stream_->emit(tag, obj, now_ts);
      }
    }
  }
}

Devourer::Devourer(const std::string &target, devourer::Source src) :
  target_(target), src_(src), netcap_(NULL), stream_(NULL)
{
  this->netdec_ = new swarm::NetDec();
}

Devourer::~Devourer(){
  delete this->netcap_;
}

void Devourer::set_fluentd(const std::string &dst, const std::string &filter)
throw(devourer::Exception) {
  size_t pos;
  if(std::string::npos != (pos = dst.find(":", 0))) {
    std::string host = dst.substr(0, pos);
    std::string port = dst.substr(pos + 1);
    this->stream_ = new devourer::FluentdStream(host, port);
    this->stream_->set_filter(filter);
  } else {
    throw new devourer::Exception("Fluentd option format must be "
                                  "'hostname:port'");
  }
}

void Devourer::set_output(const std::string &fpath, const std::string &filter)
throw(devourer::Exception) {
  this->stream_ = new devourer::FileStream(fpath);
  this->stream_->set_filter(filter);
}

void Devourer::install_module(devourer::Module *module)
  throw(devourer::Exception) {
  assert(this->netcap_);
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

  int interval = module->task_interval();
  if (interval > 0) {
    swarm::task_id tid =
      this->netcap_->set_periodic_task(module, static_cast<float>(interval));
    if (tid == swarm::TASK_NULL) {
      throw devourer::Exception(this->netcap_->errmsg());
    }
  }

  module->set_stream(this->stream_);
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

  // Setup output stream.
  if (this->stream_) {
    this->stream_->setup();
  }

  
  devourer::ModDns *mod_dns = new devourer::ModDns();
  devourer::ModFlow *mod_flow = new devourer::ModFlow(mod_dns);
  this->install_module(mod_dns);
  this->install_module(mod_flow);

  this->netcap_->start();

  delete mod_dns;
  delete mod_flow;
  return;
}
