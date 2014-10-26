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

#include "./object.h"

namespace devourer {
  namespace object {
    Map::Map() {
    }
    Map::~Map() {
      for(auto it = this->map_.begin(); it != this->map_.end(); it++) {
        delete it->second;
      }
    }

    void Map::overwrite(const std::string &key, Object *obj) {
      auto it = this->map_.find(key);
      if (it != this->map_.end()) {
        delete it->second;
        it->second = obj;
      } else {
        this->map_.insert(std::make_pair(key, obj));
      }
    }

    void Map::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const {
      pk->pack_map(this->map_.size());
      for(auto it = this->map_.begin(); it != this->map_.end(); it++) {
        pk->pack(it->first);
        (it->second)->to_msgpack(pk);
      }
    }

    void Map::put(const std::string &key, const std::string &val) {
      Object *obj = new String(val);
      this->overwrite(key, obj);
    }
    void Map::put(const std::string &key, int64_t val) {
      Object *obj = new Fixnum(val);
      this->overwrite(key, obj);
    }
    void Map::put(const std::string &key, double val) {
      Object *obj = new Float(val);
      this->overwrite(key, obj);
    }
    void Map::put(const std::string &key, Object *obj) {
      this->overwrite(key, obj);
    }

    void Fixnum::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const {
      pk->pack(this->val_);
    }
    void Float::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const {
      pk->pack(this->val_);
    }
    void String::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const {
      pk->pack(this->val_);
    }
  }
}
