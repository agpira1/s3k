FROM docker.io/hakarlsson/riscv-picolibc

USER root

ADD https://archive.ubuntu.com/ubuntu/pool/main/m/mtools/mtools_4.0.43-1build1_amd64.deb /tmp/mtools.deb

RUN dpkg -i /tmp/mtools.deb \
    && rm -f /tmp/mtools.deb
