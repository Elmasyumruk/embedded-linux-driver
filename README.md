# Embedded Linux Driver

Working toward one goal: **an FPGA IP core on a Xilinx Zynq SoC, driven by a Linux kernel
driver I wrote, on a PetaLinux image I built.**

Built in public, one step at a time. Started 9 October 2026.

## Why

I hold a B.Sc. in Computer Engineering and a B.Sc. in Software Engineering
(Eastern Mediterranean University, 2026). My degree covered Operating Systems,
Digital Logic Design in Verilog/VHDL, Embedded System Design, Real-Time System Design,
Computer Architecture and Electronics — but not driver development or Yocto.

This repository is me closing that gap, in the open, rather than claiming I already had it.

## Roadmap

- [x] **01 - Hello module.** Loadable kernel module built out-of-tree against the running kernel, with a module parameter.
- [ ] **02 - Character driver.** `/dev` node via `alloc_chrdev_region` + `cdev`, `file_operations`
      (open/read/write/release), `copy_to_user` / `copy_from_user`, a mutex-protected ring buffer,
      an `ioctl`, and a userspace test program in C.
- [ ] **03 - Yocto.** Own layer and recipes, kernel module cross-compiled into a custom image, booted under QEMU.
- [ ] **04 - CI.** Jenkins pipeline building the module and running the tests on every push.
- [ ] **05 - Zynq hardware.** AXI-Lite IP core in Verilog, PetaLinux for the board, device-tree node,
      driver rewritten against real hardware instead of a memory buffer.

## Environment

Ubuntu 24.04 LTS, kernel 7.0.0-38-generic, gcc 13.3.0, VirtualBox 7.2.20

## 01 - Hello module

Build and run:

    cd 01-hello-module
    make
    sudo insmod hello.ko who=world
    sudo dmesg | tail -2
    sudo rmmod hello

Output:

    [  842.884045] hello: module loaded, hello world
    [  842.925949] hello: module unloaded

---
Yusuf Mehmed - github.com/Elmasyumruk
