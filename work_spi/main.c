#include <stdio.h>
main(int argc,char *argv[])
{
	int op;
	op = atoi(argv[3]);
	printf("op is %d\n",op);

	spi_modules_init(argv[1]);
	if(!strcmp(argv[2],"-r"))
		{
		}
		else if(!strcmp(argv[2],"-g"))
		{
			printf("the voltage is %d\n",read_voltage());
		}
		else if(!strcmp(argv[2],"-p"))
		{

		}
		else if(!strcmp(argv[2],"-c"))
		{

			poweroff_now();
		}
		else if(!strcmp(argv[2],"-l1"))
		{
			operation_lm1(op);
		}
		else if(!strcmp(argv[2],"-o"))
		{
			out_12(op);
		}
		else if(!strcmp(argv[2],"-l2"))
		{
			operation_lm2(op);
		}
		else if(!strcmp(argv[2],"-l3"))
		{
			operation_lm2(op);
		}
		else if(!strcmp(argv[2],"-b"))
		{
			printf("operation_bark\n");
			operation_backlight(op);
		}else if(!strcmp(argv[2],"-k"))
		{
			open_key();
		}else if(!strcmp(argv[2],"-re"))
		{
			restart_system();
		}

}

