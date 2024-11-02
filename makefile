CXX = clang++
CXXFLAGS = -Wall -Wextra -o3 -std=c++23
VALGRIND = valgrind

SRC_DLIBS = lib.cpp
SRC_MAIN = main.cpp

HDR_DLIBS = $(SRC_DLIBS:.cpp=.hpp)

all: dlibs main

main: $(SRC_MAIN)
	$(CXX) $(CXXFLAGS) -o $@ $<
	$(VALGRIND) ./main

dlibs: $(SRC_DLIBS:.cpp=.so)

# .so pattern
%.so: %.cpp $(HDR_DLIBS)
	$(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $<

clean:
	rm -f main

clean_libs:
	rm -f $(SRC_DLIBS:.cpp=.so)
