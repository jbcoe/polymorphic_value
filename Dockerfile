FROM ubuntu:latest
RUN apt update ; apt install -y python3 cmake git ninja-build llvm lldb gdb g++
WORKDIR /development
COPY . .
