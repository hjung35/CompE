#include "types.h"
#include "filesys.h"
#include "paging.h"
#include "lib.h"
#include "scheduling.h"
#include "syscall.h"

//dentry_t dir_entry_arr[MAX_DENTRY_NUM];      /* Temp array for cp2 containg all possible directory entries, we use this as our pseudo file descriptor */

pcb_t * cur_pcb;

/* Helper functions */
int read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int read_dentry_by_index(uint32_t idx, dentry_t* dentry);
int read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
int get_num_dir_entries();
int get_num_inodes();
int get_num_data_blocks();
uint32_t get_data_block(int idxOffset, inode_t* inodeptr);
uint32_t get_inode_len(int inodeidx);

/*
 * init_dir
 *   DESCRIPTION:   Set all entries in dir_entry_array to have init_flag off (our way of say all fd's are available).
 *                  Note that this must be called (in kenel.c) before we can do any file operations 
 *   INPUTS: addr -- address of the filesystem loaded in kernel.c
 *   OUTPUTS: set filesys_addr to the input addr because that is the address we will use for our calculations later on in this file
 *   RETURN VALUE: none
 *   SIDE EFFECTS: set filesys_addr, initialize
 */ 
void init_dir(uint32_t* addr){
    filesys_addr = addr;
}


/*
 * file_open
 *   DESCRIPTION:   Given a file name we allocate a file descriptor and set up its for that file. See
 *                  read_dentry_by_name for more details on that.
 *   INPUTS: fname -- string name of the file we want to open
 *   OUTPUTS: none
 *   RETURN VALUE: if successful we return the file descriptor value (an index into the file array), if not return -1 for failure
 *   SIDE EFFECTS: initialize a file descriptor
 */ 
int file_open(const uint8_t* fname) {
    dentry_t cur_dentry;
    int fdnum;
    filedesc_t * fdptr = get_free_fd(&fdnum);
    if (read_dentry_by_name((uint8_t *)fname, &cur_dentry) == -1) return -1;
    if (!fdptr) return -1;
    fdptr->flags.in_use = true;
    fdptr->optbl = &regfile_optbl;
    fdptr->inode = cur_dentry.inode_num;
    fdptr->pos = 0;
    return fdnum;
}

/*
 * dir_open
 *   DESCRIPTION:   Given a directory name we allocate a file descriptor and set it up for that directory. See
 *                  read_dentry_by_name for more details on that.
 *   INPUTS: dirname -- string name of the directory we want to open
 *   OUTPUTS: none
 *   RETURN VALUE: if successful we return the file descriptor value (an index into the file array), if not return -1 for failure
 *   SIDE EFFECTS: initialize a file descriptor
 */ 
int dir_open(const uint8_t* dirname) {
    dentry_t cur_dentry;
    int fdnum;
    filedesc_t * fdptr = get_free_fd(&fdnum);
    if (read_dentry_by_name((uint8_t *)dirname, &cur_dentry) == -1) return -1;
    if (!fdptr) return -1;
    fdptr->flags.in_use = true;
    fdptr->optbl = &dir_optbl;
    return fdnum;
}

/*
 * file_close
 *   DESCRIPTION:   Undo file_open. Set the file descriptor to be flagged as free so we can use it for a different file
 *   INPUTS: fd -- the file descriptor we are closing
 *   OUTPUTS: none
 *   RETURN VALUE: if successful we return 0, if not return -1 for failure (happens when fd 0 or 1 are attempted to be closed, in checkpoint 3 not 2)
 *   SIDE EFFECTS: closes a file descriptor
 */ 
int file_close(int fd){
    filedesc_t * fdptr = syscall_getfdptr(fd);
    /* Do not allow closing of default file descriptors or file descriptors out of range */
    if(fd == 0 || fd == 1 || fd < 0 || fd > 7)
        return -1;
    /* Future check if file descriptor is allocated? */
    /* Clear PCB entry, ret success */
    fdptr->flags.in_use = 0; // This is also done in close() but whatevs.

    return 0;
}

/*
 * dir_close
 *   DESCRIPTION:   Undo dir_open. Set the file descriptor to be flagged as free so we can use it for a different file
 *   INPUTS: fd -- the file descriptor we are closing
 *   OUTPUTS: none
 *   RETURN VALUE: if successful we return 0, if not return -1 for failure (happens when fd 0 or 1 are attempted to be closed, in checkpoint 3 not 2)
 *   SIDE EFFECTS: closes a file descriptor
 */ 
int dir_close(int fd){
    /* Do not allow closing of default file descriptors or file descriptors out of range */
    if(fd == 0 || fd == 1 || fd < 0 || fd > 7)
        return -1;
    // dir_entry_arr[fd].init_flag = 0; There's no longer anything to do here.
    return 0;
}

/*
 * file_write and dir_write
 *   DESCRIPTION:   Unused in read only file system, the same for both functions.
 *   INPUTS: fd -- file descriptor for current file or directory
 *   OUTPUTS: none
 *   RETURN VALUE: -1, failure because we cannot write in a read only file system
 *   SIDE EFFECTS: none
 */ 
int file_write(int fd, const void * buf, int nbytes){
    return -1;
}
/* See above */
int dir_write(int fd, const void * buf, int nbytes){
    return -1;
}

/*
 * file_read
 *   DESCRIPTION: Given a file reads the files contents until end of file is reached or until the desired number of bytes
 *                  is read.
 *   INPUTS: fd -- file descriptor for current file or directory
 *          buf --  buffer to be filled with file contents
 *          nbytes -- number of bytes to be read
 *   OUTPUTS: buf now filled with file contents
 *   RETURN VALUE: bytes_read -- the number of bytes read from this call (0 if failed to read or already at end of file)
 *   SIDE EFFECTS: fills input buffer, updates position in file for given fd.
 */ 
int file_read(int fd, void* buf, int32_t nbytes){
    int bytes_read;     /* Number of bytes read from this call */
    filedesc_t * fdptr = syscall_getfdptr(fd);

    /* Read the file */
    bytes_read = read_data(fdptr->inode, fdptr->pos, buf, nbytes);

    /* Update position in file and return number of bytes read from read_data */
    fdptr->pos += bytes_read;
    return bytes_read;
}

/*
 * dir_read
 *   DESCRIPTION: Given a directory reads the directory entry name, successive calls read next directory entry name until no more exist
 *   INPUTS: fd -- file descriptor for current file or directory
 *          buf --  buffer to be filled with file contents
 *          nbytes -- number of bytes to be read
 *   OUTPUTS: buf now filled with file/directory name
 *   RETURN VALUE: bytes_read -- the number of bytes read from this call (0 if failed to read or already at end of file)
 *   SIDE EFFECTS: fills input buffer, updates position in directory entry to be read next for given fd.
 */ 
int dir_read(int fd, void* buf, int32_t nbytes){
    int bytes_read = 0; /* Number of bytes read (chars in name) */
    dentry_t garbage;
    

    /* If the dentry index we are trying to access is greater than the number of dentries we read 0 bytes */
    if(get_num_dir_entries() < cur_pcb->file_array[fd].pos)
        return 0;
    
    /* If the dentry isn't real we read 0 bytes */
    if(read_dentry_by_index(cur_pcb->file_array[fd].pos, &garbage) != 0)
        return 0;

    /* Copy the name over to the buffer and then update the index */
    strncpy((int8_t*)buf, (int8_t*)garbage.fname, 32);
    cur_pcb->file_array[fd].pos++;
    char* test;
    test = (char*)buf;
    /* If the name is not null terminated, lets make it so so the last char is not messed up for max size names, we make it the index after the max size so we don't
    delete any chars in the name */
    if(test[nbytes] != '\0') // This is ok to have nbytes instead of nbytes-1 because ls passes in a buffer of size 33
        test[nbytes] = '\0';
    bytes_read = strlen((int8_t*)buf);

    return bytes_read;
}

/*
 * read_dentry_by_name
 *   DESCRIPTION: Searches through the directory entries in the filesys img looking for one with the
 *                  same name as fname. Updates dentry info if found.
 *   INPUTS: fname -- string name of the file we want to find
 *          dentry --  pointer to directory entry struct that we will update the contents of
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if we successfully find the directory entry corresponding to fname
 *                  -1 if we cannot find it
 *   SIDE EFFECTS: fills dentry with the data corresponding to the directory entry w/ fname
 */ 
int read_dentry_by_name(const uint8_t* fname, dentry_t* dentry){
    uint32_t dentry_tot;    /* Number of total directory entries */
    int i;                  /* Loop Counter */
    dentry_t** dir_table;   /* Pointer to the mem location of the first directory entry */
    dentry_t* temp;         /* Pointer to dentry struct obtained by offsetting from dir table */

    /* If the length of the string is greater than the max file name size we know it doesn't exist */
    if(strlen((char*)fname) > MAX_FNAME_SIZE){
        return -1;
    }
    
    /* The first thing in the boot block is the number of directory entries */
    dentry_tot = *filesys_addr;
    /* The directory entries begin 64 bytes into the boot block (explaing the magic 64) */
    dir_table = (dentry_t**)((uint32_t)filesys_addr + 64);
    temp = (dentry_t*)((uint32_t)dir_table - DENTRY_SIZE);
    /* Iterate through the directory entries looking for a matching name */
    for(i = 0; i < dentry_tot; i++){
        /* We can find the desired entry by using the input index * 64 (because each dentr is 64 B) as an offset to find them mem location
        of the desired dentry */
        temp = (dentry_t*)((uint32_t)temp + DENTRY_SIZE);
        if(!strncmp(temp->fname, (int8_t*)fname, 32)){//strlen((int8_t*)fname))
            /* Fill dentry and return success */
            strcpy((int8_t*)dentry->fname, (int8_t*)fname);
            dentry->ftype = temp->ftype;
            dentry->inode_num = temp->inode_num;
            return 0;
        }
    }
    /* If we never returned from the loop then the name did not exist */
    return -1;
}

/*
 * read_dentry_by_index
 *   DESCRIPTION: Goes to directory entry with given index. Updates dentry info if found.
 *   INPUTS: idx -- index into the list of directory entries in the filesystem
 *          dentry --  pointer to directory entry struct that we will update the contents of
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if the index was valid and we update the dentry
 *                  -1 if we cannot find it
 *   SIDE EFFECTS: fills dentry with the data corresponding to the directory entry given by index
 */ 
int read_dentry_by_index(uint32_t idx, dentry_t* dentry){
    uint32_t dentry_tot;    /* Number of total directory entries */
    dentry_t** dir_table;   /* Pointer to the mem location of the first directory entry */
    dentry_t* temp;         /* Pointer to dentry struct obtained by offsetting from dir table */

    /* The first thing in the boot block is the number of directory entries */
    dentry_tot = *filesys_addr;
    
    /* Check if valid idx */
    if(idx > dentry_tot)
        return -1;

    /* The directory entries begin 64 bytes into the boot block (explaing the magic 64) */
    dir_table = (dentry_t**)((uint32_t)filesys_addr + 64);
    /* We can find the desired entry by using the input index * 64 (because each dentry is 64 B) as an offset to find them mem location
    of the desired dentry */
    temp = ((dentry_t*)((uint32_t)dir_table + idx * 64));

    /* Fill dentry and return success */
    strncpy(dentry->fname, temp->fname, 32);
    dentry->ftype = temp->ftype;
    dentry->inode_num = temp->inode_num;
    return 0;
}

/*
 * read_data
 *   DESCRIPTION: Given an inode number and the location in the file we want to read, we read the data from the file
 *   INPUTS: inode-- inode number for the current file we want to read
 *          offset --  number of bytes into the file we want to start reading
 *          buf -- the buffer we will be putting the read data into
 *          length -- the number of bytes we want to read
 *   OUTPUTS: fills input buffer 
 *   RETURN VALUE: bytes_read -- number of bytes in the file read this call
 *   SIDE EFFECTS: fills input buffer
 */ 
int read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    int i;                                          /* Loop counter */
    int byte_offset = offset % BLOCK_SIZE;          /* Number of bytes offset into a data block */
    int index_offset = offset / BLOCK_SIZE;         /* Number of data blocks we offset based on number of bytes offset */
    int32_t dataBlockCount = get_num_data_blocks();     /* Number of data blocks total */
    int bytes_read = 0;                             /* Number of bytes read in this call */
    uint32_t bytes_max;                             /* Number of bytes in the file we are reading */
    uint8_t start_flag = 0;                         /* Flag checking if this is the beginning of the file, so we don't offset immediately */

    /* Check if this is a bad file, if it is we read 0 B */
    if(inode > get_num_inodes())
        return 0;
    /* Make inodeptr for our inode struct to easily access the data in the inode in the filesystem */
    inode_t * inodeptr = (inode_t*)((uint32_t)filesys_addr + (inode * BLOCK_SIZE) + BLOCK_SIZE);
    bytes_max = inodeptr->length;

    
    /* Loop through the file starting from offset until length is reached or we reach the end of the file */
    for(i = 0; i < length; i++){
        /* Check if end of file reached */
        if(bytes_read + offset > bytes_max)
            return bytes_read;
        /* Check if datablock is valid */
        if(inodeptr->data_blocks[index_offset] > dataBlockCount)
            return bytes_read;

        /* This is how we check which data block we go into, we mod by blocksize. Reset the byte offset and increment the datablock offset into the file */
        if ((get_data_block(index_offset, inodeptr) + byte_offset) % BLOCK_SIZE == 0 && start_flag != 0) {
            byte_offset = 0;
            index_offset++;
        }
        /* Fill the buffer with new info from the data blocks, then increment that we've read a byte */
        buf[i] = *(uint8_t*)(get_data_block(index_offset, inodeptr) + byte_offset);
        byte_offset++;
        bytes_read++;
        start_flag = 1;
    }
    return bytes_read;
}

/*
 * file_to_mem
 *   DESCRIPTION: Helper function for system call, 'EXECUTE'
 *   INPUTS: addr -- virtual address of any user program; 128MB + offsets 
 *          fname --  file name we are trying to read
 *   OUTPUTS: none
 *   RETURN VALUE: return -1 for failure, otherwise return entry point
 *   SIDE EFFECTS: none
 */ 
int file_to_mem(uint8_t* addr, char* fname){
    dentry_t temp_dentry;           /* Used to grab the dentry data easily */
    uint8_t temp_buf[MAX_TEMP_BUF];          /* Used to grab the meta data in the format expected by read_data */
    uint32_t file_size;             /* Size in bytes of the file we are trying to execute */
    uint32_t entry;                 /* Address of entry point into new process */
    int process_num;                /* Number of current process (zero indexed) */
    pcb_t * parent_pcb = NULL;             /* Pointer ot PCB of parent process, or what cur_pcb was last time. */

    /* Check that the file exists, if it is fill in the temp_dentry, if not return failure */
    if(read_dentry_by_name((uint8_t*)fname, &temp_dentry) == -1){
        return -1;
    }
    /* Get length of file */
    file_size = get_inode_len(temp_dentry.inode_num);

    /* Read meta data from the file, if 40 bytes is not read return failure */
    if(read_data(temp_dentry.inode_num, 0, temp_buf, MAX_TEMP_BUF) != MAX_TEMP_BUF){
        return -1;
    }

    /* Check if executable, if not return failure */
    if(temp_buf[0] != EXECUTABLE_FILE_ONE || temp_buf[1] != EXECUTABLE_FILE_TWO || temp_buf[2] != EXECUTABLE_FILE_THREE || temp_buf[3] != EXECUTABLE_FILE_FOUR){
        return -1;
    }

    /* Set up paging for new process, and get process number, return failure if paging could not be made (may be at max number of processes) */
    process_num = new_process_ptable();
    if(process_num == -1){
        return -1;
    }

    /* do PCB stuff, starting at 8MB -8KB - 8KB*process num */
    /* Fill in PCB data, first by setting the address, then setting the parent id number and current process id number */
    parent_pcb = cur_pcb;
    cur_pcb = (pcb_t *)(MB_8 - KB_8 - (KB_8 * process_num));
    cur_pcb->parent_pid = process_num - 1;
    cur_pcb->pid = process_num;
    /* Determine the vconsole for this to be.  If this is the first shell, always 0.
     * If an override is set, obey it.  Otherwise, start on the parent's console. */
    cli(); /* con_ovr flag must be manipulated atomically w/ respect to interrupts. */
    if (process_num == 0) {
        cur_pcb->con = 0;
    } else if (con_ovr.flag) {
        cur_pcb->con = con_ovr.idx;
        con_ovr.flag = false;
    } else if (parent_pcb) {
        cur_pcb->con = parent_pcb->con;
    } else {
        cur_pcb->con = 0; /* Default to the first terminal if bad things are afoot. */
    }
    sti();
    
    /* Copy the executable into the new user page, if we don't copy the whole file return failure */
    if(read_data(temp_dentry.inode_num, 0, addr, file_size) != file_size){
        return -1;
    }

    /* Calculate the address of the entry point into the new process, then return that */
    /* 27,26,25 = byte offset of the file , shifting bits = 27, 26, 25 corresponds to the MSB we intends */
    entry = (temp_buf[27] << 24) + (temp_buf[26] << 16) + (temp_buf[25] << 8) + (temp_buf[24]);
    return entry;
}

/* Here to make code more readable
Description: gets and returns the number of directory entries
in the file system by accessing the first 4B in the boot block */
int32_t get_num_dir_entries(){
    return *(filesys_addr);
}

/* Here to make code more readable
Description: gets and returns the number of inodes
in the file system by accessing the second 4B in the boot block.
Should always be 64 */
int32_t get_num_inodes(){
    return *(uint32_t*)((uint32_t)filesys_addr + 4);
}

/* Here to make code more readable
Description: gets and returns the number of data blocks
in the file system by accessing the third 4B in the boot block. */
int32_t get_num_data_blocks(){
    return *(uint32_t*)((uint32_t)filesys_addr + 8);
}

/*
 * get_data_block
 *   DESCRIPTION: Find the address in memory of the datablock for the given inode and the offset into the 
 * data blocks for that inode.
 *   INPUTS: inodeptr -- pointer to inode struct containg the list of datablocks for that inode
 *          idxOffset --  Offest into the list of datablocks for the given inode
 *   OUTPUTS: none 
 *   RETURN VALUE: retAddr -- the address of the datablock corresponding to the given inode and index 
 *   SIDE EFFECTS: none
 */ 
uint32_t get_data_block(int idxOffset, inode_t* inodeptr){
    uint32_t numinode = get_num_inodes();                       /* Number of inodes */
    uint32_t dbIndex = inodeptr->data_blocks[idxOffset];        /* The datablock number in the filesystem */
    uint32_t retAddr = (uint32_t)((uint32_t)filesys_addr + BLOCK_SIZE + (numinode * BLOCK_SIZE) + (dbIndex * BLOCK_SIZE));
    return retAddr;
}

/*
 * get_data_block
 *   DESCRIPTION: Find the length in bytes of the current inode.
 *   INPUTS: inodeidx -- inode number for the inode we are analyzing
 *   OUTPUTS: none 
 *   RETURN VALUE: the length in bytes of the current inode given by inodeidx 
 *   SIDE EFFECTS: none
 */ 
uint32_t get_inode_len(int inodeidx){
    inode_t * inodeptr = (inode_t*)((uint32_t)filesys_addr + (inodeidx * BLOCK_SIZE) + BLOCK_SIZE);
    return inodeptr->length;
}
/**
 * @brief Get a pointer to a file descriptor for the current task
 * 
 * @param fd File descriptor to get a pointer to.
 * @return filedesc_t* NULL on error, pointer to FD object otherwise. 
 */
filedesc_t * syscall_getfdptr(uint32_t fd)
{
    return &(cur_pcb->file_array[fd]);
}

/**
 * @brief Get a pointer to a free file descriptor for the current process.
 * @param fdnum Pointer to an int variable where the number of the FD can be placed.
 * fdnum may be left null. 
 * @return filedesc_t* Pointer to free FD, NULL on error or not available.
 */
filedesc_t * get_free_fd(int * fdnum)
{
    int fd;
    for (fd=2; fd<NUM_FDS; fd++) {
        if (!(syscall_getfdptr(fd)->flags.in_use)) goto found_fd;
    }
    return NULL;
    found_fd:
    if (fd) *fdnum = fd;
    return syscall_getfdptr(fd);
}

/* Parse input string
Will support multiple words, but will ignore
after the first for this checkpoint (no get args) */
void parse_exec(char* string, char* filename, char* argstr){
    int i = 0;      /* Index/Loop Counter */
    int j;          /* Holds old i for indexing purposes */

    /* Grab the file name (like ls)*/
    while(string[i] != 0x20 && string[i] != '\0' && i < MAX_FNAME_SIZE){ // 0x20 -> magic number for space
        filename[i] = string[i];
        i++;
    }
    filename[i] = '\0'; // Null terminate the string :)

    /* Strip leading spaces */
    while(string[i] == 0x20){ // ASCII ' '
        i++;
    }
    /* Now copy to argstring */
    j = 0;
    while(string[i] != '\0' && j < 128 ) {//+ MAX_FNAME_SIZE){ //128 is the size of the argstr buffer, see variable declaration for details
        argstr[j] = string[i];
        /* This if statement allows traling spaces to be ignored (ie. 'frame0.txt ' but have normal fucntionality for things like 'grep very large') */
        if(string[i] != 0x20 || string[i+1] != '\0'){ 
            j++;
        }
        i++;
    }

    /* Null terminate that string */
    argstr[j] = '\0';
}

/*
 * execute 
 *   DESCRIPTION: attemps to load and execute a new program, handing off the processor to the new program until it terminates
 *   INPUTS: strname -- command that is space-separated sequence of words
 *   OUTPUTS: none
 *   RETURN VALUE: return -1 when fails, 0 for success
 *   SIDE EFFECTS: executes a program. in any failure cases, it calls 'halt' function. 
 */  
int execute(char* strname){
    uint32_t temp_esp;              /* Temp variable to hold esp when execute starts */
    uint32_t temp_ebp;              /* Temp variable to hold ebp when execute starts */
    char fname[MAX_FNAME_SIZE];     /* Name of the file we are trying to execute */
    char argstr[128];               /* The arguments that go along with that file, 128 because that is max kbdr buffer size */
    uint32_t entry_addr;            /* The address of the entry point into the new process */
    int32_t parent;                /* pid of the parent to give to the child */
    task_t temp_cur;                /* Task struct for temporarily holding the task in task_queue[0] */
    task_t temp_next;               /* Task struct for temporarily holding the task that will be created from execute */

    /* Save esp */
    asm volatile (
        "movl %%esp, %0      ;"
        :"=r" (temp_esp)
        :
    );
    /* Save ebp */
    asm volatile (
        "movl %%ebp, %0      ;"
        :"=r" (temp_ebp)
        :
        : "eax"
    );    

    /* Parse the input string to get the file name and the arguments seperated */
    parse_exec(strname, fname, argstr);

    // For giving to the child, the first 3 shells won't have parents
    if(cur_pcb == NULL || task_queue[2].pcb == NULL){
        parent = -1;
    }
    else{
        parent = cur_pcb->pid;
    }

    // Save temp current task
    temp_cur = task_queue[0];

    /* Checks if file is executeable, if it is makes new paging data for it, updates pcb, copies file to memory */
    cli();
    entry_addr = file_to_mem((uint8_t*)LOAD_LOC, fname);
    /* If file_to_mem failed execute fails (-1) */
    if(entry_addr == -1){
        return -1;
    }

    /* Set stdin and stdout in file array for pcb */
    /* stdin */
    cur_pcb->file_array[0].optbl = &stdio_optbl;
    cur_pcb->file_array[0].flags.in_use = 1;
    /* stdout */
    cur_pcb->file_array[1].optbl = &stdio_optbl;
    cur_pcb->file_array[1].flags.in_use = 1;

    /* Now that the pcb is made we can set esp and ebp and args*/
    cur_pcb->esp = temp_esp;
    cur_pcb->ebp = temp_ebp;
    strcpy(cur_pcb->args, argstr);
    /* Make sure the vidmap check is initially off! */
    cur_pcb->vidmap_check = false;
    // Set to the correct parent for cp5
    cur_pcb->parent_pid = parent;
    /* Set the return value for the pcb to be return in execute */
    /* Give control to child */
    cur_pcb->ret_addr = entry_addr;

    // If the parent pid is -1 don't sleep parent, otherwise sleep parent
    if(cur_pcb == NULL || cur_pcb->parent_pid != -1){ // irrelevant for NULL, just don't want page faults
        temp_cur.enabled = false;
    }

    // Set temp next task, safe from interrupts???
    // Will we need to protect everywhere with cur_pcb??? Probably
    //cli();
    temp_next.pcb = cur_pcb;
    temp_next.state = 1;
    temp_next.enabled = true;
    // temp_next.esp/ebp = esp/ebp
    /*
    asm volatile (
        "movl %%ebp, %%eax      ;"
        "movl %%esp, %%ebx      ;"
        : "=a" (temp_next.ebp), "=b" (temp_next.esp)
        :
        : "memory"//"eax", "ebx"
    );
    */
    // "Push" the newly executed process, the current one
    //task_push(&temp_next);
    if(cur_pcb->pid != 0){
        task_push(&temp_cur);
    }
    task_queue[0] = temp_next; // We have the new exec take priority over other tasks, shouldn't make a difference
    //sti();
    // I'm pretty sure everything from the file to mem call to tss.esp0 being set need to be in a critical section, but for now not doing it

    /* Set tss esp0 to new process kernel stack */
    tss.esp0 = ((uint32_t)cur_pcb) + KB_8 - STACK_OFF;
    sti();

    /* Push required data for iret/context switch to stack
    Order:
    1. (ds:esp) as 4 words
        - Push ds
        - Push esp (user stack at 132 MB)
    2. EFLAGS
    3. (cs:eip)
        - Push cs
        - Push eip (entry point into user program)
    */
    /* 0x7FFFBE is used to "pop" all the local vars of the stack so when ret is called eip will have the correct addr */
    asm (
        ".globl exec            ;"
        "exec:                  ;"
    );

    asm volatile (
        "pushl %%ebx            ;"
        "pushl %%edx            ;"
        "pushf                  ;"
        "pushl %%eax            ;"
        "pushl %%ecx            ;"
        "iret                   ;"
        ".globl exec_ret        ;"
        "exec_ret:              ;"
        "movl $0x7FFFBE, %%esp  ;"
        "ret                    ;"
        : 
        : "c" (cur_pcb->ret_addr), "a" (USER_CS), "b" (USER_DS), "d" (USER_STACK)
        : "memory"             
    );

    /* How'd you get here? */
    return -1;
}

/*
 * halt 
 *   DESCRIPTION: terminates a process, returning the specified value to its parent process.
 *   INPUTS: strname none
 *   OUTPUTS: none
 *   RETURN VALUE: return -1 when fails, 0 for success
 *   SIDE EFFECTS: terminates a process that is belonged to a program. this can be called separately, but also within call of execute. 
 */  
int halt(){
    int i;      /* Loop Counter */
    task_t temp;

    //cli(); //?

    /* If this is the first shell halting is not allowed! */
    if(cur_pcb->parent_pid == -1 || cur_pcb->pid == 0){
        console_clrsc();
        /* What we do is emulate the cleanup in the syscall handler (maybe not necessary but it works)
        then jump to the context switch in execute to essentially restart the shell */
        asm volatile (
            /*
            "addl $0x1C, %%esp  ;"
            "addl $16, %%esp    ;"
            "popl %%edi         ;"
            "popl %%esi         ;"
            "popl %%ebp         ;"
            "popl %%ebx         ;"
            "popl %%edx         ;"
            "popl %%ecx         ;"
            "popl %%esp         ;"
            "addl $8, %%esp     ;"
            "popf               ;"
            "addl $8, %%esp     ;"
            */
            "jmp exec           ;"
            "pushl %%eax        ;"
            "ret                ;"
            :
            : "a"(cur_pcb->ret_addr)
            : "memory"
        );
    }

    /* Clear all file descriptors */
    for(i = 0; i < MAX_OPEN_FILES; i++){
        cur_pcb->file_array[i].flags.in_use = 0;
        cur_pcb->file_array[i].inode = 0;
        cur_pcb->file_array[i].optbl = 0;
        cur_pcb->file_array[i].pos = 0; 
    }

    /* Clear the user video memory mapping */
    vmem_table2.pte[0] = 0;
    cur_pcb->vidmap_check = false;
    /* Return control back to parent! */

    /* Switch to parent's paging structure (and flush TLB) */
    return_parent_paging();

    /* Set to parent's PCB, these kB and MB values are for accessing dif locations using virtual addrs */
    cur_pcb = (pcb_t *)(MB_8 - KB_8 - (KB_8 * cur_pcb->parent_pid));

    /* Set tss esp0 to be parent's kernel stack */
    tss.esp0 = ((uint32_t)cur_pcb) + KB_8 - STACK_OFF;

    // Get the halted process out of the queue, then bring the parent to the front of the queue
    // More locking than this??
    cli();
    temp.pcb = cur_pcb; // This works because we only check pcb for queue matches
    task_grab(&temp);
    task_pop(&temp); // get parent out of the queue so we don't have duplicates
    task_queue[0] = temp; // Now make the parent the active task 
    task_queue[0].enabled = true; // make the parent runnable again

    sti(); // ??

    /* Go back to parent's stack setup then return to the end of execute */
    asm volatile (
        "movl %%ecx, %%esp      ;"
        "movl %%ebx, %%ebp      ;"
        "xorl %%eax, %%eax      ;"
        "jmp exec_ret           ;"
        :
        : "c" (cur_pcb->esp), "b" (cur_pcb->ebp)
        : "memory", "eax"
    );

    /* How'd you get here? */
    return -1;
}
