
OUT := seals
CC := g++
CFLAGS := -fopenmp -O3 -Wall -Wextra -Werror -std=c++17 -m64 -DNDEBUG

SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)

.PHONY: all clean

all: $(OUT)

$(OUT): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OUT)
	rm -f *.o
