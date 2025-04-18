## QDOS: An experimental operating system for the GBA

## Why?
For fun!
I decided to start this project because I wanted to make the GBA feel like a PC from the early '90s.
I really liked the idea of having a graphical shell like Windows 95, it's a lot of work but I might get there sometime.
Plus, this is a nice opportunity for me to learn about the inner workings of an operating system, and what better way to do it if not on my favorite console to homebrew on?

## Goals
The main goal is to provide a simple hardware abstraction layer that allows programs to interact with it as if the console was a general purpose computer, in particular:
- Build a kernel that fits in the 32 KiB of IWRAM (and in max. 32MiB of ROM)
- Utilize EWRAM as a freely allocateable pool of memory for userspace programs and heap
- Provide a simple and tiny filesystem for non-volatile FLASH storage
- Using ELF as the main executable format instead of some nonstandard format
- Builtin tools in ROM for developement directly on hardware and thinkering
- Some degree of customizability
Also, as a bonus I'd like to use the GBA link functionality to connect external peripherals such as mouse and keyboard and exchange files between systems.

## Status: moving towards milestone 2
Currently the project can execute code (not preemptively) and it's very insecure.
Right now what the OS can do is very limited, but it's a start.
A program can:
- Write strings to the console and redraw it on demand
- Open, Close, Read, Write files
- Read and write everything the OS can

## TODO (to reach milestone 2)
- Implement more systemcalls
- Console escape sequences to control the text
- Multicolor terminal output
- Enhance standard allocator function with wrappers

## TODO (in the future)
- Multitasking system
- Improve keyboard and terminal
- Write documentation

## Building the project
Please make sure that you have [DevkitPRO](https://devkitpro.org/wiki/Getting_Started) properly configured and its GBA toolchain installed.
To build, simply navigate to the root directory of this project and run `make`.
You will now have a .gba ROM.

## Testing the OS
Upon starting the ROM for the first time, the OS will likely ask you to type `fmt` to format the flash storage, to do it press L+R to open the virtual keyboard.
> Virtual keyboard controls:
> Select the highlighted option with the DPAD, press A to type/select, press B to backspace, press SELECT to change case and finally press START to RETURN and close the virtual keyboard.
> You can close the keyboard by pressing L+R again.

That's it! you are now ready to use the OS.
Type `cmd` for a list of builtin commands.
To run a command or execute a file simply type its name (you don't have to type the file extension as well, you should do it only for disambiguation).

Check out the "apps" directory to learn how to make your own programs for QDOS.
