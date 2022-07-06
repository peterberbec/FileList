FileList.exe

Reads the data of all files, recursively, in target directory. Loops forever, with a 
user prompt to quit after every loop, or until read is accomplished within a goal time. 
Inability to access a file is currently set to NOT stop the program; it just moves on
to the next file.

The purpose of this program is to load an SSD cache from a hard drive. I wrote it
to load PrimoCache with FF14, as it was doing a bad job, averaging a sub-1% cache hit
rate. After running this program on bootup, and letting it go for about 4 minutes, my 
cache hit rate is now over 99%.

The program grew from there. Now it takes the directory as a command-line argument. There
is a ""fancy" rsync-style display. I didn't figure out ncurses, so I hacked together a
line-clearing method. This integrates into the Windows "Send-To" menu, so you can run it
from File Explorer.