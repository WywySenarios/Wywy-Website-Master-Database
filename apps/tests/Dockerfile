FROM python:3.12-slim
WORKDIR /home/tests

ARG USER_ID

# create non-root user for runtime
RUN groupadd -g 2523 Wywy-Website
RUN useradd -g Wywy-Website -u ${USER_ID} -m -d /home/tests tests

# populate files via volume
COPY --chown=tests:Wywy-Website /apps/tests/requirements.txt /home/tests/requirements.txt

# install required python packages
RUN apt-get update && apt-get install -y --no-install-recommends \
    && rm -rf /var/lib/apt/lists/*
RUN pip install -r /home/tests/requirements.txt

USER tests

CMD ["python3", "-m", "unittest", "discover", "-s", "/home/wywy", "-t", "/home/wywy"]