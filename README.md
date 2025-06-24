To build and run in debug mode:

```shell
./premake5 gmake2
make -j8
./bin/main-debug-x64-gcc.exe
```


### Benchmarking

To benchmark, make sure that in `main.cpp`, the first line: `PREPARE_BENCHMARK` is always uncommented.
Then choose another line to uncomment depending on what you would like to benchmark. Below is an example if you want to benchmark
the full rendering time:

```c++
#define PREPARE_BENCHMARK
#define ENABLE_BENCHMARK_FULL
//#define ENABLE_BENCHMARK_12
//#define ENABLE_BENCHMARK_14
//#define ENABLE_BENCHMARK_15
//#define CPU_BENCHMARK
```

For example, to benchmark CPU time only:
```c++
#define PREPARE_BENCHMARK
//#define ENABLE_BENCHMARK_FULL
//#define ENABLE_BENCHMARK_12
//#define ENABLE_BENCHMARK_14
//#define ENABLE_BENCHMARK_15
#define CPU_BENCHMARK
```

Then, perform a release build using the provided `release.sh` script:

```shell
chmod 755 release.sh
./release.sh
```