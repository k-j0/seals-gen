
seals: *.cpp *.h
	g++ -fopenmp -O3 -m64 -o seals *.cpp -std=c++14
