
# Labyrinthine pattern formation model

This repository contains the C++ implementation of a physically-based numerical model to simulate the formation of labyrinthine patterns such as those seen in polar seal maxilloturbinates, frictional finger structures observed in granular fluid experiments, and the ferrofluid labyrinthine instability.

## Build

To build from the command line using G++:
```sh
$ make
```

Alternatively, the project can be built using MSVC by opening the [seal-gen.sln](seal-gen.sln) solution file in Visual Studio.

## Run

Display the help message to view arguments available:
```sh
$ ./seals help
```

Certain flags enable a different set of arguments (e.g. `-d3`); the help message is contextual, meaning these alternative arguments can be displayed using
```sh
$ ./seals help -d3
```

Sample arguments used to simulate seal maxilloturbinate growth, granular fluid frictional fingering patterns with the outward and inward models, and the ferrofluid labyrinthine instability can be found respectively in [all-seals.py](all-seals.py), [all-granular.py](all-granular.py), [all-granular-v2.py](all-granular-v2.py), and [all-ferro.py](all-ferro.py).
