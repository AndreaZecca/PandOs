# PandOs
A working C-based operating system running on uMPS3. Project for Operating Systems exam.

## Introduction
PandOs is a multi-layer operating system implementing key function of a Unix base OS, running on a μMPS3 machine.

## Technologies
* C language programming
* μMPS3 emulator (see http://wiki.virtualsquare.org/#!education/umps.md)

## Phase 1
The Queues Manager. Based on the key operating systems concept that active entities at one layer are just data structures at lower layers, this layer supports the management of queues of structures: pcb’s and asl's.

## Phase 2
The Kernel. This level implements eight new kernel-mode process management and synchronization primitives in addition to multiprogramming, a process scheduler, device interrupt handlers, and deadlock detection.

## Phase 3
The Support Level - The Basics. Level 3 is extended to support a system that can support multiple user-level processes that each run in their own virtual address space. Furthermore, support is provided to read/write to character-oriented devices.


## How to compile and run
1. Move to the phase_3 directory.
2. Execute the "make" command in order to compile all the files you need
3. Move back to the resources/testers directory
4. Execute the "make" command in order to compile all the files you need for testing the system
5. Make sure you have installed μMPS3 on your system (if not, check this guide https://github.com/virtualsquare/umps3)
6. Open the μMPS3 emulator (via umps3 command on a terminal)
7. Choose the right configuration. You can find the configuration in resources directory (resources/phase3Config)
8. Warning, you might modify the path in the Makefile adding '/local' after '/usr' level
9. Turn on the emulator and press start button
10. Test will ask you to provide a string for a string concatenate test in terminal 0

--------------------------------------------------------------------------------------
