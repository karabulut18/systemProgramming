# System Programming Experiments

This repository contains a collection of C programs and examples demonstrating various core system programming concepts. It serves as a playground for understanding low-level operations, compilation processes, and communication mechanisms in an operating system environment.

## Directory Structure

### 1. `examineBinary`
Exploration of the compilation process and binary file analysis. 
- Contains source code (`.c`) along with intermediate files generated during different stages of compilation:
  - Preprocessed (`.i`)
  - Assembly (`.s`)
  - Object (`.o`)
  - Final Executable

### 2. `interProcessCommunication` (IPC)
Examples of different mechanisms for processes to communicate with each other:
- **FIFO (Named Pipes)**
- **Message Queues**
- **Pipes** (Anonymous pipes)
- **Shared Memory**
- **Sockets**

### 3. `interThreadCommunication`
(Assumed based on directory structure)
- Examples demonstrating synchronization and communication between threads (e.g., mutexes, condition variables).

## Prerequisites

- GCC or Clang compiler
- Standard C libraries (libc)
- A Unix-like operating system (Linux/macOS) is recommended for IPC/threading examples.

## Compilation & Usage

Most examples can be compiled using `gcc`. For example, to compile a file in the `examineBinary` directory:

```bash
cd examineBinary
gcc examineBinary.c -o examineBinary
./examineBinary
```

For threading examples, ensure you link the pthread library if required:

```bash
gcc source_file.c -o output -lpthread
```

## Notes

- Some IPC mechanisms (like message queues or shared memory) may differ slightly between System V and POSIX standards.
- Ensure you have appropriate permissions when creating/accessing named pipes or shared memory segments.
