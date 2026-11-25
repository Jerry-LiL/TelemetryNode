FROM ubuntu:24.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential cmake python3 python3-pytest python3-serial \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure

ENV PYTHONPATH=/src/python
EXPOSE 8765
CMD ["./build/telemetry_node", "--link", "tcp://0.0.0.0:8765"]
