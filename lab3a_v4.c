#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h> 
#include "ext2_fs.h" 

char* filename; 
int fd;
int file; 

int superblock_offset = 1024;
int group_table_offset = 32;
int n_groups =0;

uint32_t buffer_32;
uint16_t buffer_16; 
uint8_t buffer_8;

/* globally hold info about superblock */
struct Superblock
{
  int n_blocks;
  int n_inodes;
  int block_size;
  int inode_size;
  int blocks_per_group;
  int inodes_per_group; 
  int nr_inode; 
};

/* globally hold infor about groups */
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
struct Group* group_arr; // hold array of groups' info  



/* print info to csv file and log to stderr */ 
void print_to_csv()
{
  /* SUPERBLOCK */ 
  fprintf(stderr, "SUPERBLOCK...\n");
  fprintf(stderr, "total number of blocks: %d\n", superblock.n_blocks);
  fprintf(stderr, "total number of i-nodes: %d\n", superblock.n_inodes); 
  fprintf(stderr, "block size: %d\n", superblock.block_size);
  fprintf(stderr, "i-node size: %d\n", superblock.inode_size);
  fprintf(stderr, "blocks per group: %d\n", superblock.blocks_per_group);
  fprintf(stderr, "i-nodes per group: %d\n", superblock.inodes_per_group);
  fprintf(stderr,"first non-reserved i-node: %d\n" , superblock.nr_inode);

  dprintf(file,"SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",superblock.n_blocks,superblock.n_inodes,superblock.block_size,superblock.inode_size,superblock.blocks_per_group, superblock.inodes_per_group,superblock.nr_inode);

  fprintf(stdout, "TESTING"); 

  /* GROUPS */ 
  fprintf(stderr, "GROUPS...\n");
  int i;
  for(i=0; i< n_groups; i++)
    {
      fprintf(stderr, "GROUP %d\n", i);
      fprintf(stderr, "group number: %d\n", group_arr[i].group_number);
      fprintf(stderr, "total blocks: %d\n", group_arr[i].n_blocks);
      fprintf(stderr, "total inodes: %d\n", group_arr[i].n_inodes);
      fprintf(stderr, "free blocks: %d\n", group_arr[i].n_fblocks);
      fprintf(stderr, "free inodes: %d\n", group_arr[i].n_finodes);
      fprintf(stderr, "free block bitmap: %d\n", group_arr[i].free_block_num);
      fprintf(stderr, "first block i-nodes: %d\n", group_arr[i].first_block_inode); 

      dprintf(file, "GROUP,%d,%d,%d,%d,%d,%d,%d\n", group_arr[i].group_number, group_arr[i].n_blocks, group_arr[i].n_inodes,
	      group_arr[i].n_fblocks, group_arr[i].n_finodes, group_arr[i].free_block_num, group_arr[i].first_block_inode); 
    }

  /* FREE BLOCK ENTRIES */
  // TODO

  /* FREE INODE ENTRIEDS */
  // TODO

  /* INODE SUMMARY */
  // TODO

  /* DIRECTORY ENTRIES */
  // TODO

  /* INDIRECT BLOCK REFERENCES */
  // TODO 
}

void superblock_summary()
{
  
  /* total blocks */ 
  pread(fd, &buffer_32, 4, superblock_offset + 4);
  superblock.n_blocks = buffer_32;

  /* total inodes */
  pread(fd, &buffer_32, 4, superblock_offset + 0);
  superblock.n_inodes = buffer_32; 
  
  /* block size */
  pread(fd, &buffer_32, 4, superblock_offset + 24);
  int block_size = 1024 << buffer_32;
  superblock.block_size = block_size; 
  
  /* inode size */
  pread(fd, &buffer_16, 2, superblock_offset + 88);
  superblock.inode_size = buffer_16; 
  
  /* blocks per group */
  pread(fd, &buffer_32, 4, superblock_offset + 32);
  superblock.blocks_per_group = buffer_32;

  /* inodes per group */
  pread(fd, &buffer_32, 4, superblock_offset + 40);
  superblock.inodes_per_group = buffer_32; 

  /* first non-reserved i-node */
  pread(fd, &buffer_32, 4, superblock_offset + 84);
  superblock.nr_inode = buffer_32;
  
}

void group_summary()
{

  int group_offset = superblock_offset + superblock.block_size;

  /* Find how many groups to loop through */
  n_groups = superblock.n_blocks / superblock.blocks_per_group;

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

      int num_blocks;
      double leftover_blocks = n_groups - ((double)superblock.n_blocks / superblock.blocks_per_group);

      /* number of blocks in group */
      if(leftover_blocks == 0)
	{
	  num_blocks = superblock.blocks_per_group;
	}
      else
	{
	  num_blocks = superblock.blocks_per_group - superblock.blocks_per_group * leftover_blocks; 

	}

      group.n_blocks = num_blocks; 
      
      /* number of inodes in group */
      int num_inodes;
      double leftover_inodes = ((double)superblock.n_inodes / superblock.inodes_per_group);
      if(leftover_inodes == 0)
	{
	  num_inodes = superblock.inodes_per_group; 
	}
      else
	{
	  num_inodes = superblock.inodes_per_group * leftover_inodes; 
	}
      
      group.n_inodes = num_inodes; 

      /* number of free blocks */ 
      int offset = group_offset + group_table_offset*i + 12; 
      pread(fd, &buffer_16, 2, offset);
      group.n_fblocks = buffer_16; 

      /* number of free inodes */
      offset = group_offset + group_table_offset*i + 14;
      pread(fd, &buffer_16, 2, offset);
      group.n_finodes = buffer_16; 

      /* block number of free block bitmap for this group */
      offset = group_offset + group_table_offset*i + 0;
      pread(fd, &buffer_32, 4, offset);
      group.free_block_num = buffer_32; 

      /* block number of free i-node bitmap for this group */
      offset = group_offset + group_table_offset*i + 4;
      pread(fd, &buffer_32, 4, offset);
      group.free_inode_num = buffer_32; 
      
      /* block number of first block of i-nodes in this group */ 
      offset = group_offset + group_table_offset*i + 8;
      pread(fd, &buffer_32, 4, offset);
      group.first_block_inode = buffer_32; 

      /* add group to global array */ 
      group_arr[i] = group;
    }

}


void free_blocks_summary()
{
  int i;
  for(i=0; i<n_groups; i++)
    {
      struct Group group = group_arr[i];
      fprintf(stderr, "Free Blocks...\n"); 

      // iterate through block 
      int j;
      for(j=0; j < superblock.block_size;j++)
	{
	  int offset = group.free_block_num * superblock.block_size + j;
	  pread(fd, &buffer_8, 1, offset);
	  int8_t bitmask = 1; 
	  // iterate through the byte
	  int k;
	  for(k=1; k<=8; k++)
	    {
	      int result = buffer_8 & bitmask; 
	      //	      fprintf(stderr, "Result: %d\n", result); 
	      // check if free ( == 0)
	      if(result == 0)
		{
		  fprintf(stderr,"BFREE,"); 
		  /* number of the free block */ 
		  int block_num = (j*8)+k+(i*superblock.blocks_per_group); 
		  fprintf(stderr, "number of free block: %d\n", block_num); 
		}
	      // move bitmask down a bit 
	      bitmask = bitmask << 1; 

	    }

	}
      
    }
}


void free_inode_summary()
{
  int i;
  for(i=0; i<n_groups; i++)
    {
      struct Group group = group_arr[i];
      fprintf(stderr, "Free Inodes...\n");

      // iterate through block
      int j;
      for(j=0; j < superblock.block_size;j++)
        {
          int offset = group.free_inode_num * superblock.block_size + j;
          pread(fd, &buffer_8, 1, offset);
          int8_t bitmask = 1;
          // iterate through the byte
          int k;
          for(k=1; k<=8; k++)
            {
              int result = buffer_8 & bitmask;
              //              fprintf(stderr, "Result: %d\n", result);
              // check if free ( == 0)
              if(result == 0)
                {
                  fprintf(stderr,"IFREE,");
                  /* number of the free block */
                  int block_num = (j*8)+k+(i*superblock.blocks_per_group);
                  fprintf(stderr, "number of free block: %d\n", block_num);
                }
              // move bitmask down a bit
              bitmask = bitmask << 1;

            }

        }

    }
}

void inode_summary()
{  
  // TODO  
}


void directory_summary()
{
  // TODO
}


void indirect_block_summary()
{
  // TODO 
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
    char* out_file = "lab3a.csv";
    file = open(out_file, O_CREAT | O_WRONLY | O_NONBLOCK, 0666); 

  /* get summary of file system */ 
  superblock_summary();

  group_summary(); 
  
  free_blocks_summary(); 
  
  free_inode_summary(); 

  inode_summary();

  directory_summary();

  indirect_block_summary(); 

  /* log summary to csv and stderr */ 
  print_to_csv(); 

   close(file);
  
  return 0; 
}
