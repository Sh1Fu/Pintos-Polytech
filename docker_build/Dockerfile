FROM ubuntu:16.04
RUN apt-get update
RUN apt-get install gcc-multilib make perl qemu -y
RUN ln /usr/bin/qemu-system-x86_64 /usr/bin/qemu

COPY . .
RUN tar -zxf pintos.tar.gz
WORKDIR "pintos/src/utils"

RUN cp backtrace pintos pintos-mkdisk pintos-set-cmdline Pintos.pm pintos-gdb ../misc/gdb-macros /usr/bin/
RUN chmod 755 /usr/bin/backtrace /usr/bin/pintos /usr/bin/pintos-mkdisk /usr/bin/pintos-set-cmdline /usr/bin/pintos-gdb
RUN chmod 644 /usr/bin/Pintos.pm /usr/bin/gdb-macros

WORKDIR "../threads"
RUN ls
RUN make

# Test pintos
RUN pintos --qemu -- -q run alarm-single
