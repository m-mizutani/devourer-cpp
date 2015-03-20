Devourer: Know your traffic
========

Devourer is live packet capture and statistics tool for multipurpose.

- DNS
  - Query latency
  - Response loss rate
  - Response without query
- Flow
  - IP address, port, protocol
  - Resolved Domain Name
  - Transferred data size and packet count

Prerequisite
------

- OS: Linux >= 3.2.0, MacOSX >= 10.9.5
- C++11 required
- Libraries
  - libev
  - libpcap
  - libfluent: https://github.com/m-mizutani/libfluent

Install
------

1. Install libraries, libev and libpcap
2. Clone libfluent and devourer, then build them.

```shell
% git clone https://github.com/m-mizutani/libfluent.git
% cd libfluent
% cmake . && make
% cd ..
% git clone https://github.com/m-mizutani/devourer.git
% cd devourer
% cmake -DWITH_FLUENT=../libfluent . && make
```

Usage
------

To be wrtten

Output
------

To be wrtten


