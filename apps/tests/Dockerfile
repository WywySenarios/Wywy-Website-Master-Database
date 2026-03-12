FROM python:3.15.0a7-slim-trixie
WORKDIR /apps/tests

# populate files via volume
COPY ./apps/tests .

# install required python packages
RUN apt-get update && apt-get install -y --no-install-recommends \
    && rm -rf /var/lib/apt/lists/*
RUN pip install -r requirements.txt

CMD ["python3", "main.py"]