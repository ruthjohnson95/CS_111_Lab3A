#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h> 

char* filename; 
int fd;

int superblock_offset = 1024;
int group_table_offset = 32;

uint32_t buffer_32;
uint16_t buffer_16; 

struct Superblock
{
  int n_blocks;
  int n_inodes;
  int block_size;
  int inode_size;
  int blocks_per_group;
  int inodes_per_group; 
  int nr_idnode; 
};

struct Group
{
  int group_number;
  int n_blocks;
  int n_inodes;
  int n_fblocks;
  int n_finodes;
  int free_block_num;
  int free_inode_num;
  int first_block_inode; 
};


struct Superblock superblock; 
struct Group* group_arr; 

void superblock_summary()
{
  fprintf(stdout, "Superblock...\n"); 
  
  /* total blocks */ 
  pread(fd, &buffer_32, 4, superblock_offset + 4);
  superblock.n_blocks = buffer_32;
  fprintf(stdout, "%d\n", buffer_32);

  /* total inodes */
  pread(fd, &buffer_32, 4, superblock_offset + 0);
  superblock.n_inodes = buffer_32; 
  fprintf(stdout, "%d\n", buffer_32);
  
  /* block size */
  pread(fd, &buffer_32, 4, superblock_offset + 24);
  int block_size = 1024 << buffer_32;
  superblock.block_size = block_size; 
  fprintf(stdout, "%d\n", block_size);
  
  /* inode size */
  pread(fd, &buffer_16, 2, superblock_offset + 88);
  superblock.inode_size = buffer_16; 
  fprintf(stdout, "%d\n", buffer_16); 
  
  /* blocks per group */
  pread(fd, &buffer_32, 4, superblock_offset + 32);
  superblock.blocks_per_group = buffer_32;
  fprintf(stdout, "%d\n", buffer_32); 

  /* inodes per group */
  pread(fd, &buffer_32, 4, superblock_offset + 40);
  superblock.inodes_per_group = buffer_32; 
  fprintf(stdout, "%d\n", buffer_32); 

  /* first non-reserved i-node */
  // TODO
  // ???????? 

}

void group_summary()
{

  fprintf(stdout, "Groups...\n"); 
  
  int group_offset = superblock_offset + superblock.block_size;

  /* Find how many groups to loop through */
  // block count / blocks per group
  int n_groups = superblock.n_blocks / superblock.blocks_per_group;

  // if there's a remainder, add 1
  if(superblock.n_blocks % superblock.blocks_per_group != 0 )
    {
      n_groups = n_groups + 1; 
    }

  /* initialize array for groups */
  group_arr = malloc(n_groups*sizeof(struct Group));

  int i;
  for(i=0; i<n_groups; i++)
    {
      /* initialize group structs */
      struct Group group;
      
      /* group number */ 
      group.group_number = i; 
      fprintf(stdout, "%d\n", i);

      int num_blocks;
      int leftover_blocks = superblock.n_blocks % superblock.blocks_per_group;
      /* number of blocks in group */
      if(leftover_blocks == 0)
	{
	  num_blocks = superblock.blocks_per_group;
	}
      else
	{
	  num_blocks = superblock.blocks_per_group * (n_groups - ((double)superblock.n_blocks / superblock.blocks_per_group));

	}

      group.n_blocks = num_blocks; 
      fprintf(stdout, "%d\n", num_blocks); 
      
      /* number of inodes in group */
      int num_inodes = 0;
      group.n_inodes = num_inodes; 
      fprintf(stdout, "%d\n", num_inodes); 


      /* number of free blocks */ 
      int offset = group_offset + group_table_offset*i + 12; 
      pread(fd, &buffer_16, 2, offset);
      group.n_fblocks = buffer_16; 
      fprintf(stdout, "%d\n", buffer_16); 

      /* number of free inodes */
      offset = group_offset + group_table_offset*i + 14;
      pread(fd, &buffer_16, 2, offset);
      group.n_finodes = buffer_16; 
      fprintf(stdout, "%d\n", buffer_16);

      /* block number of free block bitmap for this group */
      offset = group_offset + group_table_offset*i + 0;
      pread(fd, &buffer_32, 4, offset);
      group.free_block_num = buffer_32; 
      fprintf(stdout,"%d\n", buffer_32); 

      /* block number of free i-node bitmap for this group */
      offset = group_offset + group_table_offset*i + 4;
      pread(fd, &buffer_32, 4, offset);
      group.free_inode_num = buffer_32; 
      fprintf(stdout,"%d\n", buffer_32);
      
      /* block number of first block of i-nodes in this group */ 
      offset = group_offset + group_table_offset*i + 8;
      pread(fd, &buffer_32, 4, offset);
      group.first_block_inode = buffer_32; 
      fprintf(stdout,"%d\n", buffer_32);
      

      group_arr[i] = group;
    }

}

void free_blocks()
{

}


int main(int argc, char **argv)
{
  /* read in file system name */
  
  if(argc !=2)
    {
      fprintf(stderr, "Error: wrong number of command line args\n");
      exit(1); 
    }
  else
    {
      filename = malloc((strlen(argv[1]) + 1) * sizeof(char)); 
      filename=argv[1]; 
    }


  fd = open(filename, O_RDONLY);

  /* create csv file */
  
  
  superblock_summary();

  group_summary(); 
  
  free_blocks(); 
  
  printf("hello world!\n");
  
  return 0; 
}
