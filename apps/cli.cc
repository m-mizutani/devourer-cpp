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

#include <vector>
#include "./devourer.h"
#include "./optparse.h"

int devourer_main(int argc, char *argv[]) {
  optparse::OptionParser psr = optparse::OptionParser();
  psr.add_option("-r").dest("read_file")
    .help("Pcap format file path");
  psr.add_option("-i").dest("interface")
    .help("Interface to monitor on the fly");
  psr.add_option("-f").dest("fluentd")
    .help("Fluentd destination, e.g. 127.0.0.1:24224");
  psr.add_option("-o").dest("output")
    .help("Log file path, stdout if '-'");
  psr.add_option("-t").dest("filter").set_default("")
    .help("Tag filter with regular expression (e.g. dns\\..*");
  psr.add_option("-v").dest("verbose")
    .help("Verbose mode");
  
  optparse::Values& opt = psr.parse_args(argc, argv);
  std::vector <std::string> args = psr.args();


  Devourer *devourer = NULL;
  if (opt.is_set("read_file")) {
    devourer = new Devourer(opt["read_file"], devourer::PCAP_FILE);
  } else if (opt.is_set("interface")) {
    devourer = new Devourer(opt["interface"], devourer::INTERFACE);
  }
    
  if (!devourer) {
    std::cerr << "One from file or interface should be specified" << std::endl;
    return false;
  }

  std::string filter = opt["filter"];
  try {
    if (opt.is_set("output")) {
      devourer->setdst_filestream(opt["output"]);
    }
    
    if (opt.is_set("fluentd")) {
      devourer->setdst_fluentd(opt["fluentd"]);
    }

    devourer->start();
  } catch (const devourer::Exception &e) {
    std::cerr << "Devourer Error: " << e.what() << std::endl;
    return false;
  } catch (std::exception *e) {
    std::cerr << "Devourer Error: " << e->what() << std::endl;
    return false;
  }

  return true;
}



int main (int argc, char *argv[]) {
  if (devourer_main(argc, argv)) {
    exit(EXIT_SUCCESS);
  } else {
    exit(EXIT_FAILURE);
  }
}

