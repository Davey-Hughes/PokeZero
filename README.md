# PokeZero
PokeZero is a Monte Carlo Tree Search (MCTS) ML approach to playing Pokemon games using the Pokemon Showdown simulator.

## Build
Making the default target which includes debugging flags:
```
make
```

Making the release build:
```
make release
```

When changing targets it will probably be necessary to `make clean` or pass the `-B` flag to make to force the rebuild of intermediate files.

The default compiler is clang++, but a compiler can be specified:
```
make CXX=g++
```
