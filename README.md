A simple cross-platform arena allocator (single-header, stb-style).

## Portability

- **Windows**: `VirtualAlloc` / `VirtualFree`; resize = alloc new, copy, free old.
- **Linux, macOS, BSD (Unix-like)**: `mmap` / `munmap`; resize = mmap new, copy, munmap old.
- **Other**: fallback to `malloc` / `realloc` / `free`.

No `_GNU_SOURCE` or other feature macros required. Static arenas work on any C11 implementation; dynamic arenas need one of the above platforms or the malloc fallback.

## Build
```sh
cc -o nob nob.c
./nob          # build basic example → build/basic
./nob test     # build and run tests → build/test_aloctr
./build/basic  # run the example
```
Build artifacts go into the `build/` directory.