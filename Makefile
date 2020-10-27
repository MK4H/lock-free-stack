
all: ha1

ha1: ha1main.cpp ha1.hpp
	g++ -std=c++17 -O3 -Wall -Wextra -mcx16 -pthread -o ha1 ha1main.cpp -latomic