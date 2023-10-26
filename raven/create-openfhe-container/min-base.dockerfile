FROM ubuntu:jammy

RUN mkdir -p /opt
COPY opt/include /opt/include
COPY opt/lib /opt/lib
COPY opt/bin /opt/bin
COPY opt/share /opt/share
