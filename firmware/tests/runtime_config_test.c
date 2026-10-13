#include "runtime_config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { uint8_t pages[2][2048];unsigned operations,fail_at; } flash_t;
static const uint8_t *read_page(void *context,unsigned page) { return ((flash_t *)context)->pages[page]; }
static bool erase(void *context,unsigned page) {
    flash_t *f=context;bool fail=++f->operations==f->fail_at;
    memset(f->pages[page],255,fail?1024:2048);return !fail;
}
static bool program(void *context,unsigned page,unsigned offset,const uint8_t bytes[8]) {
    flash_t *f=context;bool fail=++f->operations==f->fail_at;
    for(unsigned i=0;i<(fail?4U:8U);i++) { assert(f->pages[page][offset+i]==255);f->pages[page][offset+i]&=bytes[i]; }
    return !fail;
}
static osg_journal_io_t io(flash_t *flash) { return (osg_journal_io_t){flash,read_page,erase,program}; }
static const osg_profile_button_t buttons[]={{0,false,0},{1,true,0}};
static const osg_profile_axis_t axes[]={{0,48,0,4095,32768}};
static const osg_profile_t defaults={.name="test",.buttons=buttons,.button_count=2,.axes=axes,.axis_count=1};
static void ok(osg_runtime_config_t *s,const char *line,uint32_t now) {
    bool apply=false;const char *error=osg_runtime_command(s,line,now,&apply);
    if(error) fprintf(stderr,"%s: %s\n",line,error);
    assert(!error);
    if(!strncmp(line,"apply ",6) || !strncmp(line,"apply_save ",11)) assert(apply);
}
static void error(osg_runtime_config_t *s,const char *line,const char *expected,uint32_t now) {
    bool apply=true;const char *actual=osg_runtime_command(s,line,now,&apply);
    assert(actual && !strcmp(actual,expected) && !apply);
}
static void transaction_test(void) {
    osg_runtime_config_t s;assert(osg_runtime_init(&s,&defaults,NULL));
    assert(s.profile.buttons==s.active.buttons && s.profile.name==s.active.name);
    uint8_t descriptor[256];memcpy(descriptor,s.descriptor,s.descriptor_length);size_t initial=s.descriptor_length;
    error(&s,"name 1 74657374","writer_required",0);ok(&s,"claim 1",1);
    error(&s,"claim 2","writer_busy",2);error(&s,"claim 0","invalid_request",3);
    error(&s,"counts 1 129 0 0 0 0 0","invalid_counts",3);
    error(&s,"button 1 0 4294967296 0 0","invalid_button",3);
    error(&s,"name 1 6100","invalid_name",3);error(&s,"name 1 eda080","invalid_name",3);
    ok(&s,"counts 1 2 1 1 1 1 1",4);
    ok(&s,"button 1 0 0 0 7",5);ok(&s,"button 1 1 24 0 7",6);
    ok(&s,"axis 1 0 7 56 100 4000 32768 1 1000",7);
    ok(&s,"hat 1 0 1 2 3 4",8);ok(&s,"encoder 1 0 5 6",9);ok(&s,"selector 1 0 25 26",10);
    ok(&s,"name 1 53c3a9f09f9880",11); /* Sé😀 */
    assert(!strcmp(s.active.name,"test") && s.generation==1);
    assert(s.descriptor_length==initial && !memcmp(descriptor,s.descriptor,initial));
    ok(&s,"apply 1",12);assert(s.pending_apply && s.generation==1);
    error(&s,"button 1 0 0 0 0","apply_pending",13);
    assert(osg_runtime_finish_apply(&s));assert(s.generation==2 && !s.pending_apply);
    assert(s.profile.buttons==s.active.buttons && s.layout.buttons==7 && s.layout.hats==1 && s.layout.axes==1);
    assert(s.layout.length==4 && (s.descriptor_length!=initial || memcmp(descriptor,s.descriptor,initial)));
    uint16_t text[64];assert(osg_runtime_utf16(s.active.name,text,64)==4);
    assert(text[1]==0xe9 && text[2]==0xd83d && text[3]==0xde00);
    ok(&s,"button 1 1 0 0 0",14);error(&s,"apply 1","invalid_profile",15);
    assert(s.active.buttons[1].input==24 && s.generation==2);
    ok(&s,"discard 1",16);assert(!s.dirty && s.draft.buttons[1].input==24);
    ok(&s,"reset 1",17);assert(!strcmp(s.draft.name,"test") && s.dirty);
    ok(&s,"release 1",18);assert(!s.owner && !s.dirty && s.draft.buttons[1].input==24);
    error(&s,"apply 1","writer_required",19);
    ok(&s,"claim 3",UINT32_MAX-100U);ok(&s,"name 3 6472616674",UINT32_MAX-50U);
    osg_runtime_tick(&s,29948);assert(s.owner==3);osg_runtime_tick(&s,29949);assert(!s.owner && !s.dirty);
    ok(&s,"claim 4",30000);ok(&s,"name 4 6472616674",30001);osg_runtime_invalidate_claim(&s);
    assert(!s.owner && !strcmp(s.draft.name,s.active.name));
    ok(&s,"claim 4",30002);error(&s,"save 4","persistence_unavailable",30003);
}
static void axis_test(void) {
    osg_runtime_config_t s;assert(osg_runtime_init(&s,&defaults,NULL));ok(&s,"claim 1",0);
    ok(&s,"axis 1 0 0 48 100 4000 32768 1 1000",1);ok(&s,"apply 1",2);assert(osg_runtime_finish_apply(&s));
    osg_profile_sample_t sample={.axes_fresh=1};osg_profile_state_t state;uint8_t report[64];
    assert(osg_profile_init(&state,&s.profile,&sample,0));
    const uint16_t raw[]={100,4000,2050};const uint16_t expected[]={65535,0,32768};
    for(unsigned i=0;i<3;i++) {
        sample.axes[0]=raw[i];osg_profile_update(&state,&sample,i+1,true,report);osg_runtime_transform_report(&s,&state,report);
        unsigned offset=state.layout.axis_offset;assert((report[offset]|((uint16_t)report[offset+1]<<8))==expected[i]);
    }
    sample.axes_fresh=0;osg_profile_update(&state,&sample,103,true,report);osg_runtime_transform_report(&s,&state,report);
    assert(!state.axis_available && (report[state.layout.axis_offset]|((uint16_t)report[state.layout.axis_offset+1]<<8))==32768);
}
static void maximum_test(void) {
    osg_runtime_config_t s;assert(osg_runtime_init(&s,&defaults,NULL));ok(&s,"claim 1",0);
    ok(&s,"counts 1 128 8 4 0 0 255",1);char line[128];
    for(unsigned i=0;i<128;i++) { snprintf(line,sizeof line,"button 1 %u %u 0 255",i,i);ok(&s,line,2); }
    for(unsigned i=0;i<8;i++) { snprintf(line,sizeof line,"axis 1 %u %u %u 0 4095 65535 1 32767",i,i,48+i);ok(&s,line,2); }
    for(unsigned i=0;i<4;i++) { snprintf(line,sizeof line,"hat 1 %u %u %u %u %u",i,128+4*i,129+4*i,130+4*i,131+4*i);ok(&s,line,2); }
    ok(&s,"apply 1",3);assert(osg_runtime_finish_apply(&s) && s.layout.length==34);
    char response[4096];size_t n=osg_runtime_snapshot(&s,false,response,sizeof response);assert(n>0 && n<sizeof response);
    assert(strstr(response,"\"reportBytes\":34") && strstr(response,"\"valid\":true"));
    assert(!osg_runtime_snapshot(&s,false,response,10) && !response[0]);
    uint8_t record[OSG_RUNTIME_RECORD_SIZE];assert(osg_runtime_encode(&s.active,s.generation,record,sizeof record));
    osg_runtime_definition_t decoded;uint32_t generation;
    assert(osg_runtime_decode(&decoded,&generation,record,sizeof record) && generation==2);
    assert(decoded.buttons[127].input==127 && decoded.hats[3].inputs[3]==143);
    record[78+1]=2;assert(!osg_runtime_decode(&decoded,&generation,record,sizeof record));
    assert(osg_runtime_encode(&s.active,s.generation,record,sizeof record));record[0]=0;
    assert(!osg_runtime_decode(&decoded,&generation,record,sizeof record));
    s.generation=UINT32_MAX;error(&s,"apply 1","generation_exhausted",4);
}
static void initial_baseline_test(void) {
    flash_t initial;memset(&initial,0,sizeof initial);memset(initial.pages,255,sizeof initial.pages);
    osg_journal_io_t backend=io(&initial);osg_runtime_config_t s,reboot;
    assert(osg_runtime_init(&s,&defaults,&backend));
    assert(s.generation==1 && s.saved_generation==1 && !s.owner && !s.dirty);
    unsigned count=initial.operations;assert(count==80); /* erase, 78 data doublewords, marker */
    initial.operations=0;assert(osg_runtime_init(&reboot,&defaults,&backend));assert(!initial.operations);
    for(unsigned cut=1;cut<=count;cut++) {
        flash_t interrupted;memset(&interrupted,0,sizeof interrupted);memset(interrupted.pages,255,sizeof interrupted.pages);
        interrupted.fail_at=cut;backend=io(&interrupted);
        assert(!osg_runtime_init(&s,&defaults,&backend));assert(interrupted.operations==cut);
        interrupted.fail_at=0;interrupted.operations=0;
        assert(osg_runtime_init(&reboot,&defaults,&backend));
        assert(reboot.generation==1 && reboot.saved_generation==1 && !strcmp(reboot.active.name,"test") && !reboot.owner);
        interrupted.operations=0;assert(osg_runtime_init(&s,&defaults,&backend));assert(!interrupted.operations);
    }
    /* The marker is last. Every incomplete byte prefix is an uncommitted
     * deterministic baseline; a fully programmed valid marker is simply loaded. */
    for(unsigned bytes=0;bytes<=8;bytes++) {
        flash_t interrupted=initial;memset(interrupted.pages[0]+24+bytes,255,8-bytes);backend=io(&interrupted);
        assert(osg_runtime_init(&s,&defaults,&backend));assert(s.generation==1 && s.saved_generation==1);
        assert(interrupted.operations==(bytes==8?0:count));
    }
    flash_t repeated=initial;memset(repeated.pages[0]+28,255,4);repeated.fail_at=1;backend=io(&repeated);
    assert(!osg_runtime_init(&s,&defaults,&backend)); /* recovery itself loses power during erase */
    repeated.fail_at=0;assert(osg_runtime_init(&s,&defaults,&backend));assert(s.saved_generation==1);
    flash_t corrupt=initial;corrupt.pages[0][32]^=1;backend=io(&corrupt);
    assert(!osg_runtime_init(&s,&defaults,&backend) && !corrupt.operations); /* committed corruption */
    corrupt=initial;memset(corrupt.pages[0]+24,255,8);corrupt.pages[1][0]=0;backend=io(&corrupt);
    assert(!osg_runtime_init(&s,&defaults,&backend) && !corrupt.operations); /* other page is not virgin */
    corrupt=initial;memset(corrupt.pages[0]+24,255,8);corrupt.pages[0][OSG_JOURNAL_PAGE_SIZE-1]=0;backend=io(&corrupt);
    assert(!osg_runtime_init(&s,&defaults,&backend) && !corrupt.operations); /* bits outside baseline */
}
static void persistence_test(void) {
    flash_t first;memset(&first,0,sizeof first);memset(first.pages,255,sizeof first.pages);osg_journal_io_t backend=io(&first);
    osg_runtime_config_t s;assert(osg_runtime_init(&s,&defaults,&backend));
    assert(s.saved_generation==1);ok(&s,"claim 1",0);
    flash_t initial=first;first.operations=0;
    ok(&s,"name 1 7361766564",2);ok(&s,"apply_save 1",3);assert(s.saved_generation==2 && s.generation==1);
    unsigned count=first.operations;assert(count>10);assert(osg_runtime_finish_apply(&s));
    osg_runtime_config_t reboot;assert(osg_runtime_init(&reboot,&defaults,&backend));
    assert(!strcmp(reboot.active.name,"saved") && reboot.generation==2 && !reboot.owner && !reboot.dirty);
    for(unsigned cut=1;cut<=count;cut++) {
        flash_t interrupted=initial;interrupted.operations=0;interrupted.fail_at=cut;backend=io(&interrupted);
        assert(osg_runtime_init(&s,&defaults,&backend));ok(&s,"claim 7",0);ok(&s,"name 7 7361766564",1);
        error(&s,"apply_save 7","flash_save_failed",2);
        assert(!s.pending_apply && s.generation==1 && !strcmp(s.active.name,"test"));
        interrupted.fail_at=0;interrupted.operations=0;
        assert(osg_runtime_init(&reboot,&defaults,&backend));
        assert(reboot.generation==1 && reboot.saved_generation==1 && !strcmp(reboot.active.name,"test") && !interrupted.operations);
        /* Retry can erase the incomplete destination and finish the first save. */
        ok(&reboot,"claim 8",3);ok(&reboot,"name 8 7361766564",4);ok(&reboot,"apply_save 8",5);
        assert(osg_runtime_finish_apply(&reboot) && reboot.generation==2 && reboot.saved_generation==2);
        interrupted.operations=0;assert(osg_runtime_init(&s,&defaults,&backend));
        assert(s.generation==2 && !strcmp(s.active.name,"saved") && !interrupted.operations);
    }
    for(unsigned cut=1;cut<=count;cut++) {
        flash_t interrupted=initial;interrupted.operations=0;backend=io(&interrupted);
        assert(osg_runtime_init(&s,&defaults,&backend));ok(&s,"claim 7",0);ok(&s,"name 7 7361766564",1);
        ok(&s,"apply 7",2);assert(osg_runtime_finish_apply(&s) && s.generation==2 && s.saved_generation==1);
        interrupted.fail_at=cut;error(&s,"save 7","flash_save_failed",3);
        assert(s.generation==2 && s.saved_generation==1 && !s.pending_apply && !strcmp(s.active.name,"saved"));
        interrupted.fail_at=0;interrupted.operations=0;
        assert(osg_runtime_init(&reboot,&defaults,&backend));
        assert(reboot.generation==1 && !strcmp(reboot.active.name,"test") && !interrupted.operations);
    }
    flash_t corrupt;memset(&corrupt,0,sizeof corrupt);memset(corrupt.pages,255,sizeof corrupt.pages);corrupt.pages[0][1]=0;backend=io(&corrupt);
    assert(!osg_runtime_init(&s,&defaults,&backend));
    backend=io(&initial);assert(osg_runtime_init(&s,&defaults,&backend));
    uint8_t record[OSG_RUNTIME_RECORD_SIZE];assert(osg_runtime_encode(&s.active,1,record,sizeof record));
    record[78+3*2]=42;osg_runtime_definition_t decoded;uint32_t generation;
    assert(!osg_runtime_decode(&decoded,&generation,record,sizeof record)); /* disabled button is reserved */
    /* A journal CRC cannot make an invalid configuration acceptable. */
    assert(osg_journal_commit(&backend,record,sizeof record,1));initial.operations=0;
    assert(!osg_runtime_init(&s,&defaults,&backend) && !initial.operations);
    s.active.buttons[2].input=42;
    assert(!osg_runtime_encode(&s.active,1,record,sizeof record));
}
static void definition_defaults_test(void) {
    osg_runtime_config_t s;assert(osg_runtime_init(&s,&defaults,NULL));
    osg_runtime_definition_t definition=s.defaults;
    definition.axis_options[0]=(osg_runtime_axis_t){.inverted=true,.deadzone=1234};
    flash_t flash={0};memset(flash.pages,255,sizeof flash.pages);osg_journal_io_t backend=io(&flash);
    assert(osg_runtime_init_definition(&s,&definition,&backend));
    assert(s.active.axis_options[0].inverted && s.active.axis_options[0].deadzone==1234);
    ok(&s,"claim 1",0);ok(&s,"axis 1 0 0 48 100 4000 32768 0 400",1);
    ok(&s,"apply_save 1",2);assert(osg_runtime_finish_apply(&s));
    assert(osg_runtime_init_definition(&s,&definition,&backend));
    assert(!s.active.axis_options[0].inverted && s.active.axis_options[0].deadzone==400);
    ok(&s,"claim 2",3);ok(&s,"reset 2",4);ok(&s,"apply 2",5);assert(osg_runtime_finish_apply(&s));
    assert(s.active.axis_options[0].inverted && s.active.axis_options[0].deadzone==1234);
    definition.axis_options[0].deadzone=32768;
    assert(!osg_runtime_init_definition(&s,&definition,NULL));
}
int main(void) { definition_defaults_test();transaction_test();axis_test();maximum_test();initial_baseline_test();persistence_test();puts("runtime configuration tests passed");return 0; }
