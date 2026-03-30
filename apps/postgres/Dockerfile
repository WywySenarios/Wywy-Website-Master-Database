# FROM debian:bookworm AS build

# Remove both the sources file and the debian.list file duplicates
# RUN rm -f /etc/apt/sources.list.d/debian.list /etc/apt/sources.list.d/debian.sources
# add repository entry & install
# RUN echo "deb http://deb.debian.org/debian/ bookworm main contrib non-free" > /etc/apt/sources.list.d/debian.list
#RUN apt-get update && apt-get install -y lftp
# RUN apt-get update
# RUN apt-get install curl -y

FROM postgres:18 AS db
WORKDIR /home/postgres

USER root

RUN groupadd -g 2523 Wywy-Website
RUN usermod -aG Wywy-Website postgres

RUN mkdir -p /home/postgres
RUN chmod 700 /home/postgres
RUN chown postgres /home/postgres

RUN apt-get update && \
    apt-get install -y openssh-client \
    postgis && \
    rm -rf /var/lib/apt/lists/*

COPY --chown=postgres:Wywy-Website /apps/postgres/backup.sh /backup.sh
COPY --chown=postgres:Wywy-Website /apps/postgres/entrypoint.sh /home/postgres/entrypoint.sh

CMD ["/home/postgres/entrypoint.sh"]

# USER postgres