# Tests

Rigorous test suite for the aloctr (stb_arena) allocator.

## Run

From the project root:

```bash
./nob test
```

Exit code 0 = all pass, 1 = failure.

## Coverage

- **Static init**: valid buffer, NULL aloc, zero-length buffer
- **Dynamic init**: valid, NULL aloc
- **alloc**: NULL aloc, zero size, single/multiple blocks, full static returns NULL, dynamic resize
- **alloc_aligned**: alignments 1–16, padding, NULL aloc, zero size
- **alloc default alignment**: natural alignment (max_align_t)
- **reset**: reuse of buffer, dynamic capacity preserved, NULL aloc no-op
- **free**: static/dynamic state cleared, NULL aloc, double free safe
- **Edge cases**: many small allocs, overflow returns NULL, alignment 0 → 1, stress alloc/reset
