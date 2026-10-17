#include "console_info.h"
#include <stdio.h>

size_t osg_console_info(char *output, size_t capacity, const char *profile,
                        const char *module, unsigned buttons, unsigned axes, unsigned hats) {
    int length=snprintf(output,capacity,
        "model=device-template board=0.1.0 profile=%s module=%s protocol=3 buttons=%u axes=%u hats=%u\r\n",
        profile,module,buttons,axes,hats);
    if(length<0 || (size_t)length>=capacity) {
        if(capacity) output[0]='\0';
        return 0;
    }
    return (size_t)length;
}
