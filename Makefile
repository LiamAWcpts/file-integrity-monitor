CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LIBS = -lcrypto

SRC = src/main.cpp src/baseline.cpp src/hash.cpp

fim: $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o fim $(LIBS)

clean:
	rm -f fim
