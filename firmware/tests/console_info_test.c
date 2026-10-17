#include "console_info.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const char module[]="4f5347441234567890abcdef12345678";
    char profile[64];
    memset(profile,'p',sizeof profile-1); profile[sizeof profile-1]='\0';
    profile[30]=' '; /* Profile labels are not restricted to single words. */
    char output[OSG_CONSOLE_INFO_CAPACITY];
    size_t length=osg_console_info(output,sizeof output,profile,module,128,8,4);
    assert(length>160 && length==strlen(output));
    char expected[256];
    int n=snprintf(expected,sizeof expected,
        "model=device-template board=0.1.0 profile=%s module=%s protocol=3 buttons=128 axes=8 hats=4\r\n",
        profile,module);
    assert(n>0 && (size_t)n==length && !strcmp(output,expected));
    assert(length<sizeof output && !strcmp(output+length-2,"\r\n"));

    /* The formatter used by firmware never emits a partial identity line. */
    char exact[OSG_CONSOLE_INFO_CAPACITY];
    assert(osg_console_info(exact,length+1,profile,module,128,8,4)==length);
    assert(!strcmp(exact,expected));
    memset(exact,'x',sizeof exact);
    assert(!osg_console_info(exact,length,profile,module,128,8,4) && !exact[0]);
    assert(!osg_console_info(exact,1,profile,module,128,8,4) && !exact[0]);
    exact[0]='x';
    assert(!osg_console_info(exact,0,profile,module,128,8,4) && exact[0]=='x');

    /* All decimal fields still fit even beyond the currently supported layout. */
    length=osg_console_info(output,sizeof output,profile,module,UINT_MAX,UINT_MAX,UINT_MAX);
    assert(length && length==strlen(output) && length<sizeof output);
    assert(!strcmp(output+length-2,"\r\n"));
    assert(osg_console_info(output,sizeof output,"output only",module,0,0,0));
    assert(strstr(output,"profile=output only module=") && strstr(output,"buttons=0 axes=0 hats=0\r\n"));
    return 0;
}
