#include "boot_session.h"
#include <string.h>
bool osg_boot_session_begin(const osg_journal_io_t *io,uint32_t *session) {
    if(!session) return false;
    uint8_t payload[4]={0}; size_t length=0; uint32_t previous=0;
    bool loaded=osg_journal_load(io,payload,sizeof payload,&length,&previous);
    if(loaded ? (length!=4 || memcmp(payload,"OSGD",4)) : !osg_journal_erased(io)) return false;
    if(previous==UINT32_MAX) return false;
    uint32_t next=previous+1; memcpy(payload,"OSGD",4);
    if(!osg_journal_commit(io,payload,sizeof payload,next)) return false;
    *session=next; return true;
}
