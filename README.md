# Storm
C++ UCI Chess engine inspired by [Stockfish](https://stockfishchess.org/).
Created using ideas learned during the developlemnt of [Boxfish](https://github.com/Totomosic/Boxfish).

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
  - Material
  - Piece squares
  - King safety
  - Space
  - Knights, Bishops, Rooks, Queens
- All perft tests passed
- Pondering
- SEE move ordering

## Installing:
1. Download or clone this repository.
2. If on windows run the `Scripts/Win-GenProjects.bat` script to generate the Visual Studio 2019 project and solution files.

## Building on Windows:
1. Run `Scripts/Win-GenProjects.bat` and build the solution using Visual Studio 2019.
2. Build outputs are located in the `bin` directory.

## Building on Linux:
1. Run `Scripts/Linux-GenProjects.sh` to generate the Makefiles.
2. Run `make -j<number_of_cores> Storm-Cli` to build Storm.
3. Build outputs are located in the `bin` directory.

## Building Python SWIG Bindings:
1. Copy `SwigConfig.lua.example` to `SwigConfigWindows.lua` or `SwigConfigLinux.lua` depending on operating system
2. Update the relevant information in the config file
3. Run the relevant `{os}-GenProjects` script
4. Build the `Storm-Swig` project as you normally would on your operating system (on linux you must use `config=distshared` or `config=releaseshared`)
5. This will generate a `.py` and a shared object file in the `bin` directory
