# Storm
C++ UCI Chess engine inspired by [Stockfish](https://stockfishchess.org/).
Created using ideas learned during the development of [Boxfish](https://github.com/Totomosic/Boxfish).

Example of the engine running [here](https://totomosic.github.io). (built to WebAssembly with Emscripten - slightly older version)

Storm currently uses the same NNUE as Stockfish (future plans to train network independently).
For now you should download a network from [https://tests.stockfishchess.org/nns](https://tests.stockfishchess.org/nns) and copy it into the working directory
before you run the program.

## Features
- Bitboards and magic bitboard move generation
- UCI protocol
- Search:
  - Transposition table with Zobrist hashing
  - PVS search
  - Aspiration windows and iterative deepening
  - Razoring
  - Adaptive null move pruning
  - Futility pruning
  - Late move reduction
- Evaluation:
  - NNUE
- All perft tests passed
- Pondering
- SEE move ordering

## Installing:
1. Download or clone this repository (use flag `--recurse-submodules` or `--recursive` to include submodules).
2. If on windows run the `Scripts/Win-GenProjects.bat` script to generate the Visual Studio 2019 project and solution files.
3. For best performance, download a compatible NNUE from [here](https://tests.stockfishchess.org/nns) and copy into working directory. Working network [nn-76a8a7ffb820.nnue](https://tests.stockfishchess.org/api/nn/nn-76a8a7ffb820.nnue). Storm will default to a classical evaluation if no valid network is found.

## Building on Windows:
1. Run `Scripts/Win-GenProjects.bat` and build the solution using Visual Studio 2019.
2. Build outputs are located in the `bin` directory.

## Building on Linux:
1. Run `Scripts/Linux-GenProjects.sh` to generate the Makefiles.
2. Run `make -j<number_of_cores> Storm-Cli config=dist` to build Storm.
3. Build outputs are located in the `bin` directory.

## Building SWIG:
On linux, must use the ```config=distshared``` when building.
1. Install SWIG
2. Setup a ```SwigConfigWindows.lua``` or ```SwigConfigLinux.lua``` depending on your platform. (Copy from ```SwigConfig.example.lua```)
3. Build the ```Storm-Swig``` project as above for your platform
