#!/usr/bin/bash

# make
# ./main_static_network > 2mpxcp_out.txt
../parse_output starlink1.log -ascii > 2ss.txt
cat 2ss.txt | grep XCP_SINK > temp2ss.txt