CXX = clang++
CXXFLAGS = -O3 -march=native -std=c++23 -Wall -DCROW_ENABLE_DEBUG -DCROW_ENABLE_COMPRESSION
LDFLAGS = -lpthread -lz

# Targets
all: main

# Build the main executable
main: main.o ssg/parser.o ssg/renderer.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.cc 
	$(CXX) $(CXXFLAGS) -c $^ -o $@

clean:
	rm -f main $(PCH_OUT)
