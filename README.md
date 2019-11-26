Very Simple File System

This is code I wrote for my operating systems class, it implements the "grandaddy of file systems".

At its core, this file system is able to take names of files specified as command line arguments and add them to the file system if they exist. Regular files are broken down into 512b chunks and allocated accordingly. A file path is broken down based on the "/" delimiter, and if the file at the tail of the path exists and is a regular file, it will be added as such. Files that don't actually exist at the end of a path are added as directory. 

Directories are added naively (and probably not exactly correctly, but it works so... *shrug*). When a file path is split, generally if a file exists as a directory it will come before a regular file, so any file name that comes before the end is a directory. When a directory is added, the inode number is added to the parent directory's list of block references (even though it isn't a block). Nodes can be discerned as either being files or directories by its associated flag. Any subsequent files added after that directory in the path are added to its list of block references.

Usage:

Addition
	./filefs -a [filepath] -f [fs name]

Extraction
	./filefs -e [filepath] -f [fs name]

Deletion
	./filefs -r [filepath] -f [fs name]
