# DevteroFlex QEMU

This is the instrumented version of QEMU used by the the QFlex simulator.

In the following section, we will describe the process to build QEMU for QFlex.

# Building QEMU

This guide assumes Ubuntu 20.04 platform.

To build QEMU, first download all the dependencies by executing the script:

```
$ bash scripts/qflex/install-deps.sh
```

Now, configure and build QEMU:
```
$ ./configure --target-list=aarch64-softmmu --enable-werror --disable-docs --disable-containers --enable-qflex
$ make -j6
```

After the build process is complete, you are ready to experiment with QEMU. 
We use git-lfs to store and share QEMU images. Download an image [here](https://github.com/parsa-epfl/images/tree/ubuntu20.04-aarch64) and give it a test!
