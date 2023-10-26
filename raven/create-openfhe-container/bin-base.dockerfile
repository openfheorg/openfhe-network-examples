FROM ubuntu:jammy

RUN mkdir -p /usr/local/lib 
COPY ./opt/include /usr/local/include
COPY ./opt/lib /usr/local/lib
RUN mkdir -p /usr/local/bin
COPY ./bin/ /usr/local/bin/

COPY ./opt/demoData /demoData
COPY ./opt/NetworkMaps /NetworkMaps
COPY ./opt/scripts /scripts

COPY ./x86_64-linux-gnu /lib/x86_64-linux-gnu/

ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
