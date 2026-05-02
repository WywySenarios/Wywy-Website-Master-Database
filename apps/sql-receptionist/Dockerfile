# fetch necessary libraries
FROM gcc:15-trixie AS fetcher

# install packages needed for fetch
RUN apt-get update && apt-get install -y \
    wget \
    bzip2

# @TODO verification with gnupg

RUN wget https://github.com/akheron/jansson/releases/download/v2.14.1/jansson-2.14.1.tar.bz2 \
    && bunzip2 -c jansson-2.14.1.tar.bz2 > /tmp/jansson.tar
RUN wget -O /tmp/libcyaml.tar.gz https://github.com/tlsa/libcyaml/archive/refs/tags/v1.4.2.tar.gz

# compilation stage
FROM gcc:15-trixie AS builder

ARG USER_ID
ARG LIBCYAML_VARIANT

ENV LD_LIBRARY_PATH=/usr/local/lib

# create non-root user for runtime
RUN groupadd -g 2523 Wywy-Website
RUN useradd -g Wywy-Website -u ${USER_ID} -m -d /home/sql-receptionist sql-receptionist

# compile Jansson library
COPY --from=fetcher /tmp/jansson.tar /tmp/jansson.tar
RUN mkdir -p /tmp/jansson
RUN tar -xf /tmp/jansson.tar -C /tmp/jansson
WORKDIR /tmp/jansson/jansson-2.14.1
RUN ./configure
RUN make
RUN make check
RUN make install

# compile libcyaml library
COPY --from=fetcher /tmp/libcyaml.tar.gz /tmp/libcyaml.tar.gz
RUN mkdir -p /tmp/libcyaml
RUN tar -xzf /tmp/libcyaml.tar.gz -C /tmp/libcyaml
WORKDIR /tmp/libcyaml/libcyaml-1.4.2
RUN make
RUN make install VARIANT=${LIBCYAML_VARIANT}

RUN apt-get update && apt-get install -y \
    libpq-dev \
    libyaml-dev

WORKDIR /home/sql-receptionist

EXPOSE 2523

FROM builder AS dev

RUN apt-get install -y \
    valgrind \
    gdb \
    nano

COPY --chown=sql-receptionist:Wywy-Website /apps/sql-receptionist /home/sql-receptionist

USER sql-receptionist

FROM builder AS prod

COPY --chown=sql-receptionist:Wywy-Website /apps/sql-receptionist /home/sql-receptionist

RUN make clean
RUN make

USER sql-receptionist