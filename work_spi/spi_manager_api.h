#ifndef __SPI_MANAGER_API__H
#define __SPI_MANAGER_API__H



#ifdef __cplusplus
extern "C" {
#endif





int open_key(void);
int  set_poweroff_status(void);
int read_voltage(void);
int operation_lm1(int op);
int operation_lm2(int op);
int operation_lm3(int op);
int operation_backlight(int op);
int open_key(void);
int  test_poweroff_status(void);
int spi_modules_init(const spidev_t* devname);
void poweroff_now(void);


#ifdef __cplusplus
}
#endif

#endif


