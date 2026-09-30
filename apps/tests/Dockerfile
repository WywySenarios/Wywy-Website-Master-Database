FROM python:3.12-slim
WORKDIR /home/tests

ARG USER_ID

# create non-root user for runtime
RUN groupadd -g 2523 Wywy-Website
RUN useradd -g Wywy-Website -u ${USER_ID} -m -d /home/tests tests

COPY --chown=tests:Wywy-Website /apps/tests/requirements.txt /home/tests/requirements.txt
RUN pip install -r /home/tests/requirements.txt
COPY --chown=tests:Wywy-Website /apps/tests /home/wywy

USER tests

CMD ["python3", "-m", "unittest", "discover", "-s", "/home/wywy", "-t", "/home/wywy"]