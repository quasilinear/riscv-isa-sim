Q (Quasilinear) Instruction Set Extension
=========================================

[Quasilinear Research](https://quasilinear.com/) is developing a probabilistic
extension to the [RISC-V Instruction Set Architecture](http://riscv.org/). The
intent of the extension is to provide new instructions for defining search and
inference problems that will be made available to programmers in a variety of
high-level compiled languages.

This extension of the RISC-V instruction set simulator, [spike]
(https://github.com/riscv/riscv-isa-sim), allows execution of binaries utilizing
those new instructions. Instead of providing several implementations of the
probabilistic semantics within this open-source project, we implement a minimal
stub that delegates execution to external plug-in libraries (called drivers
here).

Design
----------

The Q instruction set extension utilizes the `custom0` encoding space associated
with RoCC accelerators. The software interface that drivers must implement is
similar to the the RoCC accelerator interface. That is, given two general
purpose 64-bit register values `xs1` and `xs2` (which are set to zero if not read) and
a `funct` value extracted from the `custom0` instruction, the driver should
yield a 64-bit value to be written to the destination register (if the
instruction calls for writing). Drivers are provided with callbacks for reading
other registers and memory to aid in the search process. The name of the
external driver to load is given an environment variable so that driver
implementations may be changed without rebuilding the instruction set simulator.

See `qio.h` for the specification of the interface drivers (dynamically loaded
shared libraries) should implement.