FROM ubuntu:16.04

RUN apt-get update && apt-get install -y sudo curl git python build-essential libssl-dev libboost-all-dev software-properties-common libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils psmisc nano
RUN sudo add-apt-repository ppa:bitcoin/bitcoin -y && apt-get update && apt-get install -y libdb4.8-dev libdb4.8++-dev

RUN curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.33.6/install.sh | bash
ENV NVM_DIR /root/.nvm
RUN . /root/.nvm/nvm.sh && nvm install v0.10.25

EXPOSE 3032
EXPOSE 3993
EXPOSE 3256

ADD start-simulation /usr/bin/start-simulation
ADD store_log_info /usr/bin/store_log_info
ADD spendbot /usr/bin/spendbot

ADD http-proxy /opt/http-proxy
RUN cd /opt/http-proxy && . /root/.nvm/nvm.sh && npm install

ADD stratum-server /opt/stratum-server
RUN git clone https://github.com/UNOMP/node-merged-pool.git /opt/stratum-server/node_modules/stratum-pool
RUN cd /opt/stratum-server && . /root/.nvm/nvm.sh && npm install

ADD litecoin.conf /root/.adcoin/adcoin.conf
ADD litecoin.conf /root/.adcoin/regtest2/adcoin.conf
ADD wallet_docker.dat /root/.adcoin/regtest/wallet.dat
RUN mkdir /opt/litecoin
ADD bin/litecoind /opt/litecoin
ADD bin/adcoin-cli /opt/litecoin
ENV PATH="/opt/litecoin:${PATH}"

CMD /usr/bin/start-simulation
