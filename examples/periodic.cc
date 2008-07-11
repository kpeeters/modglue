
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
	{
	int counter=1;
	char buffer[100];

	while(true) {
		if(argc>1) {
			if(write(1, argv[1], strlen(argv[1]))<strlen(argv[1]))
				exit(1);
			}
		else { 
			sprintf(buffer, "test %d", counter);
			if(write(1, buffer, strlen(buffer))<strlen(buffer))
				exit(1);
			}
		++counter;
		write(1, "\n", 1);
		sleep(1);
		}
	return 0;
	}
