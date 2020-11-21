CXX=clang++
CXXFLAGS=-std=c++17 -I.
DEPS = fetcher.hpp mock_downloader.hpp
OBJ = async_fetcher_tests.o fetcher.o

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

async_fetcher_tests: $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS)
	./async_fetcher_tests

.PHONY: clean

clean:
	rm *.o async_fetcher_tests
