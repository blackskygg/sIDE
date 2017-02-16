# sIDE
Assembler, simulator and a simple IDE for a simplified 16-bit assembly language.

The specification of the simplified computer and the assembly language can be found here(in Simplified Chinese):

https://github.com/blackskygg/sIDE/blob/master/Assignment1.pdf

This Program is written as a curriculum design project in HUST. 
I don't know if the teachers in your university like asking students to write a GUI for ANY curriculum design projects, too.
If so, feel free to use this code if you want a simple IDE.

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
I've already done this for you. Download it here:

https://github.com/blackskygg/sIDE/releases

But you do need a installation of the vcruntime. If you don't have it, don't worry, just run `vcredist_x64.exe` in the zip.
