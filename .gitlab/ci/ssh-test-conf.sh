#!/bin/sh
set -e

# Append own public key as authorized so we can ssh localhost
cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys

# Append ourself to the known hosts to avoid interactive prompt
ssh-keyscan -H localhost >> ~/.ssh/known_hosts
ssh-keyscan -H $HOSTNAME >> ~/.ssh/known_hosts
