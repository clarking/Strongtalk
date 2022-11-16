# Strongtalk

Smalltalk.. with a need for speed


## Introduction

This is a personal project that qualifies as an experiment/exploration.
It currently builds on Linux x64 with GCC 12, but that's about all.

The codebase for this codebase was derived from various sources, including the official page and files but also the following projects:

* http://www.strongtalk.org/
* https://github.com/jiridanek/Strongtalk
* https://github.com/gjvc/strongtalk-2020

 ## Caveats

The Strongtalk project only started up again recently after having been inactive since 1996, and all VM development stopped at that point.   It is now starting up again, since Strongtalk is still by far the fastest Smalltalk implementation in existence, and is fully open-source unlike any other fast Smalltalk implementations, so there is much of value here.

While the C++ parts of the system now build under VisualStudio 2005 (and the Express version), the assembler files still require Borland Turbo Assembler.  To enable compilation without purchasing the assembler, we have included the .obj files in COFF format from the assembler, for convenience (in bin/asm_objs.

## Requirements

In order to run this version of the Strongtalk VM, the following is required:
 
* Intel Pentium PC, 90MHz, at least 32MB of RAM.
 
* GNU/Linux x86_64

In order to build the Strongtalk VM from scratch, additionally the following
software is required:
 
* GCC 12.2 
* Cmake 3.0

### Optional 

* CLion
 

## Build instructions

We will call the root strongtalk directory $STRTLK.


### CLion

Open the project folder with Clion, select the "Debug" configuration, optionally set 'build' to ouput directory, and Run the build


### Bash

1) git clone https://github.com/clarking/Strongtalk

2) cd into $STRTLK

3) mkdir build && cd build

4) cmake ..

5) cmake --build .


The executables end up in the $STRTLK directory.

The debug configuration turns off C++ optimizations, turns on debugging 
options, and also turns on extensive assertions in the code, so it runs 
slowly.


## Execute instructions

1) In the $STRLK directory, just run the VM executable you wish to use

strongtalk  Files needed in that directory for running an application are: 

- strongtalk.bst - the image file

- .strongtalkrc  - the VM configuration options file

- source folder containing the source code for the development environment


## Smalltalk is dangerous

It is a drug. My advice to you would be don't try it; it could ruin your life. Once you take the time to learn it (to REALLY learn it) you will see that there is nothing out there (yet) to touch it. Of course, like all drugs, how dangerous it is depends on your character. It may be that once you've got to this stage you'll find it difficult (if not impossible) to "go back" to other languages and, if you are forced to, you might become an embittered character constantly muttering ascerbic comments under you breath. Who knows, you may even have to quit the software industry altogether because nothing else lives up to your new expectations.   -- AndyBower 
