# sIDE
Assembler, simulator and a simple IDE for a simplified 16-bit assembly language.

The specification of the simplified computer and the assembly language can be found here(in Simplified Chinese):

https://github.com/blackskygg/sIDE/blob/master/Assignment1.pdf

This Program is written as a curriculum design project in HUST.

## Usage
### sas
sas is the assembler, invoke it like this:

    sas in_file out_file
    
### ssim
ssim is the emulator, invoke it like this:

    ssim filename
    
### sIDE
sIDE is the IDE, which stands for "stupid IDE", it looks like this:

![image](https://github.com/blackskygg/sIDE/blob/master/screenshots/screenshot.png)

#### Features
* Basic editing
* Line number
* Syntax highlighting
* Line positioning on error
* Integrated simulator

## Build
### Linux
Make sure you have qt5 correctly installed on you computer, and `make`. That's it.

### Windows
I've already done this for you. But you do need a installation of the vcruntime(vc++2015).
