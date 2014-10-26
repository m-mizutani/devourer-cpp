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

#ifndef SRC_OBJECT_H__
#define SRC_OBJECT_H__

#include <msgpack.hpp>

namespace devourer {
  namespace object {
    class Exception : public std::exception {
    private:
      std::string errmsg_;
    public:
      Exception(const std::string &errmsg) : errmsg_(errmsg) {}
      ~Exception() {}
      virtual const char* what() const throw() { return this->errmsg_.c_str(); }
    };

    class Object {
    public:
      Object() {}
      virtual ~Object() {}
      virtual void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const = 0;
    };

    class Map : public Object {
    private:
      std::map<std::string, Object *> map_;
      void overwrite(const std::string &key, Object *obj);

    public:
      Map();
      ~Map();
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
      void put(const std::string &key, const std::string &val);
      void put(const std::string &key, int64_t val);
      void put(const std::string &key, double val);
      void put(const std::string &key, Object *obj);
    };

    class Fixnum : public Object {
    private:
      int64_t val_;
    public:
      Fixnum(int64_t val) : val_(val) {}
      ~Fixnum() {}
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
    };

    class Float : public Object {
    private:
      double val_;
    public:
      Float(double val) : val_(val) {}
      ~Float() {}
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
    };

    class String : public Object {
    private:
      std::string val_;
    public:
      String(const std::string &val) : val_(val) {}
      ~String() {}
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
    };
  }
}

#endif   // SRC_OBJECT_H__
