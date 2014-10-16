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

#include <swarm.h>
#include "./devourer.h"

namespace devourer {

  Proc::Proc(swarm::Swarm *sw) : sw_(sw) {
    // ev_dns_  = this->sw_lookup_event_id("dns.packet");
  }
  Proc::~Proc() {
  }


  FileStream::FileStream(const std::string &fpath) {
  }
  FileStream::~FileStream() {
  }
  void FileStream::write(const msgpack::sbuffer &sbuf) throw(Exception) {
  }

  FluentdStream::FluentdStream(const std::string &host, int port) {
  }
  FluentdStream::~FluentdStream() {
  }
  void FluentdStream::write(const msgpack::sbuffer &sbuf) throw(Exception) {
  }

  void Proc::exec (const struct timespec &ts) {
    printf("yes!yes!yes!\n");
  }  

  void Proc::load_plugin(Plugin *plugin) {
    
  }

  void Plugin::write_stream(const msgpack::sbuffer &sbuf) {
    if (this->stream_) {
      this->stream_->write(sbuf);
    }
  }


  const std::string DnsTx::recv_event_ = "dns.packet";

  DnsTx::DnsTx() {
  }
  DnsTx::~DnsTx() {
  }
  void DnsTx::recv (swarm::ev_id eid, const  swarm::Property &p) {
  }
  const std::string& DnsTx::recv_event() const {
    return DnsTx::recv_event_;
  }

                        
}

Devourer::Devourer(const std::string &target, devourer::Source src) :
  target_(target), src_(src), sw_(NULL)
{
}

Devourer::~Devourer(){
  delete this->sw_;
}

void Devourer::start() throw(devourer::Exception) {
  switch(this->src_) {
  case devourer::PCAP_FILE: this->sw_ = new swarm::SwarmFile(this->target_); break;
  case devourer::INTERFACE: this->sw_ = new swarm::SwarmDev(this->target_); break;
  }

  if (!this->sw_) {
    throw new devourer::Exception("Fatal error");
  }

  if (!this->sw_->ready()) {
    throw new devourer::Exception(this->sw_->errmsg());
  }

  devourer::Proc *proc = new devourer::Proc(this->sw_);
  proc->load_plugin(new devourer::DnsTx());

  this->sw_->set_periodic_task(proc, 1.);
  this->sw_->start();

  return;
}
