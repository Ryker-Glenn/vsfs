#include "fs.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

void mapfs(int fd){
  if ((fs = mmap(NULL, FSSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == NULL){
      perror("mmap failed");
      exit(EXIT_FAILURE);
  }
}

void unmapfs(){
  munmap(fs, FSSIZE);
}

void formatfs(){
	fs->SUPERBLOCK.free_blocks = BLOCKS;
	fs->SUPERBLOCK.free_inodes = TOTAL_NODES;
	createnode("root", "root", IS_DIR, 0, 0); 
	fs->nodes[0].i = 0;
	fs->nodes[0].parent = -1;
	for(int i = 1; i < TOTAL_NODES; i++)
		fs->nodes[i].parent = -1;
}

void loadfs(){

}

/* Basically just the start of the recursive function */
void lsfs(){
	printfs(0, "", 1);
}

/* Will start from the root, and if it's a directory
 * it'll keep passing the nodes associated with it until 
 * all the nodes are passed through. Root directory is the
 * start point. */
void printfs(int n, char *indent, int last) {
	printf("%s+-%s\n", indent, fs->nodes[n].file);

	char fi[FILENAME_SIZE];
	strcpy(fi, indent);

	(last == 1) ? strcat(fi, " ") : strcat(fi, "| ");

	if(fs->nodes[n].flag & IS_DIR) {
		for(int i = 0; i < fs->nodes[n].i; i++) {
			if(fs->nodes[n].blocks[i] > -1)
				printfs(fs->nodes[n].blocks[i], fi, (fs->nodes[n].i - i));
		}
	}
}

/* Adds the specified file to the system */
void addfilefs(char* fname){
	char *split;
	char full_path[FILENAME_SIZE], fcopy[FILENAME_SIZE], pdir[FILENAME_SIZE];
	int p_ind, i, nneed;

	/* a whole lot of stuff needed here, need to copy
	 * root directory to keep cycling through the file 
	 * path and replacing parent info. 
	 * Then copies the file argument so that it can be
	 * split up to find the number of nodes needed. */
	strcpy(pdir, fs->nodes[0].full);
	strcpy(fcopy, fname);
	full_path[0] = '\0';
	nneed = nodes_needed(fcopy);
	
	if(nneed == 0) err(6);
	if(nneed > fs->SUPERBLOCK.free_inodes) err(1);
	
	split = strtok(fname, "/");

	while(split != NULL) {
		struct stat path;
		i = 0;

		/* Super useful here, finds the parent index if it exists 
		 * then finds if the current (full) file exists 
		 * if the parent exists and the file doesn't, then 
		 * the parent and the file will be passed to create a new node */
		p_ind = exists(pdir);
		
		strcat(full_path, split);
		stat(full_path, &path);
		if(exists(full_path) == 0) {
			while(((fs->nodes[i].flag & IS_USED) || (fs->nodes[i].flag & IS_DIR))) i++;

			if(S_ISDIR(path.st_mode)) 
				add_dir(full_path, split, p_ind, i);
			else if(S_ISREG(path.st_mode))
				add_reg(full_path, split, p_ind, i);
		}
		strcpy(pdir, full_path);
		strcat(full_path, "/");
		split = strtok(NULL, "/");
	}
}

/* Finds the file by the full name and deletes it */
void removefilefs(char* fname){
	int p, i = exists(fname);
	
	if(i == 0) err(3);

	/* if the file is a directory, the contents of the directory will recursively 
	 * be deleted (if that file exists) then the blocks associated will be freed */
	if(fs->nodes[i].flag & IS_DIR) {
		for(int j = 0; j < fs->nodes[i].i; j++) {
			if(fs->nodes[i].blocks[j] > -1)
				removefilefs(fs->nodes[fs->nodes[i].blocks[j]].full);

			bfree(fs->nodes[i].blocks[j]);
		}
	}
	else if(fs->nodes[i].flag & IS_USED) {
		/* if the file is just a regular file, 
		 * the blocks associated will be freed */
		for(int j = 0; j < fs->nodes[i].i; j++)
			bfree(fs->nodes[i].blocks[j]);
	}
	
	p = reset_pblock(i);	// finds the node referenced in the parent block and negates it
	ifree(i);				// then frees the node

	/* if the parent directory is empty then it'll delete the parent 
	 * then something happens below it? idk but it works, so... eh */
	if(p >= 0 && dempty(p) == 0)
		removefilefs(fs->nodes[p].full);
}

/* Okay so basically it just finds the file with the full path 
 * and prints it to the file specified in the clr */
void extractfilefs(char* fname){
	int i = exists(fname);	

	if(i == 0) err(2);

	for(int j = 0; j < fs->nodes[i].i; j++)
		printf("%s",fs->blocks[fs->nodes[i].blocks[j]].data);
}

/* Gets the number of nodes necessary to fulfill the full request */
int nodes_needed(char *path) {
	char *split;
	int nodes = 0;
	char pdir[FILENAME_SIZE], full_path[FILENAME_SIZE];

	strcpy(pdir, fs->nodes[0].full);
	full_path[0] = '\0';
	split = strtok(path, "/");

	while(split != NULL) {
		strcat(full_path, split);
		if(exists(full_path) == 0)
			nodes++;

		strcpy(pdir, full_path);
		strcat(full_path, "/");
		split = strtok(NULL, "/");
	}

	return nodes;
}

/* super basic adding directory by adding the node's number to the parent's block list */
void add_dir(char *path, char *fname, int parent, int i) {
	add_parent(path, i, parent);	
	createnode(path, fname, IS_DIR, i, 1);
}

/* Add the child's node to the parent's block list */
void add_parent(char *file, int child, int parent) {
	int block = allocateblock();
	strcpy(fs->blocks[block].data, file);
	fs->nodes[parent].blocks[fs->nodes[parent].i++]= child;
	fs->nodes[child].parent = parent;
	fs->SUPERBLOCK.free_blocks--;
}

/* Adds regular files' data in blocks to the file system */
void add_reg(char *path, char *fname, int parent, int i) {
	FILE *file;
	char buf[BLOCK_SIZE];
	int b_ct, block, read;

	/* Opens the file if it exists, 
	 * adds the reference to the parent node, 
	 * and gets the block count if it exists */
	file = fopen(path, "r");
	if(!file) err(0);
	add_parent(path, i, parent);
	b_ct = getblocks(file);

	/* for all blocks necessary, read the data from the file 
	 * and copy it into the memory of the block type, then 
	 * add the block number to the nodes block list */
	for(int j = 0; j < b_ct; j++) {
		block = allocateblock();
		read = fread(buf, sizeof(char), BLOCK_SIZE, file);
		memcpy(fs->blocks[block].data, buf, read);
		fs->nodes[i].blocks[fs->nodes[i].i++] = block;
	}
	
	/* finish creating the node */
	createnode(path, fname, IS_USED, i, b_ct);
	
	fclose(file);
}

/* Set node n's flag, copy the file's info, 
 * subtract the nodes/blocks from the superblock*/
void createnode(char *path, char *file, char flag, int node, int blocks) {
	fs->nodes[node].flag |= flag;
	strcpy(fs->nodes[node].file, file);
	strcpy(fs->nodes[node].full, path);
	fs->SUPERBLOCK.free_inodes--;
	fs->SUPERBLOCK.free_blocks -= blocks;
}

/* Returns 0 if all of the nodes in a directory have been freed */
int dempty(int node) {
	for(int i = 0; i < fs->nodes[node].i; i++) {
		if(fs->nodes[node].blocks[i] > -1)
			return 1;
	}

	return 0;
}

int reset_pblock(int c) {
	int parent = fs->nodes[c].parent;
	for(int i = 0; i < fs->nodes[parent].i; i++) {
		if(fs->nodes[parent].blocks[i] > -1) {
			if(fs->nodes[parent].blocks[i] == c)
				fs->nodes[parent].blocks[i] = -1;
		}
	}
	return parent;
}

/* gets the size of the file in blocks */
int getblocks(FILE *file) {

	/* Sets the pointer to the end of
	 * the file and gets the file size,
	 * then resets it to the beginning */
	fseek(file, 0, SEEK_END);
	int file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	/* divides the file size by the block size */
	int blocks = file_size / BLOCK_SIZE;

	/* if there's any remainder it'll go into another block */
	if(file_size % BLOCK_SIZE > 0) blocks++;

	if(blocks > fs->SUPERBLOCK.free_blocks) err(5);

	return blocks;
}

/* Frees the block data and resets the flag in the free block list */
void bfree(int block) {
	memset(fs->blocks[block].data, 0, sizeof(fs->blocks[block].data));
	fs->free_blocks[block/8] &= ~(IS_USED << (block % 8));
	fs->SUPERBLOCK.free_blocks++;
}

/* Resets the flag of the inode specified */
void ifree(int inode) {
	fs->nodes[inode].flag &= ~fs->nodes[inode].flag;
	memset(fs->nodes[inode].file, 0, sizeof(fs->nodes[inode].file));
	memset(fs->nodes[inode].full, 0, sizeof(fs->nodes[inode].full));
	memset(fs->nodes[inode].blocks, 0, sizeof(fs->nodes[inode].blocks));
	fs->nodes[inode].i = 0;
	fs->nodes[inode].parent = -1;
	fs->SUPERBLOCK.free_inodes++;
}

/* Just some basic error messages */
void err(int opt) {
	switch(opt) {
		case 0: printf("File not found, Exiting\n");
				break;
		case 1: printf("No free iNodes, Exiting\n"); 
				break;
		case 2: printf("File not found for extracting, Exiting\n");
				break;
		case 3: printf("File not found for removal, Exiting\n");
				break;
		case 4: printf("No more free memory, Exiting\n");
				break;
		case 5: printf("Not enough free blocks, Exiting\n");
				break;
		case 6: printf("File already in system, Nothing to do\n");
				break;		
	}
	exit(1);
}

/* Will find the first free block in fbl and return its number (array position * 8 + offset) */
int allocateblock() {
	for (int i = 0; i < BLOCKS; i++){
		for(int j = 0; j < 8; j++){
			if((fs->free_blocks[i] & IS_USED << j) == 0){
				fs->free_blocks[i] |= IS_USED << j;
				fs->SUPERBLOCK.free_blocks--;
				return i * 8 + j;
			}
		}
	}
	err(4);
}

/* will return the inode number of the file if it's in the system, 0 otherwise */
int exists(char *fname) {
	for(int i = 0; i < TOTAL_NODES; i++) {
		if(strcmp(fs->nodes[i].full, fname) == 0)
			return i;
	}
	return 0;
}
