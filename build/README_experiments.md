# Starlink experiments — repository-layout version

This package is adjusted to your repository layout:

```text
starlink-htsim/
  README.md
  build/
    Makefile
    build.sh
    run_benchmarks.sh
    getstat.sh
    README_experiments.md
  src/
    Makefile
    parse_output.cpp
    parse_output
    libhtsim.a
    starlink/
      main.cpp
      city.cpp / city.h
      constellation.cpp / constellation.h
      experiment_logger.cpp / experiment_logger.h
      ...
```

## Where files go

Copy/merge this package into the root of your repository:

```bash
cd starlink-htsim
unzip modified_starlink_experiments_repo_v3.zip
chmod +x build/build.sh build/run_benchmarks.sh build/getstat.sh
```

The package intentionally contains paths like `src/starlink/main.cpp` and `build/Makefile`, so it can be extracted directly into `starlink-htsim`.

## Build

From `starlink-htsim/build`:

```bash
./build.sh
```

`build.sh` does exactly this, using absolute paths internally so it can be run safely from any directory:

```bash
cd ../src
make clean
make
cd ../build
make clean
make
```

The expected binaries are:

```text
src/parse_output
build/starlink_exp
build/parse_output -> ../src/parse_output   # convenience symlink
```

For a very hard clean after changing headers such as `src/starlink/constellation.h`:

```bash
cd starlink-htsim/build
make deep-clean
./build.sh
```

## Single 2-satellite sanity run

Run from `starlink-htsim/build`:

```bash
mkdir -p results
./starlink_exp \
  --planes 1 \
  --sats-per-plane 2 \
  --orbital-slots 66 \
  --sat-selection adjacent \
  --src 0,0 \
  --dst 0,5 \
  --duration 600 \
  --route-update-ms 1000 \
  --routing-only \
  --out results/A_sanity_2sat_adjacent
```

Main outputs:

```text
results/A_sanity_2sat_adjacent.config.json
results/A_sanity_2sat_adjacent.routes.csv
results/A_sanity_2sat_adjacent.visibility.csv
results/A_sanity_2sat_adjacent.summary.csv
```

## Full benchmark suite

Run from `starlink-htsim/build`:

```bash
./run_benchmarks.sh results
```

With a deep clean first:

```bash
DEEP_CLEAN=1 ./run_benchmarks.sh results
```

Without rebuilding:

```bash
SKIP_MAKE=1 ./run_benchmarks.sh results
```

## Log parsing

`getstat.sh` lives in `build/` and defaults to using `../src/parse_output`:

```bash
./getstat.sh results/D_London_NY_24planes_ping
```

It creates:

```text
*.ascii.txt
*.events.csv
*.queue.csv
*.xcp_sink.txt
*.tcp_sink.txt
*.queue_ascii.txt
*.tcp_sink_summary.txt
*.queue_summary.txt
```

## Notes

- Do **not** compile with `-DXCP_STATIC_NETWORK`; that branch is the fixed toy topology and is not compatible with variable satellite-count experiments.
- The normal constructor of `Constellation` is preserved for existing htsim code such as `XcpNetworkTopology`.
- The extended constructor is used only by `src/starlink/main.cpp` for benchmark runs with `--orbital-slots`, `--sat-selection adjacent`, and `--sat-offset`.
- `parse_output.cpp` is now placed in `src/parse_output.cpp`, not in `src/starlink/parse_output.cpp`, matching your repository.
- `build/Makefile` links directly against `../src/libhtsim.a` instead of using `-L../src -lhtsim`, which avoids the `ld: library 'htsim' not found` issue when the library name/path is not picked up correctly by clang on macOS.
