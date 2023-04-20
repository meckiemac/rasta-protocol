#!/bin/sh

sed -i -e "s/10.0.0.100/10.10.0.101/g" rasta_client1.cfg
sed -i -e "s/10.0.0.101/10.10.2.101/g" rasta_client1.cfg
