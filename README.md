# Silk

Silk is a self-contained application that spoofs a shell interpreter and implements my own filesystem scheme for file storage. It's a fun project I worked on to learn more about how filesystems are managed within the operating system. The heart of the application is the filesystem scheme implemented within *src/OS_FileSystemScheme.c/h*.

The entire project, along with the filesystem design, was designed and written from scratch. My filesystem scheme is based off the EXT2 filesystem. This project was originally designed for my N108 OS, before I switched to using FAT32.

## Silk Specifics
As explained above, the program spoofs a shell interpreter and implements a back-end filesystem. When run for the first time, Silk creates a file in the host OS's filesystem named *myfilesystem.store.* My entire filesystem -- and any files created within my filesystem -- is stored within this file.

### Building Silk
Silk is very simple to build. Just navigate to where you cloned the repository, cd into *src/* and run make. After doing so, execute *silk.o* found within *src/build*. 

### Running Silk
Once the binary is built and run, you should encounter this prompt on your terminal:
```
Venkats-MacBook-Pro:src Venkat$ make
mkdir -p build/
gcc -o build/silk.o *.c
Venkats-MacBook-Pro:src Venkat$ cd build/
Venkats-MacBook-Pro:build Venkat$ ./silk.o 
Please enter command:
```
At this point, the shell interpreter has launched. Treat this as a very barebones OS that only supports the very basic filesystem commands. The idea is to just showcase the functionality of my filesystem scheme. To see the commands avaliable, enter help.

```
Please enter command:
help

Command Information:

help:
 Output command information.

creat:
 Creates a new file and allows writes to it.

del:
 Deletes a file

app:
 Appends attached string to the end of the file

printfile:
 Prints content of file

ls:
 List all the files on the SD card.

format:
 Formats the entire filesystem.

Command Formats: 

help
creat <filename>
del <filename>
app <filename> <string>
printfile <filename>
format
ls

Please enter command: 
```
From the above, we can see that the program supports the basic commands: ls, creat, del, app, format, and printfile. Let's play around with it; let's create a file and write a couple words to it.

```
Please enter command: 
creat welc.txt

Created.

Please enter command: 
app welc.txt hey!

Appended.


Please enter command: 
```
Cool! Let's see if it actually works:
```
Please enter command: 
ls

welc.txt :: 4  bytes

Please enter command: 
printfile welc.txt

hey!

Please enter command: 
```
Great! Looks like the filesystem scheme is working. Let's create a couple more:
```
Please enter command: 
creat hey1.txt

Created.


Please enter command: 
app hey1.txt iam14byteslong

Appended.


Please enter command: 
creat hey2.txt

Created.


Please enter command: 
app hey2.txt yothisis19byteslong

Appended.


Please enter command: 
ls

welc.txt :: 4  bytes
hey1.txt :: 14  bytes
hey2.txt :: 19  bytes


Please enter command: 

```
Finally, the most important thing about a filesystem is non-volatility. The datastore should work even when I close the program and restart it. Let's try that:
```
Please enter command: 
^C
Venkats-MacBook-Pro:build Venkat$ ./silk.o 
Please enter command:
ls

welc.txt :: 4  bytes
hey1.txt :: 14  bytes
hey2.txt :: 19  bytes

Please enter command: 
printfile hey1.txt

iam14byteslong

Please enter command: 

```
Cool, looks like everythings working fine! Let's test out the formatting ability:
```
Please enter command: 
ls

welc.txt :: 4  bytes
hey1.txt :: 14  bytes
hey2.txt :: 19  bytes

Please enter command: 
format

Formatting the filesystem.

Please restart device.

Please enter command: 
ls

Please enter command: 

```
Nice. So everything seems to work.

### Documenting Silk
The documentation for the filesystem design can be found under *docs/*. It's a PDF that goes into great detail about the internals of my filesystem scheme, such as how I manage free space, how each file marks the blocks it uses, etc. It's definitely worth a read if you're interested in learning how a basic filesystem works.
### Testing Silk
Obviously running the program through some simple user tests isn't going to exercise the filesystems' robustness. Therefore, I'm currently working on a couple of tests to automatically test the sanity of the implementation by creating and deleting thousands of files. TBD.

