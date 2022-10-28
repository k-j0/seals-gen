
OUT := seals
CC := g++
CFLAGS_CORE := -fopenmp -O3 -Wall -Wextra -Werror -fmax-errors=4
CFLAGS_CORE_CL := /openmp /O2
CFLAGS_EXTRA := -O3 -std=c++17 -m64 -DNDEBUG
WITH_CUDA := 1

SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
CFLAGS := $(CFLAGS_CORE) $(CFLAGS_EXTRA)

# turn off Cuda support if NVCC isn't installed
ifeq ($(shell which nvcc),)
  ifeq ($(WITH_CUDA),1)
    WITH_CUDA := 0
    $(warning Turning off CUDA support because which nvcc returned ''.)
  endif
endif

# if Cuda is on, build with nvcc and with the relevant compiler flags
ifeq ($(WITH_CUDA),1)
  CC = nvcc
  SOURCES_CUDA := $(wildcard *.cu)
  ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
    # cl.exe compiler flags & objects instead of gcc
    CFLAGS_CORE = $(CFLAGS_CORE_CL)
	OBJECTS = $(SOURCES:.cpp=.obj) $(SOURCES_CUDA:.cu=.obj)
  else
    OBJECTS += $(SOURCES_CUDA:.cu=.o)
  endif
  OBJECTS += $(OBJECT_CUDA)
  CFLAGS = -Xcompiler="$(CFLAGS_CORE)" $(CFLAGS_EXTRA) -Werror=all-warnings -DCUDA
endif

.PHONY: all clean

all: $(OUT)

$(OUT): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

# gcc objects
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.cu
	$(CC) $(CFLAGS) -c $< -o $@

# cl.exe objects
%.obj: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@
%.obj: %.cu
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OUT)
	rm -f *.o
	rm -f *.obj
	rm -f *.exp
	rm -f *.lib
