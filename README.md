FileList.exe

Reads the data of all files, recursively, in target directory. Loops forever,
with a user prompt to quit after every loop, or until read is accomplished
within a goal time. Inability to access a file is currently set to NOT stop the
program; it just moves on to the next file.

The purpose of this program is to load an SSD cache from a hard drive. I wrote
it to load PrimoCache with FF14, as it was doing a bad job, averaging a sub-1%
cache hit rate. After running this program on bootup, and letting it go for
about 4 minutes, my cache hit rate is now over 99%.

The program grew from there. Now it takes the directory & goal time as command
line arguments. There is a "fancy" rsync-style display. I didn't figure out
ncurses, so I hacked together a line-clearing method. This integrates into the
Windows "Send-To" menu, so you can run it from File Explorer.

One argument is required - the directory to be scanned. Put it in Quotes to make
life easier - long-filenames and spaces in directroy names.

One argument is optional - the directory goal time to quit after, in seconds.
Argument order is not important.

Example A:

```
C:\> filelist.exe C:\Directory
```

Example B:

```
C:\> filelist.exe 64 "D:\Directory\With Spaces"
```

Designed to run in Windows 64-bit. Compiles under Visual Studio 2022. Written
in C++, as vanilla as I can make it. Probably easily portable to Linux and I 
may try for my Plex box at some point, just to see if it works. Console
application because GUIs scare me. I plan on making a Win32 version at some
point, but it currently functions for my purposes quite will.

Final Fantasy 14 is copyright SquareEnix. No assets or interaction with the
game are used or done by this program.
PrimoCache is copyright Romex Software. No interaction with PrimoCache is done
by this program.
Both are fine pieces of software I highly recommend.







don't sue me