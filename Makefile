all: main

main: main.cpp
	g++ main.cpp -o main -std=c++11 -lsdl2 -lSDL2 -lSDL2_ttf
