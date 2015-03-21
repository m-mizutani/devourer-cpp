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
- CMake
- Libraries
    - libev
    - libpcap
	- msgpack for C++
    - libfluent: https://github.com/m-mizutani/libfluent >= 1.0.7

Install
------

### Install libraries and tools

Install c++ compiler, libev, libpcap and msgpack for C++.

Example for Ubuntu 14.04.

```shell
% sudo apt-get install build-essential cmake libpcap-dev libpcap0.8 libev4 libev-dev libmsgpack3 libmsgpack-dev
```

### Clone libfluent and devourer, then build them.

```shell
% git clone https://github.com/m-mizutani/libfluent.git
% cd libfluent
% cmake . && make
% cd ..
% git clone https://github.com/m-mizutani/devourer.git
% cd devourer
% cmake -DWITH_FLUENT=../libfluent . && make
% sudo make install
```

Usage
------

To be wrtten

Output Format
------

### dns.tx

```ruby
{
  "client"=>"172.20.10.2",        # Client IP address
  "latency"=>0.05015707015991211, # Latency between the request and the response
  "q_name"=>"a568.d.akamai.net.", # Domain name of query
  "server"=>"172.20.10.1",        # DNS server IP address
  "status"=>"success"             # Status of the query
}
```



### dns.log



### flow.new

Emit the message with `flow.new` tag when new flow is observed.

```ruby
{
  "dst_addr"=>"173.194.126.xx",   # Destination IP address
  "dst_name"=>"www.example.com.", # Destination Domain Name
  "dst_port"=>443,                # Destination Port Number
  "hash"=>"448CAF052DA2CBDF",     # Flow (IP address, Port, Protocol) Hsah
  "proto"=>"TCP",                 # Protocol (TCP/UDP/ICMP)
  "src_addr"=>"172.20.10.2",      # Source IP address
  "src_name"=>nil,                # Source Domain Name (nil means not resolved)
  "src_port"=>50250               # Source Port Number
}
```

### flow.log

Emit the message with `flow.log` tag when the flow stored in cache is expired. Default time to live in cache is 600 seconds.

```ruby
{
  "dst_addr"=>"173.194.126.xx",   # Destination IP address
  "dst_name"=>"www.example.com.", # Destination Domain Name
  "dst_port"=>443,                # Destination Port Number
  "hash"=>"448CAF052DA2CBDF",     # Flow (IP address, Port, Protocol) Hsah
  "proto"=>"TCP",                 # Protocol (TCP/UDP/ICMP)
  "src_addr"=>"172.20.10.2",      # Source IP address
  "src_name"=>nil,                # Source Domain Name (nil means not resolved)
  "src_port"=>50250               # Source Port Number
}
```
