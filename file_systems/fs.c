#include "fs.h"

#define FILE_NUM 64
#define INODE_NUM 65
#define FD_NUM 32

struct super_block{
  uint16_t used_block_bitmap_count;
  uint16_t used_block_bitmap_offset;
  uint16_t inode_metadata_blocks;
  uint16_t inode_metadata_offset;
};

struct inode{
  bool is_used;
  uint16_t direct_offset[10];
  uint16_t single_indirect_offset;
  uint16_t double_indirect_offset;
  unsigned long int file_size;
  int reference_count;
};

struct dir_entry{
  bool is_used;
  int inode_number;
  char name[15];
};

struct file_descriptor{
  bool is_used;
  int inode_number;
  unsigned long int offset;
};

struct super_block mounted_super_block;
uint8_t mounted_block_bitmap[DISK_BLOCKS / 8];
struct inode mounted_inodes[INODE_NUM];
struct dir_entry mounted_directory[FILE_NUM];
struct file_descriptor mounted_files[FD_NUM];
char working_buffer[BLOCK_SIZE];

int is_mounted = 0;
int is_init = 0;
void init(){
  for(int i = 0; i < FD_NUM; i++){
    mounted_files[i].is_used = 0;
    mounted_files[i].inode_number = -1;
    mounted_files[i].offset = -1;
  }
  for(int i = 0; i < FILE_NUM; i++){
    mounted_directory[i].is_used = 0;
    mounted_directory[i].inode_number = -1;
  }
  is_init = 1;
}

void set_block(int index, uint8_t (*block_bitmap)[DISK_BLOCKS / 8]){
  int byte_location = index / 8;
  int remainder = index % 8;
  (*block_bitmap)[byte_location] |= (1 << remainder);
}

void clear_block(int index, uint8_t (*block_bitmap)[DISK_BLOCKS / 8]){
  int byte_location = index / 8;
  int remainder = index % 8;
  (*block_bitmap)[byte_location] &= ~(1 << remainder);
}

int make_fs(const char *disk_name){
  //opening disk
  if(make_disk(disk_name)){
    perror("Problem making disk");
    return -1;
  }
  if(open_disk(disk_name)){
    perror("Problem making disk");
    return -1;
  }

  if(!is_init){
    init();
  }
  
  //initializing disk metadata structures and their required block sizes
  struct super_block *disk_super_block = calloc(1, sizeof(struct super_block));
  if(!disk_super_block){
    perror("calloc failed for disk_super_block");
    return -1;
  }

  struct dir_entry *disk_directory = calloc(FILE_NUM, sizeof(struct dir_entry));
  if(!disk_directory){
    perror("calloc failed for disk_directory");
    return -1;
  }

  uint8_t *disk_block_bitmap = calloc((DISK_BLOCKS / 8), sizeof(uint8_t));
  if(!disk_block_bitmap){
    perror("calloc failed for disk_block_bitmap");
    return -1;
  }
  
  struct inode *disk_inodes = calloc(INODE_NUM, sizeof(struct inode));
  if(!disk_inodes){
    perror("calloc failed for disk_inodes");
    return -1;
  }
  
  //setting values of superblock
  disk_super_block -> used_block_bitmap_count = 4;
  disk_super_block -> used_block_bitmap_offset = 1;
  disk_super_block -> inode_metadata_blocks = 1;
  disk_super_block -> inode_metadata_offset = 2;
  
  //setting values of block bitmap to used
  for(int i = 0; i < disk_super_block -> used_block_bitmap_count; i++){
    set_block(i, (uint8_t (*)[DISK_BLOCKS / 8])disk_block_bitmap);
  }

  //creating root directory and adding to inode
  disk_inodes[0].is_used = 1;
  disk_inodes[0].direct_offset[0] = disk_super_block -> inode_metadata_offset + 1;
  disk_inodes[0].file_size = sizeof(disk_directory);
  disk_inodes[0].reference_count = 1;

  //now writing data
  char buffer[BLOCK_SIZE];
  
  memset(buffer, 0, sizeof(buffer));
  memcpy(buffer, disk_super_block, sizeof(struct super_block));
  if(block_write(0, buffer)){
    perror("Error writing superblock");
    return -1;
  }
  
  memset(buffer, 0, sizeof(buffer));
  memcpy(buffer, disk_block_bitmap, (DISK_BLOCKS / 8));
  if(block_write(1, buffer)){
    perror("Error writing block_bitmap");
    return -1;
  }
  
  memset(buffer, 0, sizeof(buffer));
  memcpy(buffer, disk_inodes, INODE_NUM * sizeof(struct inode));
  if(block_write(2, buffer)){
    perror("Error writing inode_bitmap");
    return -1;
  }

  memset(buffer, 0, sizeof(buffer));
  memcpy(buffer, disk_directory, FILE_NUM * sizeof(struct dir_entry));
  if(block_write(3, buffer)){
    perror("Error writing directory");
    return -1;
  }
  
  if(close_disk()){
    perror("Could not close disk");
    return -1;
  }

  //free everything
  free(disk_super_block);
  free(disk_directory);
  free(disk_block_bitmap);
  free(disk_inodes);

  return 0;
}

int mount_fs(const char *disk_name){
  if(open_disk(disk_name)){
    perror("Error opening disk");
    return -1;
  }
  char buffer[BLOCK_SIZE];
  memset(buffer, 0, sizeof(buffer));
  if(block_read(0, buffer)){
    perror("Error reading super_block");
    return -1;
  }
  memcpy(&mounted_super_block, buffer, sizeof(struct super_block));
  memset(buffer, 0, sizeof(buffer));
  if(block_read(mounted_super_block.used_block_bitmap_offset, buffer)){
    perror("Error reading block_bitmap");
    return -1;
  }
  memcpy(&mounted_block_bitmap, buffer, sizeof(mounted_block_bitmap));
  memset(buffer, 0, sizeof(buffer));
  if(block_read(mounted_super_block.inode_metadata_offset, buffer)){
    perror("Error reading inode_bitmap");
    return -1;
  }
  memcpy(&mounted_inodes, buffer, sizeof(mounted_inodes));
  memset(buffer, 0, sizeof(buffer));
  if(block_read(mounted_inodes[0].direct_offset[0], buffer)){
    perror("Error reading directory");
    return -1;
  }
  memcpy(&mounted_directory, buffer, sizeof(mounted_directory));
  is_mounted = 1;
  return 0;
}

int umount_fs(const char *disk_name){
  char buffer[BLOCK_SIZE];
  //setting superblock as 0
  memset(buffer, 0, sizeof(buffer));
  memcpy(buffer, &mounted_super_block, sizeof(mounted_super_block));
  if(block_write(0, buffer)){
    perror("Error writing superblock");
    return -1;
  }
  //setting block_bitmap @ 1
  memset(buffer, 0, sizeof(buffer));
  memcpy(buffer, mounted_block_bitmap, sizeof(mounted_block_bitmap));
  if(block_write(1, buffer)){
    perror("Error writing block_bitmap");
    return -1;
  }
  //setting node_bitmap @ 2
  memset(buffer, 0, sizeof(buffer));
  memcpy(buffer, mounted_inodes, sizeof(mounted_inodes));
  if(block_write(2, buffer)){
    perror("Error writing inode_bitmap");
    return -1;
  }
  //setting disk_directory @ 3
  memset(buffer, 0, sizeof(buffer));
  memcpy(buffer, mounted_directory, sizeof(mounted_directory));
  if(block_write(3, buffer)){
    perror("Error writing directory");
    return -1;
  }
  //clearing out global variables
  memset(&mounted_super_block, 0, sizeof(mounted_super_block));
  memset(&mounted_block_bitmap, 0, sizeof(mounted_block_bitmap));
  memset(&mounted_inodes, 0, sizeof(mounted_inodes));
  memset(&mounted_directory, 0, sizeof(mounted_directory));
  memset(&mounted_files, 0, sizeof(mounted_files));
  
  if(close_disk()){
    perror("Could not close disk");
    return -1;
  }

  is_mounted = 0;
  return 0;
}

int fs_open(const char *name){
  int fd = -1;
  for (int i = 0; i < FD_NUM; i++){
    if (!mounted_files[i].is_used){
      fd = i;
      break;
    }
  }
  if(fd == -1){
    perror("Error: open file descriptor not found\n");
    return -1;
  }
  int working_directory = -1;
  for(int i = 0; i < FILE_NUM; i++){
    if(strcmp(name, mounted_directory[i].name) == 0){
      working_directory = i;
      break;
    }
  }
  if(working_directory == -1){
    perror("Error: filename not found\n");
    return -1;
  }
  mounted_inodes[mounted_directory[working_directory].inode_number].reference_count++;
  mounted_files[fd].inode_number = mounted_directory[working_directory].inode_number;
  mounted_files[fd].is_used = 1;
  mounted_files[fd].offset = 0;
  
  return fd;
}

int fs_close(int fd){
  if((fd < 0) || (fd >= FD_NUM)){
    perror("Error: file descriptor does not exist\n");
    return -1;
  }
  if(mounted_files[fd].is_used == 0){
    perror("Error: file descriptor is not in use\n");
    return -1;
  }
  mounted_files[fd].is_used = 0;
  int working_inode = mounted_files[fd].inode_number;
  mounted_inodes[working_inode].reference_count--;
  mounted_files[fd].inode_number = -1;
  mounted_files[fd].offset = 0;
  
  return 0;
}

int fs_create(const char *name){
  if(!is_mounted){
    perror("Disk is not mounted");
    return -1;
  }
  if(strlen(name) > 15){
    perror("name is too long");
    return -1;
  }
  int index = -1;
  for(int i = 0; i < FILE_NUM; i++){
    if(strcmp(mounted_directory[i].name, name) == 0){
      perror("file with this name already exists");
      return -1;
    }
    if((!mounted_directory[i].is_used) && (index == -1)){
      index = i;
    }
  }
  if(index == -1){
    perror("directory is full");
    return -1;
  }
  int inode_index = -1;
  for(int i = 0; i < INODE_NUM; i++){
    if(!mounted_inodes[i].is_used){
      inode_index = i;
      break;
    }
  }
  if(inode_index == -1){
    perror("no free inodes");
    return - 1;
  }

  mounted_inodes[inode_index].is_used = 1;
  mounted_inodes[inode_index].file_size = 0;
  memset(mounted_inodes[inode_index].direct_offset, 0, sizeof(mounted_inodes[inode_index].direct_offset));
  mounted_inodes[inode_index].single_indirect_offset = 0;
  mounted_inodes[inode_index].double_indirect_offset = 0;
  mounted_inodes[inode_index].reference_count = 0;

  strcpy(mounted_directory[index].name, name);
  mounted_directory[index].is_used = 1;
  mounted_directory[index].inode_number = inode_index;
  
  return 0;
}

int fs_delete(const char *name){
  //error checking
  int directory_index = -1;
  for(int i = 0; i < FILE_NUM; i++){
    if(strcmp(mounted_directory[i].name, name) == 0){
      directory_index = i;
      break;
    }
  }
  if(directory_index == -1){
    perror("could not find file with that name");
    return -1;
  }
  int inode_index = mounted_directory[directory_index].inode_number;
  for(int i = 0; i < FD_NUM; i++){
    if((mounted_files[i].inode_number == inode_index) && (mounted_files[i].is_used)){
      perror("A file descriptor references this file");
      return -1;
    }
  }

  //freeing metadata
  int block_size = mounted_inodes[inode_index].file_size/BLOCK_SIZE;
  uint16_t first_indirect_buf[BLOCK_SIZE / sizeof(uint16_t)];
  memset(&first_indirect_buf, 0, sizeof(first_indirect_buf));
  uint16_t second_indirect_buf[BLOCK_SIZE / sizeof(uint16_t)];
  memset(&second_indirect_buf, 0, sizeof(second_indirect_buf));
  if((mounted_inodes[inode_index].file_size % BLOCK_SIZE) > 0){
    block_size += 1;
  }
  if(block_size > 10){
    if(block_read(mounted_inodes[inode_index].single_indirect_offset, first_indirect_buf)){
      perror("could not read block");
      return -1;
    }
  }
  if(block_size > 266){
    if(block_read(mounted_inodes[inode_index].double_indirect_offset, second_indirect_buf)){
      perror("could not read block");
      return -1;
    }
  }

  for(int i = 0; i < block_size; i++){
    if(i < 10){
      clear_block(mounted_inodes[inode_index].direct_offset[i], &mounted_block_bitmap);
      mounted_inodes[inode_index].direct_offset[i] = -1;
    }else if(i < 266){
      clear_block(first_indirect_buf[i - 10], &mounted_block_bitmap);
      //      first_indirect_buf[i - 10] = -1;
    }else if(i < 522){
      clear_block(second_indirect_buf[i - 266], &mounted_block_bitmap);
      //  second_indirect_buf[i - 266] = -1;
    }
  }

  memset(&mounted_inodes[inode_index], 0, sizeof(mounted_inodes[inode_index]));

  mounted_directory[directory_index].is_used = 0;
  strcpy(mounted_directory[directory_index].name, "");
  mounted_directory[directory_index].inode_number = -1;

  return 0;
}



int fs_read(int fildes, void *buf, size_t nbyte){
  //error checking
  if((fildes < 0) || (fildes >= FD_NUM)){
    perror("Error: file descriptor does not exist\n");
    return -1;
  }
  if(!mounted_files[fildes].is_used){
    perror("Error: file descriptor not in use");
    return -1;
  }

  unsigned long int offset = mounted_files[fildes].offset;
  int inode_index = mounted_files[fildes].inode_number;
  if(mounted_inodes[inode_index].file_size <= offset){
    return 0;
  }
  
  size_t bytes_remaining = nbyte;
  if(bytes_remaining > mounted_inodes[inode_index].file_size){
    bytes_remaining = mounted_inodes[inode_index].file_size;
  }
  int start_block = offset / BLOCK_SIZE;
  int offset_within = offset % BLOCK_SIZE;
  int bytes_read = 0;
  int i = start_block;

  //starting block arithmetic
  int required_blocks = (offset + nbyte)/BLOCK_SIZE;
  if((offset + nbyte) % BLOCK_SIZE > 0){
    required_blocks += 1;
  }
  int file_block_size = mounted_inodes[inode_index].file_size/BLOCK_SIZE;
  if((mounted_inodes[inode_index].file_size % BLOCK_SIZE) > 0){
    file_block_size += 1;
  }
  uint16_t single_indirect_pointers[BLOCK_SIZE / sizeof(uint16_t)];
  uint16_t double_indirect_pointers[BLOCK_SIZE / sizeof(uint16_t)];
  if(file_block_size > 10){
    block_read(mounted_inodes[inode_index].single_indirect_offset, single_indirect_pointers);
  }
  if(file_block_size > 266){
    block_read(mounted_inodes[inode_index].double_indirect_offset, double_indirect_pointers);
  }
  int current_block = -1;

  while(bytes_remaining > 0){
    int to_read = 0;
    if(BLOCK_SIZE - offset_within < bytes_remaining){
      to_read = BLOCK_SIZE - offset_within;
    }else{
      to_read = bytes_remaining;
    }

    if(i < 10){
      current_block = mounted_inodes[inode_index].direct_offset[i];
    }else if(i < 266){
      current_block = single_indirect_pointers[i - 10];
    }else{
      current_block = double_indirect_pointers[i - 266];
    }

    block_read(current_block, working_buffer);
    memcpy((char*)buf + bytes_read, working_buffer + offset_within, to_read);
    
    i++;
    offset_within = 0;
    bytes_read += to_read;
    bytes_remaining -= to_read;
  }

  return bytes_read;
}

int fs_write(int fildes, void *buf, size_t nbyte){
  //error checking
  if((fildes < 0) || (fildes >= FD_NUM)){
    perror("Error: file descriptor does not exist\n");
    return -1;
  }
  if(!mounted_files[fildes].is_used){
    perror("Error: file descriptor not in use");
    return -1;
  }

  unsigned long int offset = mounted_files[fildes].offset;
  int inode_index = mounted_files[fildes].inode_number;
  
  size_t bytes_remaining = nbyte;
  int start_block = offset / BLOCK_SIZE;
  int offset_within = offset % BLOCK_SIZE;
  int bytes_written = 0;
  int i = start_block;

  //starting block arithmetic
  int required_blocks = (offset + nbyte)/BLOCK_SIZE;
  if((offset + nbyte) % BLOCK_SIZE > 0){
    required_blocks += 1;
  }
  int file_block_size = mounted_inodes[inode_index].file_size/BLOCK_SIZE;
  if((mounted_inodes[inode_index].file_size % BLOCK_SIZE) > 0){
    file_block_size += 1;
  }
  uint16_t single_indirect_pointers[BLOCK_SIZE / sizeof(uint16_t)];
  uint16_t double_indirect_pointers[BLOCK_SIZE / sizeof(uint16_t)];
  if(file_block_size > 10){
    block_read(mounted_inodes[inode_index].single_indirect_offset, single_indirect_pointers);
  }
  if(file_block_size > 266){
    block_read(mounted_inodes[inode_index].double_indirect_offset, double_indirect_pointers);
  }

  //finding free memory
  if(required_blocks  > file_block_size){
    for(int j  = 0; j < (required_blocks - file_block_size); j++){
      int first_free_bit = -1;
      int is_bit_found = 0;
      for(int k = 0; k < (DISK_BLOCKS / 8); k++){
	if(mounted_block_bitmap[k] == 0377){
	  first_free_bit += 8;
	  continue;
	}
	uint8_t bit_comparison = ~mounted_block_bitmap[k] & (mounted_block_bitmap[k] + 1);
	while(bit_comparison){
	  if (bit_comparison & 1){
	    is_bit_found = 1;
	    first_free_bit++;
	    break;
	  }
	  bit_comparison >>= 1;
	  first_free_bit++;
	}
	if(is_bit_found){
	  is_bit_found = 0;
	  break;
	}
      }
      if(first_free_bit == -1){
	perror("No free memory");
	return -1;
      }
      first_free_bit++;
      set_block(first_free_bit, &mounted_block_bitmap);
      if((j + file_block_size) < 10){
	mounted_inodes[inode_index].direct_offset[j + file_block_size] = first_free_bit;
      }else if((j + file_block_size) < 266){
	single_indirect_pointers[j + file_block_size - 10] = first_free_bit;
      }else{
	double_indirect_pointers[j + file_block_size - 266] = first_free_bit;
      }
    }
    block_write(mounted_inodes[inode_index].single_indirect_offset, single_indirect_pointers);
    block_write(mounted_inodes[inode_index].double_indirect_offset, double_indirect_pointers);
  }

  int current_block = -1;
  
  while(bytes_remaining > 0){
    int to_write;
    if(BLOCK_SIZE - offset_within < bytes_remaining){
      to_write = BLOCK_SIZE - offset_within;
    }else{
      to_write = bytes_remaining;
    }

    if(i < 10){
      current_block = mounted_inodes[inode_index].direct_offset[i];
    }else if(i < 266){
      current_block = single_indirect_pointers[i - 10];
    }else{
      current_block = double_indirect_pointers[i - 266];
    }
    
    block_read(current_block, working_buffer);
    memcpy(working_buffer + offset_within, (char*)buf + bytes_written, to_write);
    block_write(current_block, working_buffer);

    mounted_inodes[inode_index].file_size += to_write;
    i++;
    offset_within = 0;
    bytes_written += to_write;
    bytes_remaining -= to_write;
  }
  
  return bytes_written;
}

int fs_get_filesize(int fildes){
  if((fildes < 0) || (fildes >= FD_NUM)){
    perror("Error: file descriptor does not exist\n");
    return -1;
  }
  if(!mounted_files[fildes].is_used){
    perror("Error: file descriptor not in use");
    return -1;
  }

  return mounted_inodes[mounted_files[fildes].inode_number].file_size;
}

int fs_listfiles(char ***files){
  if(files == NULL){
    perror("files is NULL");
    return -1;
  }
  int files_size = 0;
  for(int i = 0; i < FILE_NUM; i++){
    if(mounted_directory[i].is_used){
      files_size += 1;
    }
  }

  *files = calloc(files_size + 1, sizeof(char*));
  if (*files == NULL) {
    return -1;
  }
  
  int j = 0;
  for(int i = 0; i < FILE_NUM; i++){
    if(mounted_directory[i].is_used){
      (*files)[j] = malloc(strlen(mounted_directory[i].name) + 1);
      strcpy((*files)[j], mounted_directory[i].name);
      j++;
    }
  }
  (*files)[j] = NULL;
  return 0;
}

int fs_lseek(int fildes, off_t offset){
  if((fildes < 0) || (fildes >= FD_NUM)){
    perror("Error: file descriptor does not exist\n");
    return -1;
  }
  if(!mounted_files[fildes].is_used){
    perror("Error: file descriptor not in use");
    return -1;
  }
  int inode_index = mounted_files[fildes].inode_number;
  if(offset > mounted_inodes[inode_index].file_size){
    perror("offset is larger than filesize");
    return -1;
  }
  mounted_files[fildes].offset = offset;
  return 0;
}

int fs_truncate(int fildes, off_t length){
  if((fildes < 0) || (fildes >= FD_NUM)){
    perror("Error: file descriptor does not exist\n");
    return -1;
  }
  if(!mounted_files[fildes].is_used){
    perror("Error: file descriptor not in use");
    return -1;
  }
  if(length < 0){
    perror("invalid length");
    return -1 ;
  }
  if(length > mounted_inodes[mounted_files[fildes].inode_number].file_size){
    perror("cannot make file larger");
    return -1;
  }				       
  
  int inode_index = mounted_files[fildes].inode_number;
  int blocks_to_keep = length/BLOCK_SIZE;
  if((length % BLOCK_SIZE) > 0){
    blocks_to_keep++;
  }
  int total_blocks = mounted_inodes[inode_index].file_size/BLOCK_SIZE;
  if((mounted_inodes[inode_index].file_size % BLOCK_SIZE) > 0){
    total_blocks++;
  }
  uint16_t single_indirect_pointers[BLOCK_SIZE / sizeof(uint16_t)];
  uint16_t double_indirect_pointers[BLOCK_SIZE / sizeof(uint16_t)];
  if(total_blocks > 10){
    block_read(mounted_inodes[inode_index].single_indirect_offset, single_indirect_pointers);
  }
  if(total_blocks > 266){
    block_read(mounted_inodes[inode_index].double_indirect_offset, double_indirect_pointers);
  }
  
  for(int i = blocks_to_keep; i < total_blocks; i++){
    if(i < 10){
      clear_block(mounted_inodes[inode_index].direct_offset[i], &mounted_block_bitmap);
      mounted_inodes[inode_index].direct_offset[i] = 0;
    }else if(i < 266){
      clear_block(single_indirect_pointers[i - 10], &mounted_block_bitmap);
      single_indirect_pointers[i - 10] = 0;
    }else{
      clear_block(double_indirect_pointers[i - 266], &mounted_block_bitmap);
      double_indirect_pointers[i - 266] = 0;
    }
  }
  block_write(mounted_inodes[inode_index].single_indirect_offset, single_indirect_pointers);
  block_write(mounted_inodes[inode_index].double_indirect_offset, double_indirect_pointers);

  mounted_inodes[inode_index].file_size = length;
  if(mounted_files[fildes].offset > length){
    mounted_files[fildes].offset = length;
  };
  if (mounted_files[fildes].offset > length){
    mounted_files[fildes].offset = length;
  }
  return 0;
}
