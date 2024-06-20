# SDP Parser

## Overview

This project parses SDP (Session Description Protocol) vectors to calculate the required RTP and RTCP bandwidth for bi-directional streams. It takes into account the RTP and RTCP bandwidth separately and calculates the total bandwidth in each direction.

## Run
```shell
$ cd build
$ cmake ..
$ make
$ ./sdp_parser ../sdp_vector
```

## Output
```shell
permit 49kbps from 2001:0:0:1::11 1234 to 2001::1:79bf:d746:a887:c550 49120
permit 3200bps from 2001:0:0:1::11 1234 to 2001::1:79bf:d746:a887:c550 49120
permit 38kbps from 2001:0:0:1::11 1234 to 2001::1:79bf:d746:a887:c550 49120
permit 2449bps from 2001:0:0:1::11 1234 to 2001::1:79bf:d746:a887:c550 49120
Total uplink from 2001:0:0:1::11 ---> 52.2 kbps
Total uplink from 2001::1:79bf:d746:a887:c550 ---> 40.4 kbps
```

