# Strongtalk

Smalltalk.. with a need for speed


## Introduction

This is a personal project that qualifies as an experiment. It currently builds on Linux x64 with GCC 12, but that's about all.

The codebase for this codebase was derived from various sources, including the official page and files but also the following projects:

* http://www.strongtalk.org/
* https://github.com/jiridanek/Strongtalk
* https://github.com/gjvc/strongtalk-2020
 
The Strongtalk project has been restarted many times throught the years since it's conception in the 90's. There has been some interest towards resurrecting the project and many improvements have been made in the past but the system remains somewhat unstable and still only running 'fully' on windows hosts. 

This fork doesn't aim to fulfill the dreams of the 'Strongtalk Developers' if any, but to serve as a vehicle for exploration, learning and in some way preservation of 'the fastest Smalltalk implementation in existence' as the original sources pointed out.

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
```bash
$ git clone https://github.com/clarking/Strongtalk

$ cd into $STRTLK

$ mkdir build && cd build

$ cmake ..

$ cmake --build .
```

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

It is a drug. 
My advice to you would be don't try it; it could ruin your life. 
Once you take the time to learn it (to REALLY learn it) 
you will see that there is nothing out there (yet) to touch it. 

Of course, like all drugs, how dangerous it is depends on your character. 

It may be that once you've got to this stage you'll find it difficult (if not impossible)
to "go back" to other languages and, if you are forced to, you might become
an embittered character constantly muttering ascerbic comments under you breath.

Who knows, you may even have to quit the software industry altogether because 
nothing else lives up to your new expectations.  

 -- AndyBower 
