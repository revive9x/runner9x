#include <stdio.h>
#include <stdarg.h>

void rlog(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	
	printf("[r9x-dbg] ");
	vprintf(fmt, args);
	printf("\n");

	va_end(args);
}
