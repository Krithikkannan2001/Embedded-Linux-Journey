# Day 17 — Cross-compilation

## Concept
- Compiled on x86_64 host (WSL2), produced binary for a different target architecture (aarch64, Pi 5)
- Toolchain naming convention: arch-vendor-os-abi (e.g. aarch64-linux-gnu-gcc)
- Compared to STM32's arm-none-eabi-gcc (bare-metal, no OS) vs aarch64-linux-gnu-gcc (Linux target)

## Hands-on
- Installed crossbuild-essential-arm64 on WSL2 (Simba)
- Wrote hello.c, compiled with aarch64-linux-gnu-gcc
- Confirmed cross-compiled binary fails on host (x86_64): "Exec format error"
- Confirmed via `file` command: ELF 64-bit LSB pie executable, ARM aarch64
- Transferred via scp to Pi, chmod +x, ran successfully on target hardware

## Relevance for later
- Same workflow needed for BeagleBone Black U-Boot cross-compilation (October)
- Same workflow underlies Yocto bitbake builds (AM5706 side project, Day 20)

## Noted for later
- Sysroot concept: needed when cross-compiling programs with more complex library
  dependencies, to avoid host/target library version mismatches (not needed for
  this simple hello-world example)
