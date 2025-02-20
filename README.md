## QDOS: An experimental operating system for the GBA

## Why?
For fun!
I decided to start this project because I wanted to make the GBA feel like a PC from the early '90s.
I really liked the idea of having a graphical shell like Windows 95, it's a lot of work but I might get there sometime.
Plus, this is a nice opportunity for me to learn about the inner workings of an operating system, and what better way to do it if not on my favorite console to homebrew on?

## Goals
The main goal is to provide a simple hardware abstraction layer that allows programs to interact with it as if the console was a general purpose computer, in particular:
- Build a kernel that fits in the 32 KB of IWRAM
- Utilize EWRAM as a freely allocateable pool of memory for userspace programs and heap
- Provide a simple and tiny filesystem for non-volatile FLASH storage
- Using ELF as the main executable format instead of some unstandard format
- Builtin tools in ROM for developement directly on hardware and thinkering
- Some degree of customizability

## Status: moving towards milestone 1
Currently the project can execute code (not preemptively) but essential systemcalls are not implemented

## TODO (to reach milestone 1)
- Implement essential systemcalls
- Implement essential file I/O operations

## TODO (in the future)
- Multitasking system
- Improve keyboard and terminal
- Write documentation
