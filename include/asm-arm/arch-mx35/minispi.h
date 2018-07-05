#ifndef MINISPI_H
#define MINISPI_H
void cspi_init(void);
void cspi_shutdown(void);
int cspi_write(u32 d);
int cspi_read(u32 *d);
extern int cspi_logging;
#endif
