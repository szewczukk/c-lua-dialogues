# dialogues

C dialogue system library (Lua-backed).

## Build

```sh
make
```

Produces `build/libdialogues.a`.

## Tests (Unity)

Tests use the [Unity](https://github.com/ThrowTheSwitch/Unity) framework via a git submodule. After cloning, initialize submodules:

```sh
git submodule update --init --recursive
```

Then build and run tests:

```sh
make test
```

## Format / lint

- `make format` — format source with clang-format
- `make check` — check formatting (CI)
- `make lint` — run clang-tidy
