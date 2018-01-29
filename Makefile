all: puzzle

puzzle: puzzle.cpp
	g++ -o puzzle puzzle.cpp `pkg-config opencv --cflags` `pkg-config opencv --libs` -fopenmp -std=c++14
