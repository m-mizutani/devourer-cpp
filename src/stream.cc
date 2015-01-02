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

#include <sys/stat.h> 
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "./stream.h"
#include "./debug.h"

namespace devourer {
  FileStream::FileStream(const std::string &fpath) : fpath_(fpath) {
  }
  FileStream::~FileStream() {
  }
  void FileStream::setup() {
    if (this->fpath_ == "-") {
      // Output to stdout
      this->fd_ = 1;
    } else {
      // Output to a file
      this->fd_ = ::open(this->fpath_.c_str(), O_WRONLY|O_CREAT, 0644);
      if (this->fd_ < 0) {
        std::string err(strerror(errno));
        throw Exception("FileStream Error: " + err);
      }
    }
  }
  void FileStream::emit(const std::string &tag, object::Object *obj, 
                        const struct timeval &ts) throw(Exception) {
    msgpack::sbuffer buf;
    msgpack::packer <msgpack::sbuffer> pk (&buf);
    
    object::Array arr;
    arr.push(tag);
    arr.push(static_cast<int64_t>(ts.tv_sec));
    arr.push(obj);
    arr.to_msgpack(&pk);
    
    ::write(this->fd_, buf.data(), buf.size());
  }


  const bool FluentdStream::DBG = false;

  FluentdStream::FluentdStream(const std::string &host, 
                               const std::string &port) : 
    host_(host), port_(port) {
  }
  FluentdStream::~FluentdStream() {
  }
  void FluentdStream::setup() {
    debug(DBG, "host=%s, port=%s", this->host_.c_str(), this->port_.c_str());

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    
    int r;
    if (0 != (r = getaddrinfo(this->host_.c_str(), this->port_.c_str(), 
                              &hints, &result))) {
      std::string errmsg(gai_strerror(r));
      throw Exception("getaddrinfo error: " + errmsg);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
      this->sock_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (this->sock_ == -1) {
        continue;
      }

      if (connect(this->sock_, rp->ai_addr, rp->ai_addrlen) != -1) {
        char buf[INET6_ADDRSTRLEN];
        struct sockaddr_in *addr_in = (struct sockaddr_in *) rp->ai_addr;
        inet_ntop(rp->ai_family, &addr_in->sin_addr.s_addr, buf, 
                  rp->ai_addrlen);

        debug(DBG, "connected to %s", buf);
        break;
      }
    }

    if (rp == NULL) {
      throw Exception("no avaiable address for " + this->host_);
    }
    freeaddrinfo(result);

  }
  void FluentdStream::emit(const std::string &tag, object::Object *obj,
                           const struct timeval &ts) throw(Exception) {
    msgpack::sbuffer buf;
    msgpack::packer <msgpack::sbuffer> pk (&buf);
    object::Array arr;
    object::Array *msg_set = new object::Array();
    object::Array *msg = new object::Array();
    std::string tag_prefix("devourer.");

    arr.push(tag_prefix + tag);
    arr.push(msg_set);
    msg_set->push(msg);
    msg->push(static_cast<int64_t>(ts.tv_sec));
    msg->push(obj);

    arr.to_msgpack(&pk);
    ::write(this->sock_, buf.data(), buf.size());
  }
}
