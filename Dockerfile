FROM alpine:3.12
RUN apk update
RUN apk add build-base git cmake
COPY . /src/chess/
RUN mkdir -p /build
WORKDIR /build
RUN cmake /src/chess
RUN cmake --build .
