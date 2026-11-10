#ifndef __LOG_H__
#define __LOG_H__

#include "stdbool.h"

bool log_init();
void log_write(const char * sFormat, ...);


#endif /* __LOG_H__ */