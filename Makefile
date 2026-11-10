# BioFix Makefile
# Build both:   make
# Run CLI:      make run     (runs inside data/ so the program finds the .txt files)
# Run website:  make web     (then open http://localhost:8080)
# Benchmark:    make bench   (hash table vs list, heap vs sorted list)
# Queue test:   make fuzz    (thousands of random operations on the heap)
# Clean:        make clean

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
# -MMD -MP: remember which .h files each .cpp uses, so changing a header rebuilds it
DEPFLAGS = -MMD -MP

# Shared code: everything in src/ except the CLI's main.cpp
CORE_SRC = $(filter-out src/main.cpp,$(wildcard src/*.cpp))
CORE_OBJ = $(patsubst src/%.cpp,build/%.o,$(CORE_SRC))

WEB_SRC  = $(wildcard web/*.cpp)
WEB_OBJ  = $(patsubst web/%.cpp,build/web_%.o,$(WEB_SRC))

# Windows needs the Winsock library for the website
ifeq ($(OS),Windows_NT)
    WEB_LIBS = -lws2_32
endif

CLI    = build/BioFix
WEBAPP = build/BioFixWeb

all: $(CLI) $(WEBAPP)

$(CLI): $(CORE_OBJ) build/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

$(WEBAPP): $(CORE_OBJ) $(WEB_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(WEB_LIBS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

build/web_%.o: web/%.cpp | build
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

build:
	mkdir -p build

BENCH = build/benchmark

$(BENCH): $(CORE_OBJ) build/bench_benchmark.o
	$(CXX) $(CXXFLAGS) -O2 -o $@ $^

build/bench_%.o: bench/%.cpp | build
	$(CXX) $(CXXFLAGS) -O2 $(DEPFLAGS) -c $< -o $@

bench: $(BENCH)
	./$(BENCH)

FUZZ = build/queue_fuzz

$(FUZZ): tests/queue_fuzz.cpp build/InputHelper.o | build
	$(CXX) $(CXXFLAGS) -O1 -o $@ $^

fuzz: $(FUZZ)
	./$(FUZZ)

run: $(CLI)
	cd data && ../$(CLI)

web: $(WEBAPP)
	./$(WEBAPP)

clean:
	rm -rf build

-include $(wildcard build/*.d)

.PHONY: all run web bench fuzz clean
