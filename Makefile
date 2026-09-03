CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
THREAD_FLAGS ?= -pthread

.PHONY: all run server sounds clean

all: nightshift nightshift-server

nightshift: src/main.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

nightshift-server: src/server.cpp
	$(CXX) $(CXXFLAGS) $(THREAD_FLAGS) $< -o $@

sounds:
	python3 tools/generate_audio.py

run: nightshift
	./nightshift

server: nightshift-server
	./nightshift-server --port 8080 --doors 20

clean:
	rm -f nightshift nightshift.exe nightshift-server nightshift-server.exe nightshift.save
