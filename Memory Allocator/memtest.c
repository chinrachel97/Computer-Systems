#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ackerman.h"
#include "my_allocator.h"

int main(int argc, char ** argv) {
    int basic_block_size = 128;
	int memory_length = 512;
	int c;

	// parse the command line arguments
	while((c = getopt(argc, argv, "b:s:")) != -1){
		switch(c){
			case 'b':{
				basic_block_size = atoi(optarg);
				break;
			}
			case 's':{
				memory_length = atoi(optarg);
				break;
			}
			default:{
				basic_block_size = 128;
				memory_length = 512;
			}
		}
	}
	if(basic_block_size > memory_length){
		printf("ERROR: Basic block size cannot be larger than total memory size.\n");
		return -1;
	}
	
	// initialize the memory manager
    int cont = init_allocator(basic_block_size, memory_length*1024);
	if(cont != 0)
		return -1;
	
	print_allocator();
	
	ackerman_main(); // this is the full-fledged test. 
    
	// The result of this function can be found from the ackerman wiki page or 
    //https://www.wolframalpha.com/. If you are not getting correct results, that means 
    //that your allocator is not working correctly. In addition, the results should be 
    //repeatable - running ackerman (3, 5) should always give you the same correct result. 
    //If it does not, there must be some memory leakage in the allocator that needs fixing
  
    // please make sure to run small test cases first before unleashing ackerman. One example
    //would be running the following: "print_allocator (); x = my_malloc (1); my_free(x); 
    //print_allocator();" the first and the last print should be identical.
    
	print_allocator();
    release_allocator();
}
