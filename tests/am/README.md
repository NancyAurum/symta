# Adaptive map regression tests

The adaptive map (`runtime/am.h`) backs every Symta hash table, every
class column, every cache. It picks one of six internal
representations (`AM_EMPTY` / `AM_GENERIC` / `AM_TEXT` / `AM_INT` /
`AM_BITMAP0` / `AM_BITMAP1`) on first write and promotes between
them when the value or key type forces it. Most of the bugs in this
area show up only when a specific representation transition fires
under a real workload -- the cls layer's GID-keyed columns, the
ECS-style bitmap-then-int promotion, mixed-key tables that get
forced into `AM_GENERIC` -- so blind black-box testing rarely catches
them. Hence this suite, written from the inside out.

## Coverage

Each `cases/tc_<name>.s` is a focused regression for one slice of the
adaptive map:

  - **modes**            -- the six representations behave correctly
                            on the operations they support (get / has /
                            got / set / del / n / l / ks)
  - **empty**            -- a freshly-allocated table; default and
                            customised void value; the "set value ==
                            void = delete" rule
  - **bitmap0/bitmap1**  -- the dense-presence representations; sparse
                            insertion, lookup, deletion; correct
                            behaviour at the bitmap-page boundary
  - **promote**          -- the transitions:
                              EMPTY -> BITMAP0/BITMAP1/INT/TEXT/GENERIC
                              BITMAP0 -> INT (writing a non-0 value)
                              BITMAP1 -> INT (writing a non-1 value)
                              INT/TEXT -> GENERIC (mixed key types)
                            Each transition preserves existing keys and
                            values.
  - **gid**              -- `gid_get_` / `gid_set_` (the builtin
                            wrappers around `amGidGet` / `amGidSet`)
                            return correct values for every AM mode.
                            Regression cover for AM-1.
  - **mixed**            -- a GENERIC table with int + fixtext + bigtext
                            + tag + list keys; round-trips through
                            insert, lookup, delete; AM-3 fast paths.
  - **iteration**        -- `.l`, `.ks`, `.n` agree with each other and
                            with the set of keys written.
  - **stress**           -- 100k entries, random-access lookup, half-
                            delete, retry. Catches issues that only
                            show up at hash-grow boundaries.

## Running

```
make test-am               # via the top-level Makefile target
bash tests/am/run.sh       # directly
bash tests/am/run.sh modes # just the `tc_modes` case
bash tests/am/run.sh --update   # rewrite goldens from current output
```

Each case prints `PASS <label>` / `FAIL <label>: ...` lines; the
runner asserts no FAIL appears and that the captured output matches
`expected/<name>.out`.
