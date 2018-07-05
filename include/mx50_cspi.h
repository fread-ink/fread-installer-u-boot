#ifndef CSPI_H
#define CSPI_H

void ecspi_init(int port);
void ecspi_shutdown(void);
int ecspi_write(u32 d);
int ecspi_read(u32 *d);

#endif
