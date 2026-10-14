#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE
#define _DEFAULT_SOURCE
/* Native device peripherals around the production module firmware. */
#include "opensimgear/line.h"
#include "config_can.h"
#include "can_delivery.h"
#include "boot_session.h"
#include "device_actions.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <limits.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>


typedef struct { uint8_t pages[2][OSG_JOURNAL_PAGE_SIZE]; int fault; const char *name; uint32_t erases,programs; } flash_t;
typedef struct {
    char id[32],label[64],endpoint[32],text[OSG_ACTION_STRING_CAPACITY];
    uint32_t writes,delay_ms;
    bool write_fail,read_fail;
} native_display_t;
static struct {
    osg_can_client_t module;
    osg_module_config_can_t module_config;
    osg_runtime_config_t runtime;
    osg_runtime_definition_t defaults;
    osg_device_actions_t actions;
    osg_device_display_port_t display_ports[OSG_DEVICE_DISPLAY_MAX];
    native_display_t displays[OSG_DEVICE_DISPLAY_MAX];
    uint8_t display_count,axes_available;
    osg_events_t events;
    osg_profile_state_t acquisition;
    osg_profile_sample_t sample;
    osg_can_delivery_t delivery;
    uint8_t report[OSG_REPORT_CAPACITY], expansion_available;
    flash_t settings_flash,boot_flash;
    osg_gateway_identity_t module_identity;
    uint32_t now,module_reconnects,can_frames,acks,node_collisions;
    bool direct,board_actions;
    char active_config[4096];
    char action_result[OSG_ACTION_REPLY_CAPACITY];
    char event_result[OSG_EVENTS_REPLY_CAPACITY];
} emulator;
static const char *state_directory;
static const char *simulation_namespace;
static char gateway_uid[25]="00112233445566778899aabb";
static FILE *response_output;
static volatile sig_atomic_t stopped;
static void standalone_cycle(bool wait);
static bool bus_send(const osg_gateway_frame_t *frame);
static bool module_send(void *context,const osg_gateway_frame_t *frame) {(void)context;return bus_send(frame);}
static void transform_event_axes(void *context,const osg_profile_state_t *state,uint8_t report[OSG_REPORT_CAPACITY]) {
    osg_runtime_transform_report(context,state,report);
}
static bool display_write(void *context,const char *text) {
    native_display_t *display=context;
    if(display->write_fail || strlen(text)>=sizeof display->text) return false;
    /* Model a peripheral's bounded synchronous latency in the real output
     * callback. Never recursively advance acquisition from a driver callback. */
    struct timespec delay={.tv_sec=0,.tv_nsec=(long)display->delay_ms*1000000L};
    while(nanosleep(&delay,&delay)<0 && errno==EINTR && !stopped) {}
    strcpy(display->text,text);
    if(display->writes<UINT32_MAX) display->writes++;
    return true;
}
static bool display_read(void *context,char *text,size_t capacity) {
    native_display_t *display=context;
    size_t length=strlen(display->text);
    if(display->read_fail || length>=capacity) return false;
    memcpy(text,display->text,length+1);return true;
}

static void disk_save(flash_t *flash) {
    if(!state_directory) return;
    char path[1024]; int n=snprintf(path,sizeof path,"%s/%s.bin",state_directory,flash->name);
    assert(n>0 && (size_t)n<sizeof path);
    FILE *file=fopen(path,"wb"); assert(file);
    assert(fwrite(flash->pages,sizeof flash->pages,1,file)==1); assert(!fclose(file));
}

static void flash_init(flash_t *flash,const char *name) {
    memset(flash->pages,255,sizeof flash->pages); flash->fault=-1; flash->name=name;
    if(!state_directory) return;
    char path[1024]; int n=snprintf(path,sizeof path,"%s/%s.bin",state_directory,name);
    assert(n>0 && (size_t)n<sizeof path); FILE *file=fopen(path,"rb");
    if(file) { assert(fread(flash->pages,sizeof flash->pages,1,file)==1); assert(!fclose(file)); }
}

static const uint8_t *flash_read(void *context,unsigned page) {
    flash_t *flash=context; return page<2 ? flash->pages[page] : NULL;
}

static bool interrupted(flash_t *flash) {
    if(flash->fault<0) return false;
    if(!flash->fault) { flash->fault=-1; return true; }
    flash->fault--; return false;
}

static bool flash_erase(void *context,unsigned page) {
    flash_t *flash=context; if(page>1) return false;
    flash->erases++;
    bool fail=interrupted(flash); memset(flash->pages[page],255,fail ? OSG_JOURNAL_PAGE_SIZE/2 : OSG_JOURNAL_PAGE_SIZE);
    disk_save(flash); return !fail;
}

static bool flash_program(void *context,unsigned page,unsigned offset,const uint8_t data[8]) {
    flash_t *flash=context; if(page>1 || offset%8 || offset+8>OSG_JOURNAL_PAGE_SIZE) return false;
    flash->programs++;
    bool fail=interrupted(flash); unsigned length=fail ? 4U : 8U;
    for(unsigned i=0;i<length;i++) {
        if((flash->pages[page][offset+i]&data[i])!=data[i]) return false;
        flash->pages[page][offset+i]&=data[i];
    }
    disk_save(flash); return !fail;
}

static osg_journal_io_t io(flash_t *flash) {
    return (osg_journal_io_t){.context=flash,.read=flash_read,.erase=flash_erase,.program=flash_program};
}

static void rearm(void) {
    assert(osg_profile_init(&emulator.acquisition,&emulator.runtime.profile,&emulator.sample,emulator.now));
    osg_profile_neutral(&emulator.acquisition,emulator.report); osg_can_delivery_rearm(&emulator.delivery);
}

static void init_module_client(uint32_t boot) {
    osg_gateway_profile_t p; assert(osg_can_profile(&emulator.runtime.profile,&emulator.runtime.layout,&p));
    assert(osg_can_client_init(&emulator.module,&emulator.module_identity,&p,boot,module_send,NULL));
    emulator.module.configuration_generation=emulator.runtime.generation;
    osg_module_config_can_init(&emulator.module_config,&emulator.module,&emulator.runtime);
    osg_module_config_can_actions(&emulator.module_config,&emulator.actions.registry);
    osg_module_config_can_events(&emulator.module_config,&emulator.events);
    osg_can_client_usb(&emulator.module,emulator.direct); rearm();
}

static void boot_module(void) {
    osg_cyphal_destroy(&emulator.module.cyphal);
    uint32_t boot; osg_journal_io_t settings=io(&emulator.settings_flash),counter=io(&emulator.boot_flash);
    assert(osg_runtime_init_definition(&emulator.runtime,&emulator.defaults,&settings));
    for(unsigned i=0;i<emulator.display_count;i++) {
        emulator.displays[i].text[0]=0;emulator.displays[i].writes=emulator.displays[i].delay_ms=0;
        emulator.displays[i].write_fail=emulator.displays[i].read_fail=false;
    }
    assert(osg_device_actions_init_outputs(&emulator.actions,!emulator.board_actions,emulator.display_ports,emulator.display_count));emulator.action_result[0]=0;
    assert(osg_events_init(&emulator.events,&emulator.runtime.profile,&emulator.sample,emulator.now,emulator.runtime.generation));
    osg_events_set_transform(&emulator.events,transform_event_axes,&emulator.runtime);
    emulator.event_result[0]=0;
    assert(osg_boot_session_begin(&counter,&boot)); init_module_client(boot);
}

static void receive_module(const osg_gateway_frame_t *frame) {
    emulator.can_frames++;
    uint32_t generation=emulator.module.generation;
    uint8_t source=osg_cyphal_node_id(&emulator.module.cyphal);
    if(osg_can_client_receive(&emulator.module,frame,emulator.now)) {
        emulator.acks++; osg_can_delivery_completed(&emulator.delivery,&emulator.acquisition,
            emulator.module.ack_sequence,emulator.module.acknowledged,emulator.now);
    }
    if(source!=osg_cyphal_node_id(&emulator.module.cyphal)) emulator.node_collisions++;
    if(generation!=emulator.module.generation) rearm();
}

static void tick(void) {
    osg_runtime_tick(&emulator.runtime,emulator.now);
    emulator.sample.axes_fresh=(uint8_t)(emulator.runtime.layout.adc_mask&emulator.axes_available);
    emulator.sample.expansion_available=emulator.expansion_available;
    osg_events_observe(&emulator.events,&emulator.sample,emulator.now);
    osg_profile_update(&emulator.acquisition,&emulator.sample,emulator.now,
        osg_can_client_active(&emulator.module) || emulator.direct,emulator.report);
    osg_runtime_transform_report(&emulator.runtime,&emulator.acquisition,emulator.report);
    osg_can_delivery_observe(&emulator.delivery,&emulator.acquisition,emulator.module.next_sequence);
    uint8_t routed[OSG_GW_MAX_REPORT_SIZE]={0};memcpy(routed,emulator.report,emulator.runtime.layout.length);
    osg_can_client_tick(&emulator.module,emulator.now,routed);
    osg_module_config_can_tick(&emulator.module_config,emulator.now);
    if(emulator.runtime.pending_apply && !osg_module_config_can_busy(&emulator.module_config)) {
        assert(osg_runtime_finish_apply(&emulator.runtime));emulator.module_reconnects++;
        assert(osg_events_init(&emulator.events,&emulator.runtime.profile,&emulator.sample,emulator.now,emulator.runtime.generation));
        osg_events_set_transform(&emulator.events,transform_event_axes,&emulator.runtime);
        assert(osg_can_profile(&emulator.runtime.profile,&emulator.runtime.layout,&emulator.module.profile));
        emulator.module.configuration_generation=emulator.runtime.generation;
        osg_can_client_reset(&emulator.module);rearm();
    }
}
static void advance(unsigned milliseconds) {
    uint32_t start=emulator.now;
    do {standalone_cycle(true);} while(!stopped && (uint32_t)(emulator.now-start)<milliseconds);
}

static void hex(const uint8_t *bytes,size_t n) { for(size_t i=0;i<n;i++) fprintf(response_output,"%02x",bytes[i]); }
static void json_text(const char *text) {
    fputc('"',response_output);
    for(const unsigned char *p=(const unsigned char *)text;*p;p++) {
        if(*p=='"' || *p=='\\') fputc('\\',response_output);
        if(*p<32) fprintf(response_output,"\\u%04x",*p);else fputc(*p,response_output);
    }
    fputc('"',response_output);
}
static void contact(uint8_t input,bool closed) {
    if(input<24) {
        uint32_t bit=UINT32_C(1)<<input;
        if(closed) emulator.sample.digital_closed|=bit;else emulator.sample.digital_closed&=~bit;
    } else {
        unsigned bank=(input-24)/16,pin=(input-24)%16;
        if(closed) emulator.sample.expansion_closed[bank]|=(uint16_t)(1U<<pin);
        else emulator.sample.expansion_closed[bank]&=(uint16_t)~(1U<<pin);
    }
}
static void turn(unsigned ordinal,int detents) {
    const osg_profile_encoder_t encoder=emulator.runtime.profile.encoders[ordinal];
    static const uint8_t positive[]={2,0,1,3},negative[]={1,0,2,3};
    const uint8_t *sequence=detents>0 ? positive : negative;
    contact(encoder.a,false);contact(encoder.b,false);advance(2);
    unsigned count=(unsigned)(detents>0 ? detents : -detents);
    for(unsigned i=0;i<count && !stopped;i++) for(unsigned j=0;j<4;j++) {
        contact(encoder.a,(sequence[j]&1)==0);contact(encoder.b,(sequence[j]&2)==0);
        advance(2);
    }
}

static void inspect(uint32_t id,bool ok,const char *error) {
    size_t n=osg_runtime_snapshot(&emulator.runtime,true,emulator.active_config,sizeof emulator.active_config);assert(n);
    fprintf(response_output,"{\"id\":%u,\"ok\":%s,\"error\":",id,ok?"true":"false");
    if(error)fprintf(response_output,"\"%s\"",error);else fprintf(response_output,"null");
    fprintf(response_output,",\"simulation\":{\"clockMs\":%u,\"moduleBoot\":%u,\"moduleReconnects\":%u,\"canFrames\":%u,\"acks\":%u,\"settingsErases\":%u,\"settingsPrograms\":%u,\"descriptorHex\":\"",
        emulator.now,emulator.module.boot,emulator.module_reconnects,emulator.can_frames,emulator.acks,emulator.settings_flash.erases,emulator.settings_flash.programs);
    hex(emulator.runtime.descriptor,emulator.runtime.descriptor_length);
    fprintf(response_output,"\",\"moduleReportHex\":\"");hex(emulator.report,emulator.runtime.layout.length);
    char action_state[OSG_ACTION_REPLY_CAPACITY];
    assert(osg_actions_command(&emulator.actions.registry,"action state",action_state,sizeof action_state));
    char event_state[OSG_EVENTS_REPLY_CAPACITY];
    assert(osg_events_command(&emulator.events,"events state",event_state,sizeof event_state));
    fprintf(response_output,"\",\"sourceNode\":%u,\"nodeCollisions\":%u,\"module\":%s,\"actions\":%s,\"actionResult\":%s,\"events\":%s,\"eventResult\":%s,\"displays\":[",
        osg_cyphal_node_id(&emulator.module.cyphal),emulator.node_collisions,emulator.active_config,
        action_state,emulator.action_result[0] ? emulator.action_result : "null",event_state,
        emulator.event_result[0] ? emulator.event_result : "null");
    for(unsigned i=0;i<emulator.display_count;i++) {
        native_display_t *d=&emulator.displays[i];const osg_device_display_port_t *port=&emulator.display_ports[i];
        fprintf(response_output,"%s{\"id\":",i ? "," : "");json_text(d->id);
        fprintf(response_output,",\"endpoint\":");json_text(d->endpoint);
        fprintf(response_output,",\"columns\":%u,\"rows\":%u,\"encoding\":\"%s\",\"text\":",
            port->display.columns,port->display.rows,port->display.encoding==OSG_ACTION_DISPLAY_ASCII ? "ascii" : "utf8");
        json_text(d->text);fprintf(response_output,",\"writes\":%u,\"delayMs\":%u}",d->writes,d->delay_ms);
    }
    fprintf(response_output,"]}}\n");fflush(response_output);
}
static void admin(const char *line) {
    unsigned id=0,n=0,a=0,b=0;int detents=0;bool ok=true;const char *error=NULL;
    if(sscanf(line,"sim %u %n",&id,&n)!=1 || !n) {inspect(id,false,"invalid_request");return;}
    const char *command=line+n;
    if(!strcmp(command,"inspect")) {}
    else if(sscanf(command,"can_node %u",&a)==1 && a>0 && a<=CANARD_NODE_ID_MAX) {
        assert(canard_set_node_id(&emulator.module.cyphal.canard,(uint8_t)a));osg_can_client_reset(&emulator.module);rearm();
    } else if(!strncmp(command,"direct_config ",14)) {
        bool apply=false;error=osg_runtime_command(&emulator.runtime,command+14,emulator.now,&apply);
        ok=error==NULL;if(apply)advance(400);
    } else if(!strncmp(command,"direct_action ",14)) {
        char action_command[OSG_ACTION_COMMAND_CAPACITY];
        int length=snprintf(action_command,sizeof action_command,"action %s",command+14);
        if(length<0 || (size_t)length>=sizeof action_command ||
           !osg_actions_command(&emulator.actions.registry,action_command,emulator.action_result,sizeof emulator.action_result)) {
            ok=false;error="invalid_request";
        }
    } else if(!strncmp(command,"direct_events ",14)) {
        char event_command[OSG_EVENTS_COMMAND_CAPACITY];
        int length=snprintf(event_command,sizeof event_command,"events %s",command+14);
        if(length<0 || (size_t)length>=sizeof event_command ||
           !osg_events_command(&emulator.events,event_command,emulator.event_result,sizeof emulator.event_result)) {
            ok=false;error="invalid_request";
        }
    } else if(!strcmp(command,"reboot")) {boot_module();advance(600);}
    else if(sscanf(command,"fault %u",&a)==1 && a<=1000)emulator.settings_flash.fault=(int)a;
    else if(sscanf(command,"advance %u",&a)==1 && a<=60000)advance(a);
    else if(sscanf(command,"axis %u %u",&a,&b)==2 && a<8 && b<=4095) {emulator.sample.axes[a]=(uint16_t)b;advance(30);}
    else if(sscanf(command,"axes_available %u",&a)==1 && a<=255) {emulator.axes_available=(uint8_t)a;advance(110);}
    else if(sscanf(command,"display_fail %u %u",&a,&b)==2 && a<emulator.display_count && b<=1) emulator.displays[a].write_fail=b!=0;
    else if(sscanf(command,"display_read_fail %u %u",&a,&b)==2 && a<emulator.display_count && b<=1) emulator.displays[a].read_fail=b!=0;
    else if(sscanf(command,"display_delay %u %u",&a,&b)==2 && a<emulator.display_count && b<=250) emulator.displays[a].delay_ms=b;
    else if(sscanf(command,"turn %u %d",&a,&detents)==2 && a<emulator.runtime.profile.encoder_count && detents && detents>=-128 && detents<=128) turn(a,detents);
    else if(sscanf(command,"expansion_available %u",&a)==1 && a<=255) {emulator.expansion_available=(uint8_t)a;advance(30);}
    else if(sscanf(command,"contact %u %u",&a,&b)==2 && a<OSG_INPUTS && b<=1) {
        contact((uint8_t)a,b!=0);
        advance(30);
    } else if(sscanf(command,"direct %u",&a)==1 && a<=1) {
        emulator.direct=a!=0;osg_can_client_usb(&emulator.module,emulator.direct);rearm();advance(600);
    } else {ok=false;error="invalid_request";}
    inspect(id,ok,error);
}
/* OSGB local CAN envelope: header[12], identity[16], CAN payload[64]. */
enum {BUS_PACKET_SIZE=92,BUS_HELLO=1,BUS_ACK=2,BUS_FRAME=3};
static struct {
    int bus,state_lock;
    char root[96],bus_manifest[160];
    struct sockaddr_in broker;
    pid_t broker_pid;
    bool joined,stdin_open,hello_sent;
    uint32_t hello_at,ack_at,gateway_epoch;
    struct timespec started;
    char input[OSG_RUNTIME_COMMAND_CAPACITY+128];
    osg_line_t input_line;
} local={.bus=-1,.state_lock=-1,.stdin_open=true};
static int exit_status;

static void die(const char *message) { fprintf(stderr,"%s: %s\n",message,strerror(errno)); exit(1); }

static void stop_signal(int signal_number) { (void)signal_number; stopped=1; }

static void owned_directory(const char *path) {
    struct stat info;
    if(mkdir(path,0700)<0 && errno!=EEXIST) die("Could not create local gateway directory");
    if(lstat(path,&info)<0) die("Could not inspect local gateway directory");
    if(!S_ISDIR(info.st_mode) || info.st_uid!=geteuid() || (info.st_mode&0777)!=0700) {
        errno=EPERM; die("Local gateway directories must be private and owned by the current user");
    }
}

static void make_state_directory(const char *path) {
    char scratch[1024]; size_t length=strlen(path);
    if(!length || length>=sizeof scratch) { errno=ENAMETOOLONG;die("Invalid state directory"); }
    memcpy(scratch,path,length+1);
    for(char *c=scratch+1;*c;c++) if(*c=='/') { *c=0; if(mkdir(scratch,0700)<0 && errno!=EEXIST) die("Could not create state parent"); *c='/'; }
    if(mkdir(scratch,0700)<0 && errno!=EEXIST) die("Could not create state directory");
    struct stat info;if(lstat(scratch,&info)<0 || !S_ISDIR(info.st_mode) || info.st_uid!=geteuid()) { errno=EPERM;die("State directory must belong to current user"); }
}

static void lock_state(void) {
    char path[1100];int n=snprintf(path,sizeof path,"%s/.lock",state_directory);
    if(n<0 || (size_t)n>=sizeof path) {errno=ENAMETOOLONG;die("State lock path too long");}
    local.state_lock=open(path,O_RDWR|O_CREAT|O_NOFOLLOW,0600);
    if(local.state_lock<0)die("Could not open flash state lock");
    struct stat info;
    if(fstat(local.state_lock,&info)<0 || !S_ISREG(info.st_mode) || info.st_uid!=geteuid() || (info.st_mode&0777)!=0600) {errno=EPERM;die("Flash state lock must be private and owned by current user");}
    if(flock(local.state_lock,LOCK_EX|LOCK_NB)<0)die("Flash state is already in use");
    if(fcntl(local.state_lock,F_SETFD,FD_CLOEXEC)<0)die("Could not configure flash state lock");
}

static void nonblocking(int fd) {
    int flags=fcntl(fd,F_GETFL); if(flags<0 || fcntl(fd,F_SETFL,flags|O_NONBLOCK)<0) die("Could not configure local socket");
    if(fcntl(fd,F_SETFD,FD_CLOEXEC)<0) die("Could not configure close-on-exec");
}

static uint32_t wall_ms(void) {
    struct timespec now;if(clock_gettime(CLOCK_MONOTONIC,&now)<0) die("Monotonic clock unavailable");
    int64_t elapsed=(int64_t)(now.tv_sec-local.started.tv_sec)*1000+(now.tv_nsec-local.started.tv_nsec)/1000000;
    return (uint32_t)elapsed;
}

static void put32(uint8_t *p,uint32_t value) { for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(value>>(8*i)); }

static uint32_t get32(const uint8_t *p) { return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }

static void packet_init(uint8_t packet[BUS_PACKET_SIZE],uint8_t kind) {
    memset(packet,0,BUS_PACKET_SIZE);memcpy(packet,"OSGB",4);packet[4]=1;packet[5]=kind;
}

static bool packet_valid(const uint8_t *packet,size_t length) {
    return length==BUS_PACKET_SIZE && !memcmp(packet,"OSGB",4) && packet[4]==1 &&
        packet[5]>=BUS_HELLO && packet[5]<=BUS_FRAME;
}

static bool send_packet(const struct sockaddr_in *address,const uint8_t packet[BUS_PACKET_SIZE]) {
    return sendto(local.bus,packet,BUS_PACKET_SIZE,MSG_DONTWAIT,(const struct sockaddr *)address,sizeof *address)==BUS_PACKET_SIZE;
}

static bool bus_send(const osg_gateway_frame_t *frame) {
    if(frame->deadline_valid && (int32_t)(emulator.now-frame->deadline_ms)>=0) return true;
    if(frame->length>OSG_GW_FRAME_CAPACITY) return false;
    uint8_t packet[BUS_PACKET_SIZE];packet_init(packet,BUS_FRAME);
    packet[6]=(uint8_t)((frame->extended?1:0)|(frame->fd?2:0)|(frame->brs?4:0));
    packet[7]=frame->length;put32(packet+8,frame->id);memcpy(packet+28,frame->data,frame->length);
    if(!local.joined) return false;
    if(send_packet(&local.broker,packet)) return true;
    if(errno==ENOENT || errno==ECONNREFUSED) {local.joined=false;osg_can_client_reset(&emulator.module);}
    return false;
}

static bool discover_bus(void) {
    int fd=open(local.bus_manifest,O_RDONLY|O_NOFOLLOW);
    if(fd<0)return false;
    struct stat info;
    if(fstat(fd,&info)<0 || !S_ISREG(info.st_mode) || info.st_uid!=geteuid() || info.st_size>256) {close(fd);return false;}
    char buffer[257];ssize_t n=read(fd,buffer,sizeof buffer-1);close(fd);if(n<=0)return false;buffer[n]=0;
    unsigned long pid=0;unsigned port=0;int used=0;
    if(sscanf(buffer,"{\"protocol\":1,\"pid\":%lu,\"port\":%u}%n",&pid,&port,&used)!=2 ||
       pid==0 || pid>INT_MAX || port==0 || port>65535 || (buffer[used] && strcmp(buffer+used,"\n")) || kill((pid_t)pid,0)<0)return false;
    if(local.broker_pid!=(pid_t)pid || local.broker.sin_port!=htons((uint16_t)port)) {
        local.joined=false;osg_can_client_reset(&emulator.module);rearm();
    }
    local.broker=(struct sockaddr_in){.sin_family=AF_INET,.sin_port=htons((uint16_t)port),.sin_addr={.s_addr=htonl(INADDR_LOOPBACK)}};
    local.broker_pid=(pid_t)pid;return true;
}

static void hello(void) {
    local.hello_sent=true;local.hello_at=emulator.now;
    if(!discover_bus()) {
        if(local.joined) {local.joined=false;osg_can_client_reset(&emulator.module);rearm();}
        return;
    }
    uint8_t packet[BUS_PACKET_SIZE];packet_init(packet,BUS_HELLO);
    memcpy(packet+12,emulator.module_identity.bytes,OSG_GW_IDENTITY_SIZE);put32(packet+28,(uint32_t)getpid());
    (void)send_packet(&local.broker,packet);
}

static void device_packet(const struct sockaddr_in *address,const uint8_t *packet) {
    if(address->sin_addr.s_addr!=local.broker.sin_addr.s_addr || address->sin_port!=local.broker.sin_port)return;
    if(packet[5]==BUS_ACK) {
        if(packet[28]) {
            fprintf(stderr,"Device could not join gateway: %s\n",packet[28]==1?"identity already active":"bus is full");
            stopped=1;exit_status=1;return;
        }
        uint32_t epoch=get32(packet+8);
        if(!local.joined || local.gateway_epoch!=epoch) {osg_can_client_reset(&emulator.module);rearm();}
        local.joined=true;local.gateway_epoch=epoch;local.ack_at=emulator.now;
    } else if(packet[5]==BUS_FRAME && local.joined && packet[7]<=OSG_GW_FRAME_CAPACITY && !(packet[6]&~7U)) {
        osg_gateway_frame_t frame={.id=get32(packet+8),.length=packet[7],.extended=(packet[6]&1)!=0,
            .fd=(packet[6]&2)!=0,.brs=(packet[6]&4)!=0};memcpy(frame.data,packet+28,frame.length);
        receive_module(&frame);
    }
}

static void receive_bus(void) {
    for(unsigned i=0;i<512;i++) {
        uint8_t packet[BUS_PACKET_SIZE+1];struct sockaddr_in address;memset(&address,0,sizeof address);
        socklen_t address_length=sizeof address;
        ssize_t n=recvfrom(local.bus,packet,sizeof packet,MSG_DONTWAIT,(struct sockaddr *)&address,&address_length);
        if(n<0) {if(errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) die("Local CAN receive failed");break;}
        if(!packet_valid(packet,(size_t)n)) continue;
        device_packet(&address,packet);
    }
}

static void cleanup(void) {
    if(local.bus>=0)close(local.bus);
    if(local.state_lock>=0)close(local.state_lock);
}

static void standalone_cycle(bool wait) {
    if(wait) {struct pollfd p={.fd=local.bus,.events=POLLIN};(void)poll(&p,1,1);}
    emulator.now=wall_ms();receive_bus();
    {
        if(local.joined && (uint32_t)(emulator.now-local.ack_at)>2000U) {local.joined=false;osg_can_client_reset(&emulator.module);rearm();}
        if(!local.hello_sent || (uint32_t)(emulator.now-local.hello_at)>=500U)hello();
    }
    tick();
}

static void serve_stdin(void) {
    if(!local.stdin_open)return;
    for(unsigned i=0;i<1024;i++) {
        struct pollfd input={.fd=STDIN_FILENO,.events=POLLIN};
        if(poll(&input,1,0)<=0)return;
        char byte;ssize_t n=read(STDIN_FILENO,&byte,1);
        if(n==0) {local.stdin_open=false;return;}
        if(n<0) {if(errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR)local.stdin_open=false;return;}
        osg_line_result_t result=osg_line_feed(&local.input_line,byte);
        if(result==OSG_LINE_READY) {
            if(!strncmp(local.input,"sim ",4))admin(local.input);
            else fprintf(stderr,"Use sim <id> <operation> for terminal administration.\n");
            return;
        }
        if(result==OSG_LINE_TOO_LONG) {inspect(0,false,"invalid_request");return;}
    }
}

static void init_local(void) {
    char parent[80];snprintf(parent,sizeof parent,"/tmp/opensimgear-%lu",(unsigned long)geteuid());owned_directory(parent);
    if(simulation_namespace) snprintf(local.root,sizeof local.root,"%s/gateways-%s",parent,simulation_namespace);
    else snprintf(local.root,sizeof local.root,"%s/gateways",parent);
    owned_directory(local.root);
    snprintf(local.bus_manifest,sizeof local.bus_manifest,"%s/%s.can.json",local.root,gateway_uid);
    local.bus=socket(AF_INET,SOCK_DGRAM,0);if(local.bus<0)die("Could not create local CAN endpoint");
    local.broker=(struct sockaddr_in){.sin_family=AF_INET,.sin_addr={.s_addr=htonl(INADDR_LOOPBACK)}};
    if(bind(local.bus,(struct sockaddr *)&local.broker,sizeof local.broker)<0)die("Could not bind local CAN endpoint");
    nonblocking(local.bus);
    if(clock_gettime(CLOCK_MONOTONIC,&local.started)<0)die("Monotonic clock unavailable");
    osg_line_init(&local.input_line,local.input,sizeof local.input);atexit(cleanup);
    struct sigaction action={0};action.sa_handler=stop_signal;sigemptyset(&action.sa_mask);
    if(sigaction(SIGINT,&action,NULL)<0 || sigaction(SIGTERM,&action,NULL)<0)die("Could not install shutdown handler");
    signal(SIGPIPE,SIG_IGN);
}

static bool uid_valid(const char *uid) {
    if(strlen(uid)!=24)return false;
    bool nonzero=false,nonff=false;
    for(unsigned i=0;i<24;i++) {char c=uid[i];if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;nonzero|=c!='0';nonff|=c!='f';}
    return nonzero&&nonff;
}

static bool namespace_valid(const char *name) {
    if(!name || !*name) return false;
    size_t n=0;
    for(;name[n];n++) {
        char c=name[n];
        if(n>=31 || !((c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9') ||
                       c=='.' || c=='_' || c=='-')) return false;
    }
    return true;
}

static void usage(const char *program) {
    fprintf(stderr,"Usage: %s --definition-fd FD --gateway UID24 --state-dir PATH [--validate]\nStart JSON devices with simulation/run.py --config FILE.\n",program);exit(2);
}
static void load_definition(int fd) {
    uint8_t payload[8+16+OSG_RUNTIME_RECORD_SIZE+39+OSG_DEVICE_DISPLAY_MAX*132];size_t used=0;
    while(used<sizeof payload) {
        ssize_t n=read(fd,payload+used,sizeof payload-used);
        if(n<=0) {fprintf(stderr,"Incomplete device startup definition\n");exit(2);}used+=(size_t)n;
    }
    uint8_t extra;if(read(fd,&extra,1)!=0 || memcmp(payload,"OSGSIM03",8)) {fprintf(stderr,"Invalid device startup definition\n");exit(2);}close(fd);
    memcpy(emulator.module_identity.bytes,payload+8,16);uint32_t generation;
    if(!osg_gateway_identity_valid(&emulator.module_identity) ||
       !osg_runtime_decode(&emulator.defaults,&generation,payload+24,OSG_RUNTIME_RECORD_SIZE) || generation!=1) {
        fprintf(stderr,"Invalid device profile: production firmware rejected its mappings, name, counts or axis options\n");exit(2);
    }
    const uint8_t *p=payload+24+OSG_RUNTIME_RECORD_SIZE;
    emulator.sample.digital_closed=get32(p);p+=4;
    for(unsigned i=0;i<8;i++,p+=2)emulator.sample.axes[i]=(uint16_t)(p[0]|(uint16_t)p[1]<<8);
    emulator.expansion_available=*p++;emulator.sample.expansion_available=emulator.expansion_available;
    for(unsigned i=0;i<8;i++,p+=2)emulator.sample.expansion_closed[i]=(uint16_t)(p[0]|(uint16_t)p[1]<<8);
    if(*p>1) {fprintf(stderr,"Invalid device output catalogue\n");exit(2);}
    emulator.board_actions=*p++!=0;
    if(*p>OSG_DEVICE_DISPLAY_MAX) {fprintf(stderr,"Too many device display ports\n");exit(2);}
    emulator.display_count=*p++;emulator.axes_available=255;
    for(unsigned i=0;i<OSG_DEVICE_DISPLAY_MAX;i++,p+=132) {
        if(i>=emulator.display_count) continue;
        native_display_t *d=&emulator.displays[i];
        memcpy(d->id,p,32);memcpy(d->label,p+32,64);memcpy(d->endpoint,p+96,32);
        if(!memchr(d->id,0,sizeof d->id) || !memchr(d->label,0,sizeof d->label) || !memchr(d->endpoint,0,sizeof d->endpoint) || p[130]>1) {
            fprintf(stderr,"Invalid device display port\n");exit(2);
        }
        emulator.display_ports[i]=(osg_device_display_port_t){.id=d->id,.label=d->label,.endpoint=d->endpoint,
            .display={.columns=p[128],.rows=p[129],.encoding=(osg_action_display_encoding_t)p[130]},
            .max_bytes=p[131],.write=display_write,.read=display_read,.context=d};
    }
    osg_device_actions_t validation;
    if(!osg_device_actions_init_outputs(&validation,!emulator.board_actions,emulator.display_ports,emulator.display_count)) {
        fprintf(stderr,"Invalid device output catalogue\n");exit(2);
    }
}
int main(int argc,char **argv) {
    int definition_fd=-1;bool validate=false;response_output=stdout;
    simulation_namespace=getenv("OSG_SIMULATION_NAMESPACE");
    if(simulation_namespace && !namespace_valid(simulation_namespace)) {
        fprintf(stderr,"OSG_SIMULATION_NAMESPACE must contain 1..31 ASCII letters, digits, '.', '_' or '-'\n");return 2;
    }
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"--validate")) {validate=true;continue;}
        if(i+1>=argc)usage(argv[0]);
        if(!strcmp(argv[i],"--state-dir"))state_directory=argv[++i];
        else if(!strcmp(argv[i],"--gateway")) {if(!uid_valid(argv[i+1]))usage(argv[0]);strcpy(gateway_uid,argv[++i]);}
        else if(!strcmp(argv[i],"--definition-fd")) {
            char *end=NULL;long number=strtol(argv[++i],&end,10);if(!end || *end || number<3 || number>INT_MAX)usage(argv[0]);definition_fd=(int)number;
        } else usage(argv[0]);
    }
    if(definition_fd<0 || (!validate && !state_directory))usage(argv[0]);
    load_definition(definition_fd);
    if(validate) {puts("Device JSON and production profile are valid");return 0;}
    make_state_directory(state_directory);lock_state();
    flash_init(&emulator.settings_flash,"settings");flash_init(&emulator.boot_flash,"boot");
    boot_module();init_local();
    char identity[33];osg_gateway_identity_format(&emulator.module_identity,identity);
    fprintf(stderr,"OpenSimGear device %s ready\n",identity);
    while(!stopped) {standalone_cycle(true);serve_stdin();}
    osg_cyphal_destroy(&emulator.module.cyphal);return exit_status;
}
