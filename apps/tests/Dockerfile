FROM python:3.12-slim
WORKDIR /home/tests

ARG USER_ID

# create non-root user for runtime
RUN groupadd -g 2523 Wywy-Website
RUN useradd -g Wywy-Website -u ${USER_ID} -m -d /home/tests tests

# populate files via volume
COPY --chown=tests:Wywy-Website /apps/tests/requirements.txt /home/tests/requirements.txt

# install required python packages
RUN apt-get update && apt-get install -y --no-install-recommends curl \
    && rm -rf /var/lib/apt/lists/*

# Download config.yml for CI (no runtime mount needed).
RUN curl -fsSL https://raw.githubusercontent.com/WywySenarios/Wywy-Website-Control/main/config/config.yml \
    -o /config.yml \
    || echo "config.yml download skipped (will use runtime mount in local dev)"
RUN pip install -r /home/tests/requirements.txt

USER tests

CMD ["python3", "-m", "unittest", "discover", "-s", "/home/wywy", "-t", "/home/wywy"]