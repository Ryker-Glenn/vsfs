all:
	gcc -g sfs.c filefs.c -o filefs

clean:
	rm filefs
