# Build the portable C++ engine foundation in a reproducible Linux container.
FROM ubuntu:24.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY engine/ ./engine/
RUN cmake -S engine -B /build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /build --target retroshelf-engine --parallel 2

# Small runtime image containing the engine executable only.
FROM ubuntu:24.04 AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends libstdc++6 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=build /build/retroshelf-engine /usr/local/bin/hush-engine
ENTRYPOINT ["/usr/local/bin/hush-engine"]
