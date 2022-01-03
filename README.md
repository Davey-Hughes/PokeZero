# PokeZero
PokeZero is a Monte Carlo Tree Search (MCTS) ML approach to playing Pokemon
games using the Pokemon Showdown simulator.

## Dependencies
* [nlohmann/json](https://github.com/nlohmann/json):  
    * The current makefile uses `pkg-config` to find the location of this
      library automatically if it was installed by a package manager.

## Build
Making the default target which includes debugging flags:
```
make
```

Making the release build:
```
make release
```

The default compiler is clang++, but a compiler can be specified:
```
make CXX=g++
```

The easiest way to run the resulting executable is with
```
make run
```
