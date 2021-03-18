# gem5/SST integration (2021)

## Status
SST can load gem5 library and run gem5 simulation.
The current setup works for running gem5 simulation with learning gem5's two-level cache system configuration, which consists of,
-  TimingSimpleCPU
-  Two-level cache (both are within gem5)
-  DDR3_1600_8x8 DRAM
However, currently, there is no support for allowing interactions between gem5 and SST.

## Running the simulation
```sh
env LD_LIBRARY_PATH=$GEM5_FOLDER/build/X86 sst --add-lib-path $GEM5_FOLDER/ext/sst2/ $GEM5_FOLDER/ext/sst2/sst-configs/test_x86.py
```

## Setting up the simulation

### Compiling gem5
```sh
scons3 build/X86/gem5.opt CPU_MODELS=AtomicSimpleCPU,MinorCPU,O3CPU,TimingSimpleCPU -j256 --without-tcmalloc
```

### Compiling libgem5_opt.so (gem5 library)
```sh
scons3 build/X86/libgem5_opt.so -j 256 --without-tcmalloc
```

### Adding libgem5_opt.so to LD_LIBRARY_PATH
```sh
echo "export LD_LIBRARY_PATH=$GEM5_FOLDER/X86/build:$LD_LIBRARY_PATH" > ~/.bashrc
```

### Downloading SST
Download SST source code from here <http://sst-simulator.org/SSTPages/SSTMainDownloads/>.
```sh
wget https://github.com/sstsimulator/sst-core/releases/download/v10.1.0_Final/sstcore-10.1.0.tar.gz
tar xf sstcore-10.1.0.tar.gz
```

### Compiling SST Core
Original documentation is here <http://sst-simulator.org/SSTPages/SSTBuildAndInstall10dot1dot0SeriesDetailedBuildInstructions/>.

Setting up the environment variables,
```sh
export SST_CORE_HOME=$HOME/sst             # where to install SST
export SST_CORE_ROOT=$HOME/sstcore-10.1.0  # the location of SST source code
```

Configure SST-Core,
```sh
./configure --prefix=$SST_CORE_HOME --with-python=/usr/bin/python3-config
            --disable-mpi # to compile without MPI
```

Compile SST,
```sh
make all -j8
make install
```

Update `PATH`,
```sh
export PATH=$SST_CORE_HOME/bin:$PATH
```

Testing the installation,
```sh
which sst
```


### Downloading SST Elements
Download SST source code from here <http://sst-simulator.org/SSTPages/SSTMainDownloads/>.
```sh
wget https://github.com/sstsimulator/sst-elements/releases/download/v10.1.0_Final/sstelements-10.1.0.tar.gz
tar xf sstelements-10.1.0.tar.gz
```
### Compile SST Elements
Original documentation is here <http://sst-simulator.org/SSTPages/SSTBuildAndInstall10dot1dot0SeriesDetailedBuildInstructions/>.

Setting up the environment variables,
```sh
export SST_ELEMENTS_HOME=$HOME/sst/elements                 # where to install SST
export SST_ELEMENTS_ROOT=$HOME/sst-elements-library-10.1.0  # the location of SST source code
```

Configure SST-Elements,
```sh
./configure --prefix=$SST_ELEMENTS_HOME --with-sst-core=$SST_CORE_HOME --with-python=/usr/bin/python3-config
```

Compile SST-Element,
```sh
make all -j8
make install
```

Update `PATH`,
```sh
export PATH=$SST_ELEMENTS_HOME/bin:$PATH
```

Testing the installation,
```sh
sst $SST_ELEMENTS_ROOT/src/sst/elements/simpleElementExample/tests/test_simpleRNGComponent_mersenne.py
```

### Compiling gem5/SST interface
This step must be done after gem5 is compiled.
```sh
make -C ext/sst2
```