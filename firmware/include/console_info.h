#pragma once
#include <stddef.h>

/* Includes the complete metadata line and terminating NUL at maximum profile size. */
#define OSG_CONSOLE_INFO_CAPACITY 256U

/* Returns zero and clears output when the complete response cannot fit. */
size_t osg_console_info(char *output, size_t capacity, const char *profile,
                        const char *module, unsigned buttons, unsigned axes, unsigned hats);
