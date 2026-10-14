#include "runtime_config.h"
#include "opensimgear/gateway_protocol.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static osg_profile_t view(const osg_runtime_definition_t *d) {
    return (osg_profile_t){.name=d->name,.buttons=d->buttons,.button_count=d->button_count,
        .axes=d->axes,.axis_count=d->axis_count,.hats=d->hats,.hat_count=d->hat_count,
        .encoders=d->encoders,.encoder_count=d->encoder_count,.selectors=d->selectors,
        .selector_count=d->selector_count,.expander_mask=d->expander_mask};
}
static bool utf8_next(const char **cursor,uint32_t *value) {
    const unsigned char *p=(const unsigned char *)*cursor; uint32_t c=*p++;
    unsigned extra=0; uint32_t minimum=0;
    if(c>=0xc2 && c<=0xdf) { extra=1;c&=31;minimum=0x80; }
    else if(c>=0xe0 && c<=0xef) { extra=2;c&=15;minimum=0x800; }
    else if(c>=0xf0 && c<=0xf4) { extra=3;c&=7;minimum=0x10000; }
    else if(c>=0x80) return false;
    for(unsigned i=0;i<extra;i++) { if((*p&0xc0)!=0x80) return false;c=(c<<6)|(*p++&63); }
    if(c<minimum || c>0x10ffff || (c>=0xd800 && c<=0xdfff)) return false;
    *cursor=(const char *)p;*value=c;return true;
}
static bool name_valid(const char *name) {
    size_t n=0;while(n<OSG_RUNTIME_NAME_CAPACITY && name[n]) n++;
    if(!n || n==OSG_RUNTIME_NAME_CAPACITY) return false;
    const char *p=name;while(*p) { uint32_t c;if(!utf8_next(&p,&c) || c<32 || c==127) return false; }
    for(size_t i=n+1;i<OSG_RUNTIME_NAME_CAPACITY;i++) if(name[i]) return false;
    return true;
}
size_t osg_runtime_utf16(const char *name,uint16_t *out,size_t capacity) {
    if(!name || !out) return 0;
    size_t length=0;const char *p=name;
    while(*p) {
        uint32_t c;if(!utf8_next(&p,&c)) return 0;
        size_t units=c>=0x10000?2U:1U;if(capacity-length<units) return 0;
        if(units==2) { c-=0x10000;out[length++]=(uint16_t)(0xd800|(c>>10));out[length++]=(uint16_t)(0xdc00|(c&1023)); }
        else out[length++]=(uint16_t)c;
    }
    return length;
}
static void canonicalize(osg_runtime_definition_t *d) {
    memset(d->buttons+d->button_count,0,(OSG_MAX_BUTTONS-d->button_count)*sizeof d->buttons[0]);
    memset(d->axes+d->axis_count,0,(OSG_ADC_CHANNELS-d->axis_count)*sizeof d->axes[0]);
    memset(d->axis_options+d->axis_count,0,(OSG_ADC_CHANNELS-d->axis_count)*sizeof d->axis_options[0]);
    memset(d->hats+d->hat_count,0,(OSG_MAX_HATS-d->hat_count)*sizeof d->hats[0]);
    memset(d->encoders+d->encoder_count,0,(OSG_MAX_ENCODERS-d->encoder_count)*sizeof d->encoders[0]);
    memset(d->selectors+d->selector_count,0,(OSG_MAX_SELECTORS-d->selector_count)*sizeof d->selectors[0]);
}
static bool valid(const osg_runtime_definition_t *d,osg_profile_layout_t *layout) {
    if(!name_valid(d->name)) return false;
    osg_profile_t p=view(d);if(osg_profile_validate(&p,layout)!=OSG_PROFILE_OK) return false;
    for(unsigned i=0;i<d->axis_count;i++) if(d->axis_options[i].deadzone>32767) return false;
    for(unsigned i=d->button_count;i<128;i++) if(d->buttons[i].input || d->buttons[i].maintained || d->buttons[i].exclusive_group) return false;
    for(unsigned i=d->axis_count;i<8;i++) if(d->axes[i].channel || d->axes[i].usage || d->axes[i].minimum || d->axes[i].maximum || d->axes[i].neutral || d->axis_options[i].inverted || d->axis_options[i].deadzone) return false;
    for(unsigned i=d->hat_count;i<4;i++) for(unsigned j=0;j<4;j++) if(d->hats[i].inputs[j]) return false;
    for(unsigned i=d->encoder_count;i<8;i++) if(d->encoders[i].a || d->encoders[i].b) return false;
    for(unsigned i=d->selector_count;i<8;i++) if(d->selectors[i].low || d->selectors[i].high) return false;
    return true;
}
static void put16(uint8_t **p,uint16_t v) { *(*p)++=(uint8_t)v;*(*p)++=(uint8_t)(v>>8); }
static uint16_t get16(const uint8_t **p) { uint16_t v=(*p)[0]|((uint16_t)(*p)[1]<<8);*p+=2;return v; }
static void put32(uint8_t **p,uint32_t v) { put16(p,(uint16_t)v);put16(p,(uint16_t)(v>>16)); }
static uint32_t get32(const uint8_t **p) { uint32_t low=get16(p);return low|((uint32_t)get16(p)<<16); }
bool osg_runtime_encode(const osg_runtime_definition_t *d,uint32_t generation,uint8_t *out,size_t capacity) {
    osg_profile_layout_t layout;
    if(!d || !generation || !out || capacity<OSG_RUNTIME_RECORD_SIZE || !valid(d,&layout)) return false;
    osg_runtime_definition_t clean=*d;canonicalize(&clean);const osg_runtime_definition_t *c=&clean;
    uint8_t *p=out;memcpy(p,"OSGC",4);p+=4;put32(&p,generation);memcpy(p,c->name,64);p+=64;
    *p++=c->button_count;*p++=c->axis_count;*p++=c->hat_count;*p++=c->encoder_count;*p++=c->selector_count;*p++=c->expander_mask;
    for(unsigned i=0;i<128;i++) { *p++=c->buttons[i].input;*p++=c->buttons[i].maintained;*p++=c->buttons[i].exclusive_group; }
    for(unsigned i=0;i<8;i++) {
        const osg_profile_axis_t *a=&c->axes[i];*p++=a->channel;*p++=a->usage;put16(&p,a->minimum);put16(&p,a->maximum);put16(&p,a->neutral);
        *p++=c->axis_options[i].inverted;put16(&p,c->axis_options[i].deadzone);
    }
    for(unsigned i=0;i<4;i++) for(unsigned j=0;j<4;j++) *p++=c->hats[i].inputs[j];
    for(unsigned i=0;i<8;i++) { *p++=c->encoders[i].a;*p++=c->encoders[i].b; }
    for(unsigned i=0;i<8;i++) { *p++=c->selectors[i].low;*p++=c->selectors[i].high; }
    return (size_t)(p-out)==OSG_RUNTIME_RECORD_SIZE;
}
bool osg_runtime_decode(osg_runtime_definition_t *out,uint32_t *generation,const uint8_t *data,size_t length) {
    if(!out || !generation || !data || length!=OSG_RUNTIME_RECORD_SIZE || memcmp(data,"OSGC",4)) return false;
    osg_runtime_definition_t d={0};const uint8_t *p=data+4;uint32_t g=get32(&p);memcpy(d.name,p,64);p+=64;
    d.button_count=*p++;d.axis_count=*p++;d.hat_count=*p++;d.encoder_count=*p++;d.selector_count=*p++;d.expander_mask=*p++;
    for(unsigned i=0;i<128;i++) { d.buttons[i].input=*p++;if(*p>1) return false;d.buttons[i].maintained=*p++;d.buttons[i].exclusive_group=*p++; }
    for(unsigned i=0;i<8;i++) {
        osg_profile_axis_t *a=&d.axes[i];a->channel=*p++;a->usage=*p++;a->minimum=get16(&p);a->maximum=get16(&p);a->neutral=get16(&p);
        if(*p>1) return false;
        d.axis_options[i].inverted=*p++;d.axis_options[i].deadzone=get16(&p);
    }
    for(unsigned i=0;i<4;i++) for(unsigned j=0;j<4;j++) d.hats[i].inputs[j]=*p++;
    for(unsigned i=0;i<8;i++) { d.encoders[i].a=*p++;d.encoders[i].b=*p++; }
    for(unsigned i=0;i<8;i++) { d.selectors[i].low=*p++;d.selectors[i].high=*p++; }
    uint8_t canonical[OSG_RUNTIME_RECORD_SIZE];
    if(!osg_runtime_encode(&d,g,canonical,sizeof canonical) || memcmp(canonical,data,sizeof canonical)) return false;
    *out=d;*generation=g;return true;
}
static bool prepare(osg_runtime_config_t *s) {
    s->profile=view(&s->active);
    if(!valid(&s->active,&s->layout)) return false;
    s->descriptor_length=osg_profile_descriptor(&s->profile,s->descriptor,sizeof s->descriptor);
    return s->descriptor_length!=0 || s->layout.length==0;
}
/* The pinned journal's first write has sequence/epoch 1, this exact factory
 * payload, and a commit marker programmed last. A torn first initialization can
 * be erased and retried only while the other page is still virgin. Every clear
 * bit must belong to that deterministic image, including partially programmed
 * doublewords or a partially erased retry. A complete commit marker with bad
 * data is corruption and is never repaired here. This does not accept arbitrary
 * damaged user settings, other generations, or a valid record with bad schema. */
static bool interrupted_baseline(const osg_journal_io_t *io,const uint8_t record[OSG_RUNTIME_RECORD_SIZE]) {
    const uint8_t *first=io->read(io->context,0),*second=io->read(io->context,1);
    if(!first || !second) return false;
    for(unsigned i=0;i<OSG_JOURNAL_PAGE_SIZE;i++) if(second[i]!=255) return false;
    uint8_t header[32];memset(header,255,sizeof header);uint8_t *p=header;
    put32(&p,UINT32_C(0x314a474f));put32(&p,1);put32(&p,OSG_RUNTIME_RECORD_SIZE);put32(&p,1);
    uint8_t scratch[16+OSG_RUNTIME_RECORD_SIZE];memcpy(scratch,header,16);memcpy(scratch+16,record,OSG_RUNTIME_RECORD_SIZE);
    put32(&p,osg_gateway_crc32(scratch,sizeof scratch));put32(&p,0);memcpy(p,"OGCOMMIT",8);
    if(!memcmp(first+24,header+24,8)) return false;
    bool touched=false;
    for(unsigned i=0;i<OSG_JOURNAL_PAGE_SIZE;i++) {
        uint8_t expected=i<32?header[i]:i<32+OSG_RUNTIME_RECORD_SIZE?record[i-32]:255;
        if((first[i]&expected)!=expected) return false;
        touched|=first[i]!=255;
    }
    return touched;
}
bool osg_runtime_init(osg_runtime_config_t *s,const osg_profile_t *defaults,const osg_journal_io_t *io) {
    osg_profile_layout_t layout;if(!s || osg_profile_validate(defaults,&layout)!=OSG_PROFILE_OK) return false;
    size_t length=strlen(defaults->name);if(!length || length>=64) return false;
    osg_runtime_definition_t definition={0};osg_runtime_definition_t *d=&definition;memcpy(d->name,defaults->name,length+1);
    d->button_count=defaults->button_count;d->axis_count=defaults->axis_count;d->hat_count=defaults->hat_count;
    d->encoder_count=defaults->encoder_count;d->selector_count=defaults->selector_count;d->expander_mask=defaults->expander_mask;
#define COPY(field,count) do { if(d->count) memcpy(d->field,defaults->field,d->count*sizeof d->field[0]); } while(0)
    COPY(buttons,button_count);COPY(axes,axis_count);COPY(hats,hat_count);COPY(encoders,encoder_count);COPY(selectors,selector_count);
#undef COPY
    return osg_runtime_init_definition(s,d,io);
}

bool osg_runtime_init_definition(osg_runtime_config_t *s,const osg_runtime_definition_t *defaults,const osg_journal_io_t *io) {
    osg_profile_layout_t layout;if(!s || !defaults || !valid(defaults,&layout)) return false;
    /* Copy first so callers may reuse a definition owned by this runtime. */
    osg_runtime_definition_t definition=*defaults;
    memset(s,0,sizeof *s);s->defaults=definition;
    const osg_runtime_definition_t *d=&s->defaults;
    s->active=*d;s->generation=1;
    if(io) {
        if(!io->read || !io->erase || !io->program) return false;
        s->settings=*io;s->persistence=true;uint8_t record[OSG_RUNTIME_RECORD_SIZE];size_t n=0;uint32_t journal_generation=0;
        if(osg_journal_load(io,record,sizeof record,&n,&journal_generation)) {
            if(!osg_runtime_decode(&s->active,&s->generation,record,n) || s->generation!=journal_generation) return false;
            s->saved_generation=s->generation;
        } else {
            if(!osg_runtime_encode(&s->active,1,record,sizeof record)) return false;
            if(!osg_journal_erased(io) && !interrupted_baseline(io,record)) return false;
            /* Establish recovery before accepting the first user transaction. */
            if(!osg_journal_commit(io,record,sizeof record,1)) return false;
            s->saved_generation=1;
        }
    }
    s->draft=s->active;return prepare(s);
}
void osg_runtime_invalidate_claim(osg_runtime_config_t *s) {
    if(!s) return;
    s->owner=0;s->claim_expires_ms=0;
    if(!s->pending_apply) { s->draft=s->active;s->dirty=false; }
}
void osg_runtime_tick(osg_runtime_config_t *s,uint32_t now) {
    if(s && s->owner && (int32_t)(now-s->claim_expires_ms)>=0) osg_runtime_invalidate_claim(s);
}
static bool persist(osg_runtime_config_t *s,const osg_runtime_definition_t *d,uint32_t generation) {
    uint8_t record[OSG_RUNTIME_RECORD_SIZE];
    if(!s->persistence || !osg_runtime_encode(d,generation,record,sizeof record) ||
       !osg_journal_commit(&s->settings,record,sizeof record,generation)) return false;
    s->saved_generation=generation;return true;
}
bool osg_runtime_finish_apply(osg_runtime_config_t *s) {
    if(!s || !s->pending_apply || s->generation==UINT32_MAX) return false;
    s->active=s->draft;s->generation++;s->pending_apply=false;s->dirty=false;return prepare(s);
}
static bool token_u32(const char **cursor,uint32_t *v) {
    const char *p=*cursor;uint32_t n=0;unsigned digits=0;
    while(*p>='0' && *p<='9') { uint32_t d=(uint32_t)(*p-'0');if(n>(UINT32_MAX-d)/10U) return false;n=n*10U+d;p++;digits++; }
    if(!digits || (*p && *p!=' ')) return false;
    *v=n;*cursor=*p?p+1:p;return true;
}
static bool numbers(const char *p,uint32_t *out,unsigned count) {
    for(unsigned i=0;i<count;i++) if(!token_u32(&p,&out[i])) return false;
    return !*p;
}
static int hex_digit(char c) { return c>='0' && c<='9'?c-'0':c>='a' && c<='f'?c-'a'+10:c>='A' && c<='F'?c-'A'+10:-1; }
const char *osg_runtime_command(osg_runtime_config_t *s,const char *line,uint32_t now,bool *apply_requested) {
    if(apply_requested) *apply_requested=false;
    if(!s || !line || !apply_requested) return "invalid_request";
    size_t length=0;while(length<OSG_RUNTIME_COMMAND_CAPACITY && line[length]) length++;
    if(!length || length>=OSG_RUNTIME_COMMAND_CAPACITY || line[length-1]==' ') return "invalid_request";
    osg_runtime_tick(s,now);
    if(!strcmp(line,"get") || !strcmp(line,"get_active")) return NULL;
    char op[16];const char *p=strchr(line,' ');if(!p || p-line>=16) return "invalid_request";
    memcpy(op,line,(size_t)(p-line));op[p-line]=0;p++;
    uint32_t owner;if(!token_u32(&p,&owner) || !owner) return "invalid_request";
    if(!strcmp(op,"claim")) {
        if(*p) return "invalid_request";
        if(s->pending_apply) return "apply_pending";
        if(s->owner && s->owner!=owner) return "writer_busy";
        if(!s->owner) { s->draft=s->active;s->dirty=false; }
        s->owner=owner;s->claim_expires_ms=now+OSG_RUNTIME_CLAIM_MS;return NULL;
    }
    if(s->owner!=owner) return "writer_required";
    if(s->pending_apply) return "apply_pending";
    uint32_t a[9]={0};bool changed=false;
    if(!strcmp(op,"release") || !strcmp(op,"discard") || !strcmp(op,"reset")) {
        if(*p) return "invalid_request";
        s->draft=!strcmp(op,"reset")?s->defaults:s->active;s->dirty=!strcmp(op,"reset");
        if(!strcmp(op,"release")) { osg_runtime_invalidate_claim(s);return NULL; }
    } else if(!strcmp(op,"counts")) {
        if(!numbers(p,a,6) || a[0]>128 || a[1]>8 || a[2]>4 || a[3]>8 || a[4]>8 || a[5]>255) return "invalid_counts";
        s->draft.button_count=(uint8_t)a[0];s->draft.axis_count=(uint8_t)a[1];s->draft.hat_count=(uint8_t)a[2];
        s->draft.encoder_count=(uint8_t)a[3];s->draft.selector_count=(uint8_t)a[4];s->draft.expander_mask=(uint8_t)a[5];canonicalize(&s->draft);changed=true;
    } else if(!strcmp(op,"name")) {
        size_t n=strlen(p);char name[64]={0};if(!n || n%2 || n>126) return "invalid_name";
        for(size_t i=0;i<n/2;i++) { int h=hex_digit(p[2*i]),l=hex_digit(p[2*i+1]);if(h<0 || l<0 || !(h|l)) return "invalid_name";name[i]=(char)((h<<4)|l); }
        if(!name_valid(name)) return "invalid_name";
        memcpy(s->draft.name,name,sizeof name);changed=true;
    } else if(!strcmp(op,"button")) {
        if(!numbers(p,a,4) || a[0]>=s->draft.button_count || a[1]>=152 || a[2]>1 || a[3]>255) return "invalid_button";
        s->draft.buttons[a[0]]=(osg_profile_button_t){(uint8_t)a[1],a[2]!=0,(uint8_t)a[3]};changed=true;
    } else if(!strcmp(op,"axis")) {
        if(!numbers(p,a,8) || a[0]>=s->draft.axis_count || a[1]>=8 || a[2]<48 || a[2]>56 || a[3]>=a[4] || a[4]>4095 || a[5]>65535 || a[6]>1 || a[7]>32767) return "invalid_axis";
        s->draft.axes[a[0]]=(osg_profile_axis_t){(uint8_t)a[1],(uint8_t)a[2],(uint16_t)a[3],(uint16_t)a[4],(uint16_t)a[5]};
        s->draft.axis_options[a[0]]=(osg_runtime_axis_t){a[6]!=0,(uint16_t)a[7]};changed=true;
    } else if(!strcmp(op,"hat")) {
        if(!numbers(p,a,5) || a[0]>=s->draft.hat_count) return "invalid_hat";
        for(unsigned i=1;i<=4;i++) if(a[i]>=152) return "invalid_hat";
        for(unsigned i=0;i<4;i++) s->draft.hats[a[0]].inputs[i]=(uint8_t)a[i+1];
        changed=true;
    } else if(!strcmp(op,"encoder")) {
        if(!numbers(p,a,3) || a[0]>=s->draft.encoder_count || a[1]>=24 || a[2]>=24) return "invalid_encoder";
        s->draft.encoders[a[0]]=(osg_profile_encoder_t){(uint8_t)a[1],(uint8_t)a[2]};changed=true;
    } else if(!strcmp(op,"selector")) {
        if(!numbers(p,a,3) || a[0]>=s->draft.selector_count || a[1]>=152 || a[2]>=152) return "invalid_selector";
        s->draft.selectors[a[0]]=(osg_profile_selector_t){(uint8_t)a[1],(uint8_t)a[2]};changed=true;
    } else if(!strcmp(op,"apply") || !strcmp(op,"apply_save") || !strcmp(op,"save")) {
        if(*p) return "invalid_request";
        bool save_only=!strcmp(op,"save"),saving=save_only || !strcmp(op,"apply_save");
        if(!save_only) { osg_profile_layout_t layout;if(!valid(&s->draft,&layout)) return "invalid_profile";if(s->generation==UINT32_MAX) return "generation_exhausted"; }
        uint32_t g=s->generation+(save_only?0U:1U);
        if(saving && !persist(s,save_only?&s->active:&s->draft,g)) return s->persistence?"flash_save_failed":"persistence_unavailable";
        if(!save_only) { s->pending_apply=true;*apply_requested=true; }
    } else return "unknown_command";
    if(changed) s->dirty=true;
    s->claim_expires_ms=now+OSG_RUNTIME_CLAIM_MS;return NULL;
}
void osg_runtime_transform_report(const osg_runtime_config_t *s,const osg_profile_state_t *state,uint8_t *report) {
    if(!s || !state || !report) return;
    for(unsigned i=0;i<s->active.axis_count;i++) if(state->axis_available&(1U<<i)) {
        unsigned offset=state->layout.axis_offset+2*i;uint32_t value=report[offset]|((uint32_t)report[offset+1]<<8);
        if(s->active.axis_options[i].inverted) value=65535U-value;
        uint32_t center=s->active.axes[i].neutral,dead=s->active.axis_options[i].deadzone;
        if(dead) {
            uint32_t low=center>dead?center-dead:0,high=center+dead<65535U?center+dead:65535U;
            if(value>=low && value<=high) value=center;
            else if(value<low) value=low?value*center/low:0;
            else value=high<65535U?center+(value-high)*(65535U-center)/(65535U-high):65535U;
        }
        report[offset]=(uint8_t)value;report[offset+1]=(uint8_t)(value>>8);
    }
}
typedef struct { char *out;size_t size,length;bool ok; } writer_t;
static void append(writer_t *w,const char *format,...) {
    if(!w->ok) return;
    va_list args;va_start(args,format);int n=vsnprintf(w->out+w->length,w->size-w->length,format,args);va_end(args);
    if(n<0 || (size_t)n>=w->size-w->length) { w->ok=false;return; }w->length+=(size_t)n;
}
static void quoted(writer_t *w,const char *p) {
    append(w,"\"");for(;*p;p++) { if(*p=='"' || *p=='\\') append(w,"\\%c",*p);else append(w,"%c",*p); }append(w,"\"");
}
size_t osg_runtime_snapshot(const osg_runtime_config_t *s,bool active_view,char *out,size_t capacity) {
    if(!s || !out || !capacity) return 0;
    out[0]=0;
    bool draft=s->owner && !active_view;const osg_runtime_definition_t *d=draft?&s->draft:&s->active;
    osg_profile_layout_t layout;bool checked=valid(d,&layout);osg_profile_t p=view(d);uint8_t descriptor[256];
    size_t descriptor_length=checked?osg_profile_descriptor(&p,descriptor,sizeof descriptor):0;
    writer_t w={out,capacity,0,true};
    append(&w,"{\"generation\":%lu,\"savedGeneration\":%lu,\"owner\":%lu,\"claimExpiresMs\":%lu,\"dirty\":%s,\"pendingApply\":%s,\"view\":\"%s\",\"valid\":%s,\"config\":{\"name\":",
        (unsigned long)s->generation,(unsigned long)s->saved_generation,(unsigned long)s->owner,(unsigned long)s->claim_expires_ms,
        s->dirty?"true":"false",s->pending_apply?"true":"false",draft?"draft":"active",checked?"true":"false");quoted(&w,d->name);
    append(&w,",\"expanderMask\":%u,\"buttons\":[",d->expander_mask);
    for(unsigned i=0;i<d->button_count;i++) append(&w,"%s[%u,%u,%u]",i?",":"",d->buttons[i].input,d->buttons[i].maintained,d->buttons[i].exclusive_group);
    append(&w,"],\"axes\":[");for(unsigned i=0;i<d->axis_count;i++) { const osg_profile_axis_t *a=&d->axes[i];append(&w,"%s[%u,%u,%u,%u,%u,%u,%u]",i?",":"",a->channel,a->usage,a->minimum,a->maximum,a->neutral,d->axis_options[i].inverted,d->axis_options[i].deadzone); }
    append(&w,"],\"hats\":[");for(unsigned i=0;i<d->hat_count;i++) append(&w,"%s[%u,%u,%u,%u]",i?",":"",d->hats[i].inputs[0],d->hats[i].inputs[1],d->hats[i].inputs[2],d->hats[i].inputs[3]);
    append(&w,"],\"encoders\":[");for(unsigned i=0;i<d->encoder_count;i++) append(&w,"%s[%u,%u]",i?",":"",d->encoders[i].a,d->encoders[i].b);
    append(&w,"],\"selectors\":[");for(unsigned i=0;i<d->selector_count;i++) append(&w,"%s[%u,%u]",i?",":"",d->selectors[i].low,d->selectors[i].high);
    append(&w,"]},\"layout\":");
    if(checked) append(&w,"{\"buttons\":%u,\"axes\":%u,\"hats\":%u,\"reportBytes\":%u}",layout.buttons,layout.axes,layout.hats,layout.length);else append(&w,"null");
    append(&w,",\"descriptorHex\":\"");for(size_t i=0;i<descriptor_length;i++) append(&w,"%02x",descriptor[i]);append(&w,"\"}");
    if(!w.ok) { out[0]=0;return 0; }return w.length;
}
