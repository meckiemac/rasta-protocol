#!/bin/sh

sed -i -e "s/10.0.0.100/$SERVER_CH1/g" rasta_server.cfg
sed -i -e "s/10.0.0.101/$SERVER_CH2/g" rasta_server.cfg
