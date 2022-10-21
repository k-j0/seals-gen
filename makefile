
OUT := seals
CC := g++
CFLAGS := -fopenmp -O3 -Wall -Wextra -Werror -std=c++17 -m64 -DNDEBUG
WITH_CUDA := 1
CC_CUDA := nvcc
CFLAGS_CUDA := -O3 -Wall -Wextra -Werror -std=c++17 -m64 -DNDEBUG

SOURCES := $(wildcard *.cpp)
SOURCES_CUDA := $(wildcard *.cu)
OBJECTS := $(SOURCES:.cpp=.o)

# on Cygwin, always turn off Cuda build
ifeq ($(findstring CYGWIN,$(shell uname)), CYGWIN)
	WITH_CUDA := 0
endif

# turn off Cuda support if NVCC isn't installed
ifeq ($(shell which nvcc),)
	WITH_CUDA := 0
endif

# if Cuda is on, build .cu files into the executable
ifeq ($(WITH_CUDA),1)
	CFLAGS += -DCUDA
	CFLAGS_CUDA += -DCUDA
	OBJECTS += $(SOURCES_CUDA:.cu=.co)
endif

.PHONY: all clean

all: $(OUT)

$(OUT): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.co: %.cu
	$(CC_CUDA) $(CFLAGS_CUDA) -c $< -o $@

clean:
	rm -f $(OUT)
	rm -f *.o
	rm -f *.co
