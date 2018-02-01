#!/bin/bash
mkdir -p $HOME/adcoin_regtest/1
mkdir -p $HOME/adcoin_regtest/2
src/qt/adcoin-qt -regtest -datadir=$HOME/adcoin_regtest/1 -rpcuser=test -rpcpassword=test -rest -port=3339 -rpcport=4443 -server -addnode=localhost:3338 &
src/qt/adcoin-qt -regtest -datadir=$HOME/adcoin_regtest/2 -rpcuser=test -rpcpassword=test -rest -port=3338 -rpcport=4442 -server -addnode=localhost:3339 &
echo "Run 'src/adcoin-cli -rpcport=4443 -rpcuser=test -rpcpassword=test <command>' for any commands"
echo "Will output logs of the second node (the one on RPC port 4442)..."
sleep 5
tail -f $HOME/adcoin_regtest/2/regtest/debug.log
