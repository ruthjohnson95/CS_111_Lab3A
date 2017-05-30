#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h> 
#include "ext2_fs.h" 
#include <errno.h>
#include <time.h>

char* filename; 
int fd;
int file; 

int superblock_offset = EXT2_MIN_BLOCK_SIZE;
int group_table_offset = 32;
int n_groups =0;

struct ext2_super_block super_block; 
struct ext2_group_desc* ext2_group_desc_arr; 

struct ext2_inode* inode_array; // holds array of inodes info
int* validInodeIndex_array;
int validInodeCount = 0;
int* validDirectoryIndex_array;
int validDirecotyCount = 0;


__u32 buffer_32, buf_u32;
__u16 buffer_16, buf_u16; 
__u8  buffer_8;
__s32 buf_s32;
__s16 buf_s16;

int blocks_in_group()
{
  int n_blocks=super_block.s_blocks_count%super_block.s_blocks_per_group;  
  return n_blocks; 
}

/* print info to csv file and log to stderr */ 
void print_to_csv()
{
  /* SUPERBLOCK */ 
  fprintf(stderr, "SUPERBLOCK...\n");
  fprintf(stderr, "total number of blocks: %d\n", super_block.s_blocks_count);
  fprintf(stderr, "total number of i-nodes: %d\n", super_block.s_inodes_count); 

  __u32 block_size = EXT2_MIN_BLOCK_SIZE << super_block.s_log_block_size;

  fprintf(stderr, "block size: %d\n", block_size);
  fprintf(stderr, "i-node size: %d\n", super_block.s_inodes_count);
  fprintf(stderr, "blocks per group: %d\n", super_block.s_blocks_per_group);
  fprintf(stderr, "i-nodes per group: %d\n", super_block.s_inodes_per_group);
  fprintf(stderr,"first non-reserved i-node: %d\n" , super_block.s_first_ino);
  fprintf(stdout,"SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",super_block.s_blocks_count,super_block.s_inodes_count,block_size, super_block.s_inode_size, super_block.s_blocks_per_group, super_block.s_inodes_per_group ,super_block.s_first_ino); 

  /* GROUPS */ 
  fprintf(stderr, "GROUPS...\n");
  int i;
  for(i=0; i< n_groups; i++)
    {
      fprintf(stderr, "GROUP %d\n", i);
      fprintf(stderr, "group number: %d\n", i);
      fprintf(stderr, "total blocks: %d\n", blocks_in_group());
      fprintf(stderr, "total inodes: %d\n", super_block.s_inodes_per_group);
      fprintf(stderr, "free blocks: %d\n", ext2_group_desc_arr[i].bg_free_blocks_count);
      fprintf(stderr, "free inodes: %d\n", ext2_group_desc_arr[i].bg_free_inodes_count);
      fprintf(stderr, "free block bitmap: %d\n", ext2_group_desc_arr[i].bg_block_bitmap);
      fprintf(stderr, "free i-node bitmap:: %d\n", ext2_group_desc_arr[i].bg_inode_bitmap);
      fprintf(stderr, "first block i-nodes: %d\n", ext2_group_desc_arr[i].bg_inode_table); 

      fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", i, blocks_in_group(),super_block.s_inodes_per_group, ext2_group_desc_arr[i].bg_free_blocks_count, ext2_group_desc_arr[i].bg_free_inodes_count, ext2_group_desc_arr[i].bg_block_bitmap, ext2_group_desc_arr[i].bg_inode_bitmap, ext2_group_desc_arr[i].bg_inode_table);
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
  super_block.s_blocks_count = buffer_32;

  /* total inodes */
  pread(fd, &buffer_32, 4, superblock_offset + 0);
  super_block.s_inodes_count = buffer_32; 
  
  /* block size */
  pread(fd, &buffer_32, 4, superblock_offset + 24);
  __u32 block_size = 1024 << buffer_32;
  //   superblock.block_size = block_size; 
  super_block.s_log_block_size = EXT2_MIN_BLOCK_SIZE >> block_size;

  /* inode size */
  pread(fd, &buffer_16, 2, superblock_offset + 88);
  super_block.s_inode_size = buffer_16; 
  
  /* blocks per group */
  pread(fd, &buffer_32, 4, superblock_offset + 32);
  super_block.s_blocks_per_group = buffer_32;

  /* inodes per group */
  pread(fd, &buffer_32, 4, superblock_offset + 40);
  super_block.s_inodes_per_group = buffer_32; 

  /* first non-reserved i-node */
  pread(fd, &buffer_32, 4, superblock_offset + 84);
  super_block.s_first_ino = buffer_32;

   block_size = EXT2_MIN_BLOCK_SIZE << super_block.s_log_block_size;
  
  fprintf(stdout,"SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",super_block.s_blocks_count,super_block.s_inodes_count,block_size, super_block.s_inode_size, super_block.s_blocks_per_group, super_block.s_inodes_per_group ,super_block.s_first_ino);

}

void group_summary()
{
  __u32 block_size = EXT2_MIN_BLOCK_SIZE << super_block.s_log_block_size;

  int group_offset = superblock_offset + block_size;

  /* Find how many groups to loop through */
  n_groups = super_block.s_blocks_count / super_block.s_blocks_per_group;

  // if there's a remainder, add 1
  if(super_block.s_blocks_count % super_block.s_blocks_per_group != 0 )
    {
      n_groups = n_groups + 1; 
    }

  
  /* initialize array for groups */
  ext2_group_desc_arr = malloc(n_groups*sizeof(struct ext2_group_desc)); 

  int i;
  for(i=0; i<n_groups; i++)
    {
      /* initialize group structs */
      struct ext2_group_desc group_desc;

      /* number of free blocks */ 
      int offset = group_offset + group_table_offset*i + 12; 
      pread(fd, &buffer_16, 2, offset);
      group_desc.bg_free_blocks_count = buffer_16; 
      
      /* number of free inodes */
      offset = group_offset + group_table_offset*i + 14;
      pread(fd, &buffer_16, 2, offset);
      group_desc.bg_free_inodes_count = buffer_16; 

      /* block number of free block bitmap for this group */
      offset = group_offset + group_table_offset*i + 0;
      pread(fd, &buffer_32, 4, offset);
      group_desc.bg_block_bitmap = buffer_32; 

      /* block number of free i-node bitmap for this group */
      offset = group_offset + group_table_offset*i + 4;
      pread(fd, &buffer_32, 4, offset);
      group_desc.bg_inode_bitmap = buffer_32; 
      
      /* block number of first block of i-nodes in this group */ 
      offset = group_offset + group_table_offset*i + 8;
      pread(fd, &buffer_32, 4, offset);
      group_desc.bg_inode_table = buffer_32; 

      /* add group to global array */ 
      ext2_group_desc_arr[i] = group_desc; 
      fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", i, blocks_in_group(),super_block.s_inodes_per_group, ext2_group_desc_arr[i]. bg_free_blocks_count, ext2_group_desc_arr[i].bg_free_inodes_count, ext2_group_desc_arr[i].bg_block_bitmap, ext2_group_desc_arr[i].bg_inode_bitmap, ext2_group_desc_arr[i].bg_inode_table);
   }

}


void free_blocks_summary()
{
  int i;
  for(i=0; i<n_groups; i++)
    {
      struct ext2_group_desc group_desc  = ext2_group_desc_arr[i];

      //      fprintf(stderr, "Free Blocks...\n"); 

      // iterate through block 
      int j;
      int block_size = EXT2_MIN_BLOCK_SIZE << super_block.s_log_block_size; 
      for(j=0; j < block_size;j++)
	{
	  int offset = group_desc.bg_block_bitmap * block_size + j;
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
		  /* number of the free block */ 
		  int block_num = (j*8)+k+(i*super_block.s_blocks_per_group); 
		  fprintf(stdout, "BFREE,%d\n", block_num); 
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
      struct ext2_group_desc group_desc  = ext2_group_desc_arr[i]; 

      //      fprintf(stderr, "Free Inodes...\n");
      __u32 block_size = EXT2_MIN_BLOCK_SIZE << super_block.s_log_block_size;

      // iterate through block
      int j;
      for(j=0; j < block_size;j++)
        {
          int offset = group_desc.bg_inode_bitmap * block_size + j;

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
                  /* number of the free block */
                  int block_num = (j*8)+k+(i*super_block.s_blocks_per_group);
                  fprintf(stdout, "IFREE,%d\n", block_num);
                }
              // move bitmask down a bit
              bitmask = bitmask << 1;

            }

        }

    }
}

void inode_summary()
{  
  int num_inodes = super_block.s_inodes_count;
  int i,j;

  inode_array = malloc (sizeof(struct ext2_inode) * num_inodes);
  validInodeIndex_array = malloc (sizeof(int)*num_inodes);
  validDirectoryIndex_array = malloc (sizeof(int)*num_inodes);

  inode_array = malloc (sizeof(struct ext2_inode) * num_inodes);
  validInodeIndex_array = malloc (sizeof(int)*num_inodes);
  validDirectoryIndex_array = malloc (sizeof(int)*num_inodes);

  for(i=0; i<n_groups; i++) {
    // TODOS -- change
    for (j=0; j< num_inodes; j++) {

      __u32 block_size = EXT2_MIN_BLOCK_SIZE << super_block.s_log_block_size;
      int offset = ext2_group_desc_arr[i].bg_inode_table * block_size + j* super_block.s_inode_size ;

      //get link count -- 7
      pread(fd, &buf_u16, 2, offset+26);
      inode_array[j].i_links_count= buf_u16;

      // get mode  -- 4
      pread(fd, &buf_u16, 2, offset);
      inode_array[j].i_mode = buf_u16 & 0x01FF;
      

      if (inode_array[j].i_mode != 0 && inode_array[j].i_links_count!=0) {

	char c; 
	if ((buf_u16 & 0x8000) )
          {
            c = 'f';
	    validInodeIndex_array[validInodeCount] = j;
	    validInodeCount ++;

          }
	else if ((buf_u16 & 0xA000)) {
	  c = 's';
	}
	else if (buffer_16 & 0x4000){
	  c = 'd';
	  validDirectoryIndex_array[validDirecotyCount] = j;
	  validDirecotyCount ++;

	  validInodeIndex_array[validInodeCount] = j;
	  validInodeCount ++;
	}
	else {
	  c = '?';
	}


	//getowner -- 5
	pread(fd, &buf_u16, 2, offset+2);
	inode_array[j].i_uid = buf_u16;

	// get group --6
	pread(fd, &buf_u16, 2, offset+24);
	inode_array[j].i_gid = buf_u16;

	// get time of last I-node change (mm/dd/yy hh:mm:ss, GMT) -- 8
	pread(fd, &buf_u32, 4, offset+12);
	inode_array[j].i_ctime = buf_u32;
	char change_time[80];
	time_t c_rawtime = (time_t)buf_u32;
	strftime (change_time,80,"%D %T",gmtime(&c_rawtime));
      
	// get time of modification time (mm/dd/yy hh:mm:ss, GMT) -- 9
	pread(fd, &buf_u32, 4, offset+16);
	inode_array[j].i_mtime = buf_u32;
	char mod_time[80];
	time_t m_rawtime = (time_t)buf_u32;     
	strftime (mod_time,80,"%D %T",gmtime(&m_rawtime));
          
	// get time of last access -- 10
	pread(fd, &buf_u32, 4, offset+8);
	inode_array[j].i_atime = buf_u32;
	char acc_time[80];
	time_t a_rawtime = (time_t)buf_u32;
	struct tm timeinfo; 
	gmtime_r(&a_rawtime, &timeinfo);
	strftime (acc_time,80,"%D %T",&timeinfo);
  
	// get file size (decimal) -- 11
	pread(fd, &buf_s32, 4, offset+4);
	inode_array[j].i_size = buf_s32;
          
	// get number of blocks (decimal) --12
	pread(fd, &buf_s32, 4, offset+28);
	inode_array[j].i_blocks = buf_s32;

	fprintf(stdout, "%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d", j+1,c, inode_array[j].i_mode, inode_array[j].i_uid , inode_array[j].i_gid,inode_array[j].i_links_count, change_time, mod_time,acc_time,inode_array[j].i_size, inode_array[j].i_blocks  );

	//print blocks information 
	int m = 0;
	pread(fd, &buf_s32, 4, offset+40);
	//inode_array[j].i_block = buf_s32;
	for (m=0; m <15; m++){
	  pread(fd, &buf_s32, 4, offset+40 + m*4);
	  inode_array[j].i_block[m] = buf_s32;
	  fprintf(stdout, ",%d", inode_array[j].i_block[m]);
	}
	
	fprintf(stdout, "\n");
      }
      
    }
    
  }
  
}


void directory_block_info (int block_num, int parent_inode_num, int global_offset){
{
  __u32 block_size = EXT2_MIN_BLOCK_SIZE << super_block.s_log_block_size;
  //find block offset 
  int block_offset = block_size * block_num;
  
  int accumulate_offset = 0;  // the offset within block 

  while ( accumulate_offset + 9 <= block_size) {// the length of rec_len + acc < blcok size 
  
    pread (fd, &buf_u16, 2, block_offset+accumulate_offset + 4); 
    if (buf_u16+ accumulate_offset > block_size){
      fprintf(stderr, "File is not complete\n");
      exit(1);
    }
    struct ext2_dir_entry * temp_entry = malloc (buf_u16);
    if (temp_entry == NULL) {
      fprintf(stderr, "ERROR during malloc\n");
      exit(2);
    }
    pread (fd, temp_entry, buf_u16, block_offset+accumulate_offset); // the temp_entry now has data 
    
    if (buf_u16 <= 0){break;}

    accumulate_offset += buf_u16;
    
    if ((temp_entry->inode) == 0) break;
    // print out the entries 
    fprintf(stderr, "DIRENT,%d,%d,%d,%d,%d,'%s'\n",parent_inode_num, global_offset+accumulate_offset - buf_u16, temp_entry -> inode, temp_entry ->rec_len, temp_entry->name_len, temp_entry ->name);
    
  }
 }
}
 void directory_summary() {

   int i, m;
   for (i =0; i < validDirecotyCount ; i ++){

     for (m = 0; m< 12; m++) {

       int block_num = inode_array[validDirectoryIndex_array[i]].i_block[m];
       directory_block_info( block_num, validDirectoryIndex_array[i]+1, 0 );
     }
    
   }
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

    if(fd < 0 )
      {
	fprintf(stderr, "Error opening file: %s\n", strerror(errno)); 
	exit(1); 
      }

  /* get summary of file system */ 
  superblock_summary();

  group_summary(); 
  
  free_blocks_summary(); 
  
  free_inode_summary(); 

  inode_summary();

  directory_summary();

  indirect_block_summary(); 

  /* log summary to csv and stderr */ 
  //  print_to_csv(); 

  
  return 0; 
}
