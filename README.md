A file system scheme that I developed. The file system itself can be used for any system, although I developed and tested it with TI's TM4C123 chip. I utilized the launchpad version for my prototyping, and interfaced it with a ST7735 LCD. This LCD also includes an attached SD card reader, and by doing interfacing it to my TM4C, I was able to exercise and verify the file system's functionality. The code in this repo also includes this SD IO driver code as well.

The code was a part of my TM4C Real-Time OS (before I switched to FAT32), so there's a lot of calls to the OS that needs to be re-plugged into your OS.

For a very detailed breakdown of this filesystem, please consult the osfs_exp.pdf file included in this repo.

