gem5/SST integration

To compile gem5, the simulator,
```sh
scons3 build/RISCV/gem5.opt CPU_MODELS=AtomicSimpleCPU,MinorCPU,O3CPU,TimingSimpleCPU -j 100 --without-tcmalloc
```

To compile gem5 lib,
```sh
scons3 build/RISCV/libgem5_opt.so -j 100 --without-tcmalloc
```

To compile gem5 component, the SST component,
```sh
make -C ext/sst2
```

To run the simulation,
```sh
cd ext/sst2
sst sst/simple.py
```