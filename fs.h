#ifndef __FS_H__
#define __FS_H__
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FSSIZE 		 		10000000
#define BLOCK_SIZE	 		512
#define FILENAME_SIZE		256
#define BLOCKS				10000	
#define TOTAL_NODES		 	100
#define BLOCKS_PER_INODE 	100
#define BLOCK_LIST_SIZE		(BLOCKS / 8)
#define IS_USED 			0x1
#define IS_DIR  			0x2

struct superblock {
	int 	free_blocks;
	int		free_inodes;
};

struct block{
	char 	data[BLOCK_SIZE];
};

struct inode {
	char 	file[FILENAME_SIZE], full[FILENAME_SIZE];
	char	flag;
	int 	i, parent;
	int 	blocks[BLOCKS_PER_INODE];
};

struct fs {
	struct 	superblock SUPERBLOCK;
	char 	free_blocks[BLOCK_LIST_SIZE];
	struct 	inode nodes[TOTAL_NODES];
	struct 	block blocks[BLOCKS];
};

struct fs *fs;

void mapfs(int fd);
void unmapfs();
void formatfs();
void loadfs();
void lsfs();
void addfilefs(char* fname);
void removefilefs(char* fname);
void extractfilefs(char* fname);

 /****************
 *	 ls helpers  *
 ****************/
void printfs(int, char*, int);

 /****************************
 *	adding addfilefs helpers *
 ****************************/
int allocateblock();
int nodes_needed(char*);
void add_parent(char*, int, int);
int exists(char*);
void createnode(char*, char*, char, int, int);

 /********************
 *	adding directory *
 ********************/
void add_dir(char*, char*, int, int);

 /************************
 *	adding regular files *
 ************************/
void add_reg(char*, char*, int, int);
int getblocks(FILE*);

 /***********************
 *	delete file helpers	*
 ***********************/
void bfree(int);										
void ifree(int);
int dempty(int);
int reset_pblock(int);
void err(int);

#endif
