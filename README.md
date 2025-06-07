# ARM64 CPU Emulator
This project is a cycle-accurate ARM64 CPU emulator, written in C, that provides a detailed simulation of a modern processor's core functionalities. It is designed from the ground up to model the complex interactions within a CPU pipeline, implementing a significant subset of the ARMv8-A instruction set architecture (ISA).

## Core Architectural Features
The emulator is built around a classic five-stage pipeline:

1. Fetch: Retrieves the next instruction from memory using the program counter.
2. Decode: Interprets the instruction, identifies operands, and generates control signals for subsequent stages.
3. Execute: Performs the primary computation using an emulated Arithmetic Logic Unit (ALU), calculating results or effective memory addresses.
4. Memory: Handles all memory access operations (LDUR/STUR), interacting with the cache and main memory system.
5. Write-back: Writes the final result of an instruction back to the register file.
The state of the machine is advanced on a cycle-by-cycle basis, with data flowing through a series of pipeline registers that connect these stages.

## Advanced Simulation Components
Beyond the basic pipeline structure, the emulator implements critical features necessary for realistic processor simulation:

- Hazard Control: To ensure correct program execution, the emulator features a sophisticated hazard control unit. It correctly detects and mitigates all three types of hazards:

        - Data Hazards: Resolved through a comprehensive forwarding unit (or bypassing) that sends results from the Execute, Memory, and Write-back stages directly to the Decode stage's inputs, preventing unnecessary stalls. When forwarding is not possible, the pipeline is stalled correctly.
        - Control Hazards: Detected upon decoding a branch instruction. The pipeline is flushed (or "bubbled") to discard incorrectly fetched instructions and ensure the program counter is updated to the correct branch target.
        
- Configurable Cache Simulation: The emulator includes a highly detailed simulation of a set-associative L1 data cache. This system moves beyond a simple, ideal memory model to accurately simulate real-world memory latency. When a memory operation occurs, the cache is checked first. A cache hit results in no pipeline delay, while a cache miss triggers a stall for a configurable number of cycles, simulating the penalty of fetching data from main memory. The cache's geometry (associativity, line size, and total capacity) and miss penalty are all configurable parameters of the simulation.

## Software Design
The source code in the src/ directory is organized modularly to reflect the distinct components of the CPU:

- The pipe/ directory contains the core logic for the pipeline stages, hazard detection, and forwarding.
- The cache/ directory houses the implementation of the L1 data cache.
- The base/ directory provides the fundamental building blocks, including the emulated hardware elements (ALU, register file), the main simulation loop, and the memory system interface.

This project serves as a practical and in-depth exploration of computer architecture, demonstrating the intricate logic that underpins modern processor design.