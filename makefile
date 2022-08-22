
seals: *.cpp *.h
	g++ -fopenmp -O3 -m64 -Wall -o seals *.cpp -std=c++14
