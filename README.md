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
- [x] **02 - Character driver.** `/dev/sigbuf` via `alloc_chrdev_region` + `cdev` + `class_create`,
      `file_operations` (open/read/write/release), `copy_to_user` / `copy_from_user`,
      a mutex-protected circular buffer, three `ioctl` commands, and a userspace test suite.
- [ ] **03 - Yocto.** Own layer and recipes, kernel module cross-compiled into a custom image, booted under QEMU.
- [ ] **04 - CI.** Jenkins pipeline building the module and running the tests on every push.
- [ ] **05 - Zynq hardware.** AXI-Lite IP core in Verilog, PetaLinux for the board, device-tree node,
      driver rewritten against real hardware instead of a memory buffer.

## Environment

Ubuntu 24.04 LTS, kernel 7.0.0-38-generic, gcc 13.3.0, VirtualBox 7.2.20

## 01 - Hello module

    cd 01-hello-module
    make
    sudo insmod hello.ko who=world
    sudo dmesg | tail -2
    sudo rmmod hello

## 02 - Character driver

A character device backed by a circular buffer: userspace writes bytes to `/dev/sigbuf`
and reads them back in FIFO order — the shape of a sample buffer between a hardware
producer and a userspace consumer.

- dynamic major number via `alloc_chrdev_region`, device node created through `class_create` / `device_create`
- reads and writes cross the user/kernel boundary with `copy_to_user` / `copy_from_user`, never a raw pointer
- the ring wraps, so a single transfer can be two physically separate chunks
- all shared state is protected by a mutex taken with `mutex_lock_interruptible`
- `ioctl`: `SIGBUF_RESET`, `SIGBUF_GET_STORED`, `SIGBUF_GET_CAPACITY`, with an `_IOC_TYPE` guard
- failure paths in `init` unwind in reverse order, so a failed load leaves nothing behind

Build and test:

    cd 02-char-driver
    make
    sudo insmod sigbuf.ko
    sudo ./sigbuf_test

Result:

    ioctl GET_CAPACITY                             PASS
       capacity reported: 1024 bytes
    ioctl RESET                                    PASS
    buffer empty after reset                       PASS
    write returns the full length                  PASS
    GET_STORED matches bytes written               PASS
    read returns the same bytes                    PASS
    buffer empty after draining                    PASS
    read on empty buffer returns 0                 PASS
    data survives a wrap around the ring           PASS
    full buffer rejects writes with ENOSPC         PASS

    ALL TESTS PASSED

---
Yusuf Mehmed - github.com/Elmasyumruk
