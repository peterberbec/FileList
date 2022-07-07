# Fill_SSD_Cache #
\
Reads the data of all files, recursively, in target directory. Loops forever, with a user prompt to quit after every loop, or until read is accomplished within a goal time. Inability to access a file is currently set to NOT stop the program; it just moves on to the next file.
\
\
The purpose of this program is to load an SSD cache from a hard drive. I wrote it to load PrimoCache with FF14, as it was doing a bad job, averaging a sub-1% cache hit rate. After running this program on bootup, and letting it go for about 4 minutes, my cache hit rate is now over 99%. 
\
\
The program grew from there. Now it takes the `directory` to read & (`goal time` or `endless`-flag) as command line arguments. There is a "fancy" rsync-style display. I didn't figure out ncurses, so I hacked together a line-clearing method. This integrates into the Windows "Send-To" menu, so you can run it from File Explorer. 
\
\
Default behavior is to read directory until 350MB/second is reached.
### Usage ###
Argument order does not matter.\
\
One argument is required - the directory to be scanned. Put it in Quotes to make life easier - long-filenames and spaces in directroy names are things I didn't want to deal with.\
\
The second argument can be two things.\
\
Argument 2, choice 1 - goal time, positive integer. If the read completes in this many seconds, quit.\
Argument 2, choice 2 - `/f` or `-f` or `/forever` or `-forever` - If this is set, never stop reading the directory.\
\
Example A:
```
C:\> Fill_SSD_Cache.exe C:\Directory
```
Example B:
```
C:\> Fill_SSD_Cache.exe 64 "D:\Directory\With Spaces"
```
Example C:
```
C:\> Fill_SSD_Cache.exe "D:\Square Enix" /forever
```
Designed to run in Windows 64-bit. Compiles under Visual Studio 2022. Written in C++, as vanilla as I can make it. Probably easily portable to Linux and I may try for my Plex box at some point, just to see if it works. Console application because GUIs scare me. I plan on making a Win32 version at some point, but it currently functions for my purposes quite will.\
\
[Final Fantasy 14](https://www.finalfantasyxiv.com/) is copyright [SquareEnix](https://www.square-enix.com/). No assets or interaction with the game are used or done by this program.\
[PrimoCache](https://www.romexsoftware.com/en-us/primo-cache/) is copyright [Romex Software](https://www.romexsoftware.com). No interaction with PrimoCache is done by this program.\
Both are fine pieces of software I highly recommend.\

```
don't sue me
```