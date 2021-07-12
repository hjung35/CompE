#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"
#include "paging.h"
#include "i8259.h"
#include "interrupts.h"
#include "filesys.h"
#include "console.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].st.offset_15_00 == NULL) && 
			(idt[i].st.offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}


/**
 * @brief Test to see if paging was initialized correctly
 * 
 * @return int PASS/FAIL
 */
int page_init_test() {
	TEST_HEADER;

	/* Initialize lots of vars to zero */
	int non_vmem_counter = 0;
	int vmem_check = 0;
	int kernel_check = 0;
	int vmem_pde_check = 0;
	int page_dir_aligned = 0;
	int vmem_table_aligned = 0;
	int result = PASS;
	int i;
	unsigned int addr;

	/* Check that all pages in the first 4MB are uniitialized, except for the one containing video memory */
	for (i = 0; i < TBL_SIZE; i++) {
		addr = (vmem_table.pte[i] & 0xF0F); //0xF0F because we set those bits, but access and dirty bits can change so lets ignore those
		if (addr == 0x6 || addr == 0x00) { //0x6 is unititialized and 00 is first 4kb with r/w turned off 
			non_vmem_counter++;
		} 
		else if (addr == 0x107) {
			vmem_check++;
		}
	}

	/* Check page directory and page table are 4KB aligned */
	if (((unsigned int) &page_dir % (4 * DIR_SIZE)) == 0) {
		page_dir_aligned++;
	}

	if (((unsigned int) &vmem_table % (4 * TBL_SIZE)) == 0) {
		vmem_table_aligned++;
	}

	/* Check that the first entry in the page directory corresponds to the start of the page table */
	int compAddr = ((unsigned int) &(vmem_table.pte[0]) & ADDR_MASK);
	int compAddr2 = (page_dir.pde[0] & ADDR_MASK);
	if (compAddr == compAddr2) {
		vmem_pde_check++;
	}

	/* Check that the second entry in the page directory is the 4MB page for the kernel */
	int compAddr3 = (page_dir.pde[1] & ADDR_MASK);
	if (compAddr3 == KENREL_LOC) {
		kernel_check++;
	}

	/* If any of the tests above do not match expected values than we fail the test */
	if (non_vmem_counter != 1023 ||
		vmem_check != 1 || 
		vmem_pde_check != 1 || 
		kernel_check != 1 || 
		page_dir_aligned != 1 || 
		vmem_table_aligned != 1) 
	{
		assertion_failure();
		result = FAIL;
	}
	return result;
}

/**
 * @brief Test whether the RTC and KBD interrupts were enabled properly.
 * This test will not fail if any other interrupts have been enabled.
 * @return int PASS/FAIL
 */
int pic_kbdrtc_test()
{
	// Get the current interrupt mask from the PIC
	uint8_t pic_mask = inb(MASTER_8259_DATAPORT);
	// RTC is IRQ 0, KBD is IRQ 1, so check to make sure those bits are 0.
	if (pic_mask & 0x03) {
		// One of them is still masked: FAIL.
		return FAIL;
	} else {
		return PASS;
	}
}

/**
 * @brief Test if the PIC mask is the same as what we think it should be.
 * 
 * @return int PASS/FAIL
 */
int pic_mask_test()
{
	uint8_t pic_mask = inb(MASTER_8259_DATAPORT);
	if (get_master_mask() != pic_mask) return FAIL;
	pic_mask = inb(SLAVE_8259_DATAPORT);
	if (get_slave_mask() != pic_mask) return FAIL;
	return PASS;
}

/**
 * @brief Read back the IDTR and make sure it points to the IDT.
 * 
 * @return int PASS/FAIL
 */
int idtr_test()
{
	x86_desc_t read_idtr;
	sidt(read_idtr);
	if (read_idtr.addr != idt_desc_ptr.addr) return FAIL;
	if (read_idtr.size != idt_desc_ptr.size) return FAIL;
	return PASS;
}

/**
 * @brief Intentionally segfault to test that segmentation is working.
 * Don't call this in TEST_OUTPUT.
 * @return int Unused, never returns.
 */
/* NOTE: For now this should just page fault */
int deref_null_test()
{
	printf("[TEST segfault_test]: ");
	int * null_ptr = 0;
	// This should segfault.
	return *null_ptr;
}

/**
 * @brief Intentionally divide by zero to test that exception is working.
 * Don't call this in TEST_OUTPUT.
 * @return int result, shoud never return, returns FAIL if it somehow does.
 */
// We comment this out to avoid the compiler yelling at us (causing us to lose points) for doing division by zero. To test simply uncomment this and call in launch test
/*
int div0_test()
{
	printf("[TEST divide_by_zero]: ");
	int bad_math = 3/0; // this should raise div by zero exception
	return bad_math;
}
*/

/**
 * @brief Intentionally attempt to mov into a non accessible cr to test that invalid instruction exceptions are working.
 * Don't call this in TEST_OUTPUT.
 * @return int FAIL, shoud never return, returns FAIL if it somehow does.
 */
int invalid_opcode_test(){
	TEST_HEADER;

		/* Wait. That's illegal */
	    asm volatile (
        "mov %%eax, %%cr6                     ;"
        : 
        : 
        : "eax"                                    
    );

	return FAIL;
}

/**
 * @brief Intentionally access uninitialized page to test that page faults are working.
 * Don't call this in TEST_OUTPUT.
 * @return int result, shoud never return, returns FAIL if it somehow does.
 */
int page_fault_test(){
	TEST_HEADER;

	int *bad_addr;
	int bad_val;
	int result = PASS;

	bad_addr = (int*) 0x1000; // This address is in an uninitialized page so we should page fault
	bad_val = *bad_addr;

	/* Uh oh we didn't page fault? */
	result = FAIL;
	return result;
}

/**
 * @brief Access initialized (jumbo) page to test that page is correctly initialized
 * 
 * @return int result, shoud always return PASS, throws exception if it fails
 */
int kernel_pg_access_test(){
	TEST_HEADER;

	int* kernel_mem_start;
	int mem_in_kernel;
	int result = PASS;

	kernel_mem_start = (int*) KENREL_LOC; // 4MB is the start of kernel memory
	mem_in_kernel = *kernel_mem_start; // if our paging is correct we should not get a fault here,

	// We don't test for failure because failure throws the page fault! So we wouldn't get here
	return result;
}

/**
 * @brief Access initialized 4KB page to test that page is correctly initialized
 * 
 * @return int result, shoud always return PASS, throws exception if it fails
 */
int vmem_pg_access_test(){
	TEST_HEADER;

	int* vmem_start;
	int vmem_val;
	int result = PASS;

	vmem_start = (int*) VIDEO_LOC;
	vmem_val = *(vmem_start + 10);  // If our paging is correct this should access some place in video memory, and get no fault

	//  We don't test for failure because failure throws the page fault! So we wouldn't get here
	return result;
}

/**
 * @brief Purposely fail a test to see that assertion_failure is handled correctly
 * 
 * @return none, we see if this is successful by seeing if ASSERTION FAILED is printed (by the exception handler)
 */
void failure_test(){
	TEST_HEADER;

	assertion_failure();
}

/* Checkpoint 2 tests */
/**
 * @brief Test that we are getting RTC interrupts
 * 
 * @return int PASS or FAIL
 */
int rtc_test(){
	TEST_HEADER;
	const char spinny[4] = {'\\', '|', '/', '-'};

	uint16_t freq = 2;
	rtc_open(NULL); //open the null pointer, butt rtc_open just sets the frequency of 2
	while(freq <= 1024){
		int i;
		printf("%d: |", freq);
		for(i = 0; i < freq; i++){
			rtc_read(0, NULL, 0); 
			// printf("%d ", freq);
			console_bksp();
			console_putchar(spinny[(i % 4)]);
		}
		freq *= 2;

		rtc_write(0, (void*) &freq, 4);
		printf("\n");
	}
	rtc_close(NULL); // rtc close testitng. 
	return PASS;
}

/**
 * @brief Ensure the RTC write rejects invalid settings
 * 
 * @return int PASS or FAIL
 */
int rtc_garbage_test()
{
	int garbage = 0x23;
	if (rtc_write(0, &garbage, 4) == -1) return PASS; else return FAIL;
}

/**
 * @brief Open the files that we see in fsdir
 * 
 * @return pASS if they all succesfully open, otherwise FAIL
 */
int file_open_test(){
	TEST_HEADER;

	int result = PASS;
	int i;
	/* Table of the filenames that we want to open */
	const char* files[14] = {"cat", "counter", "fish", "frame0",
	"frame1", "grep", "hello", "ls", "pingpong", "shell", "sigtest", 
	"syserr", "testprint","verylargetextwithverylongname"};

	/* Loop through opening all files, if one doesn't open then FAIL */
	for(i = 0; i < 14; i++)
	if(file_open((uint8_t*)files[i]) == -1){
		result = FAIL;
		assertion_failure();
		break;
	}
	return result;
}

/**
 * @brief Open the directory "."
 * 
 * @return pASS if succesfully opened, otherwise FAIL
 */
int dir_open_test(){
	TEST_HEADER;

	int result = PASS;
	const char* dir = ".";

	if(dir_open((uint8_t*)dir) == -1){
		result = FAIL;
		assertion_failure();
	}
	return result;
}

/**
 * @brief Attempt writing to both a directory and a file, both should fail since this is read only
 * 
 * @return pASS if both writes fail (return -1), otherwise FAIL
 */
int write_test(){
	TEST_HEADER;

	int result = PASS;

	if(file_write(0, NULL, 0) != -1 || dir_write(14, NULL, 0) != -1){
		result = FAIL;
		assertion_failure();
	}
	return result;
}

/**
 * @brief Read the entirety of whatever file is passed in.
 * Note that the 3rd arg in file can be changed to read however many bytes you desire to test.
 * 
 * Input: c -- the file descriptor value for whatever file you want to read
 * Note that the file open test must be called for this to work, refer to the table provided in that
 * test to open certain files (ie. fd = 0 -> files[0] -> cat, etc.)
 * 
 * @return none, we check this passes by comparing the output the the actual file in the case of a txt file, or by checking that
 * the printed values to the terminal match those printed by xxd in devel
 */
void file_read_test(int c){
	console_clrsc();
	TEST_HEADER;
	uint8_t buf[10000];
	int i;
	int bytes_read;
	/* Make the buffer empty just for safety */
	for(i = 0; i < 10000; i++){
		buf[i] = 0;
	}

	bytes_read = file_read(c, buf, 10000);
	for(i = 0; i < bytes_read; i++){
		console_putchar((char)buf[i]);
	}	
	printf("\n");
}

/**
 * @brief Show that consecutive reads to the same file start from the place left off
 * 
 * Note that we only test with one file, becausue this would do the same thing for all files. Change the first
 * arg in the file reads to see for yourself. Refer to the above function header for details on what file that arg opens.
 * 
 * @return none, we check this passes by comparing the output the the actual file in the case of a txt file, or by checking that
 * the printed values to the terminal match those printed by xxd in devel
 */
void file_consec_read_test(){
	clear();
	TEST_HEADER;
	int i;
	int bytes_read;
	uint8_t buf[200];
	for(i = 0; i < 200; i++){
		buf[i] = 0;
	}
	bytes_read = file_read(3, buf, 30);
	for(i = 0; i < bytes_read; i++){
		console_putchar((char)buf[i]);
	}	
	printf("\nNext read\n");
	bytes_read = file_read(3, buf, 40);
	for(i = 0; i < bytes_read; i++){
		console_putchar((char)buf[i]);
	}	
	printf("\nNext read\n");
	bytes_read = file_read(3, buf, 200);
	for(i = 0; i < bytes_read; i++){
		console_putchar((char)buf[i]);
	}
	printf("\n");
}
/**
 * @brief Print the name of the directory entry for the given fd (for this test the one for the directory)
 * In this test we print all files in the filesystem image, because censecutive calls to dir read increment the
 * directory entry name being read.
 * Note that this test proves functionality of the read_dentry_by_index function making the test 4 in the piazza hint pointless
 * because we already proved above that file reads. That test covers the same things that these two tests do, while also proving other
 * functionalities. So we just use these.
 * 
 * 
 * @return none, we check this passes by seeing if it prints out all the directory entries in the filesystem image
 */
void dir_read_test(){
	console_clrsc();
	TEST_HEADER;
	uint8_t buf[32];
	int i;
	int bytes_read;
	for(i = 0; i < 32; i++){
		buf[i] = 0;
	}
	for(i = 0; i < 25 ; i++){
		dir_read(14, buf, 32);
		bytes_read = console_puts((char*)buf,32);
		if(bytes_read != 0)
			console_puts("\n", 1);
	}

}

/**
 * @brief Close all the files, if any closes fail then the test fails (none should fail
 * because for this checkpoint there is no fail condition)
 * 
 * 
 * @return PASS if the closes succeed, otherwise FAIL
 */
int file_close_test(){
	TEST_HEADER;

	int result = PASS;
	int i;

	for(i = 0; i < 14; i++)
	if(file_close(i) == -1){
		result = FAIL;
		assertion_failure();
		break;
	}
	return result;
}

/**
 * @brief Close the directory, (the way we implemented the filedescriptor for this checkpoint this must be closed like files)
 * 
 * 
 * @return PASS if the close succeed, otherwise FAIL
 */
int dir_close_test(){
	TEST_HEADER;

	int result = PASS;

	if(dir_close(15) == -1){
		result = FAIL;
		assertion_failure();
	}
	return result;
}

/**
 * @brief Check to make sure the console has been marked as initialized.
 * 
 * @return int PASS or FAIL
 */
int console_init_test()
{
	if (console_isinit()) return PASS; else return FAIL;
}

/**
 * @brief Check to make sure characters are being printed to the console properly.
 * 
 * @return int PASS or FAIL
 */
int console_print_test()
{
	console_puts("\n1234567890", 11);
	int curx, cury;
	console_getcursorpos(&curx, &cury);
	return (curx == 10)? PASS : FAIL;
}

/**
 * @brief Check to make sure we're getting input from the console.
 * 
 * @return int PASS or FAIL
 */
int console_input_test()
{
	char in_str[128] = {'\0'};
	printf("Please type the following at the prompt followed by a RETURN: TESTING 123!\n>");
	console_readline(in_str, 127);
	printf("Typed input: %s", in_str);
	if (strncmp("TESTING 123!\n", in_str, 14)) return FAIL; else return PASS;
}

/**
 * @brief Check to see if the buffered input driver handles overflows correctly.
 * 
 * @return int PASS or FAIL
 */
int console_ovflw_test()
{
	char in_str[19] = {'\0'};
	in_str[18] = 0x1B; // Canary
	printf("Please type at least 16 characters at the prompt.  Do not press RETURN.\n>");
	console_readline(in_str, 16);
	printf("\nTyped input: %s", in_str);
	if (in_str[18] == 0x1B) return PASS; else return FAIL;
}



/* Checkpoint 3 tests */
/*
int exec_parse_test(){
	char* string = "Test 123";
	char fname[32];
	char argstr[100];

	//parse_exec(string, fname, argstr);

	if(!strncmp("Test", fname, sizeof("Test")) || !strncmp("123", argstr, sizeof("123"))){
		return PASS;
	}

	return FAIL;
}
*/
int exec_pages_test(){
	execute("Test");
	if((process_pdir_table[0].pde[32] & 0x1) == 1){
		return FAIL;
	}

	return PASS;
}

int exec_elf_check_test(){
	int res = execute("ls");
	if(res == -1){
		return PASS;
	}

	return FAIL;
}

/**
 * @brief Check to see if invalid system calls are rejected.
 * 
 * @return int PASS or FAIL
 */
int syscall_invalid_test()
{
	int retval;
	// Use assembly linkage to call an invalid syscall
	asm volatile (
		"movl $-2, %%eax\n\t"
		"int $0x80\n\t"
		"movl %%eax, %0"
		: "=r" (retval)
		: /* No inputs */
		: "eax"
	);
	return (retval == -1)? PASS : FAIL;
}

/**
 * @brief Try to call a system call that has no handler
 * 
 * @return int PASS or FAIL
 */
int syscall_null_test()
{
	int retval;
	syscall_add(14, NULL);
	// Use assembly linkage to call a NULL syscall
	asm volatile (
		"movl $14, %%eax\n\t"
		"int $0x80\n\t"
		"movl %%eax, %0"
		: "=r" (retval)
		: /* No inputs */
		: "eax"
	);
	return (retval == -1)? PASS : FAIL;
}

/**
 * @brief Call read and write system calls to verify they exist
 * 
 * @return int PASS or FAIL
 */
int syscall_functions_test()
{
	int retval1 = 0, retval2 = 0;
	// Use assembly linkage to call READ and WRITE for FD's 0 and 1.
	// FIXME: These will fail right now because of null pointers, but getting back -1 means they were called.
	asm volatile (
		"movl $3, %%eax\n\t" // System call 3
		"movl $0, %%ebx\n\t" // fd=0: stdin
		"movl $0, %%ecx\n\t" // buf = NULL so this should fail.
		"movl $0, %%edx\n\t"
		"int $0x80\n\t"
		"movl %%eax, %0"
		: "=r" (retval1)
		: /* No inputs */
		: "eax", "ebx", "ecx", "edx"
	);
	// This is in case GCC doesn't touch eax in the middle
	asm volatile(
		"movl $0, %%eax"
		::
		: "eax"
	);
	asm volatile (
		"movl $4, %%eax\n\t" // System call 4
		"movl $1, %%ebx\n\t" // fd=1: stdout
		"movl $0, %%ecx\n\t" // buf=NULL so this should fail too.
		"movl $0, %%edx\n\t"
		"int $0x80\n\t"
		"movl %%eax, %0"
		: "=r" (retval2)
		: /* No inputs */
		: "eax", "ebx", "ecx", "edx"
	);
	if (retval1 != -1 || retval2 != -1) return FAIL; else return PASS;
}
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	char garbage;
	// console_clrsc(); /* Let's clear the screen so then we can see the result of the tests clearly */
	printf("[TEST BAT 1: MEMORY]\n");
	TEST_OUTPUT("idt_test", idt_test());
	TEST_OUTPUT("idtr_test", idtr_test());
	TEST_OUTPUT("pic_mask_test", pic_mask_test());
	TEST_OUTPUT("page_init_test", page_init_test());
	TEST_OUTPUT("kernel_pg_access_test", kernel_pg_access_test());
	TEST_OUTPUT("vmem_pg_access_test",vmem_pg_access_test());
	printf("Press RETURN to continue...");
	console_getchar(&garbage);
	console_clrsc();
	printf("[TEST BAT 2: CONSOLE]\n");
	TEST_OUTPUT("console_init_test", console_init_test());
	TEST_OUTPUT("console_print_test", console_print_test());
	TEST_OUTPUT("console_input_test", console_input_test());
	TEST_OUTPUT("console_ovflw_test", console_ovflw_test());
	printf("Press RETURN to continue...");
	console_getchar(&garbage);
	console_clrsc();

	//failure_test(); // See function header for explanation on no TEST_OUTPUT and why we can return

	/* The tests below are not through TEST_OUTPUT, because they should never return (they are exceptions) 
	This means that only one should be enabled at a time (only the first one enabled will be called anyway) */

	//div0_test(); // DO NOT uncomment unless you uncomment the function as well
	//deref_null_test();
	//page_fault_test();
	//invalid_opcode_test();

	// /* Filesys tests : Disable these ones, they use the old system */
	// //clear(); /* Just look at these tests for now */
	// printf("[TEST BAT 3: FILESYSTEM]\n");
	// TEST_OUTPUT("file_open_test", file_open_test());
	// printf("Press RETURN to continue...");
	// console_getchar(&garbage);
	// console_clrsc();
	// TEST_OUTPUT("dir_open_test", dir_open_test());
	// printf("Press RETURN to continue...");
	// console_getchar(&garbage);
	// console_clrsc();
	// TEST_OUTPUT("write_test", write_test());
	// printf("Press RETURN to continue...");
	// console_getchar(&garbage);
	// console_clrsc();
	// /* Only input integers from 0<=c<=13 or else this will not work. Each number controls the file, use the indices
	// in the array of filenames in file_open_test to figure out which file you want to open, see test function header
	// for more details */
	// /* To make it visible what each test does just leave one of these uncommented at a time, or all if you only wanna look at other tests */
	// file_read_test(13);
	// printf("Press RETURN to continue...");
	// console_getchar(&garbage);
	// console_clrsc();
	// //file_consec_read_test();
	// //dir_read_test();

	// /* Make sure these are the last file tests because the files need to be open for us to run other tests on them */
	// TEST_OUTPUT("file_close_test", file_close_test());
	// TEST_OUTPUT("dir_close_test", dir_close_test());

	printf("[TEST BAT 4: RTC]\n");
	// TEST_OUTPUT("rtc_test", rtc_test());
	TEST_OUTPUT("rtc_garbage_test", rtc_garbage_test());
	TEST_OUTPUT("test_exec_stuff",exec_elf_check_test());

	printf("[TEST BAT 5: SYSTEM CALLS]\n");
	TEST_OUTPUT("syscall_invalid_test", syscall_invalid_test());
	TEST_OUTPUT("syscall_null_test", syscall_null_test());
	TEST_OUTPUT("syscall_functions_test", syscall_functions_test());
	printf("[TESTS COMPLETE]\n");
}
