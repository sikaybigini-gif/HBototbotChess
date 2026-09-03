CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

.PHONY: all run clean

all: nightshift

nightshift: src/main.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

run: nightshift
	./nightshift

clean:
	rm -f nightshift nightshift.exe nightshift.save
