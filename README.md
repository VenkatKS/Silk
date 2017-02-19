A file system scheme that I developed. The file system itself can be used for any system, although I originally developed and tested it with TI's TM4C123 chip. I utilized the launchpad version for my prototyping, and interfaced it with a ST7735 LCD. This LCD also includes an attached SD card reader, and by doing interfacing it to my TM4C, I was able to exercise and verify the file system's functionality. The code in this repo also includes this SD IO driver code as well. These TM4C specific files are found within TM4C_CODE/.

The TM4C_CODE/ code was a part of my TM4C Real-Time OS (before I switched to FAT32), so there's a lot of calls to the OS that needs to be re-plugged into your OS.

For exercising the file system alone, please see the XCode project found within SYSTEM_TEST/. The c files within SYSTEM_TEST/FileSystemTestBench/ can be compiled using XCode/gcc in an Unix environment. Modifications have been made to the original TM4C specific code so that it runs on any Unix environment. These files create a spoofed disk file (called "myfilesystem.store") on the host OS and uses that to store files that are created using my filesystem. My file system can be interfaced with using the included interpreter (typing 'help' when the program starts up will give all commands).

For a very detailed breakdown of this filesystem, please consult the osfs_exp.pdf file included in this repo.

