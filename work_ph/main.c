#include <stdio.h>
main(int argc,char *argv[])
{
	photoresponse_modules_init(argv[1]);
	while(1) {
		printf("%d",photoresponse_readval()); // light2 is back ling status
		sleep(2);
	}
}
