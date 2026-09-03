CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

.PHONY: all run sounds clean

all: nightshift

nightshift: src/main.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

sounds:
	python3 tools/generate_audio.py

run: nightshift
	./nightshift

clean:
	rm -f nightshift nightshift.exe nightshift.save
