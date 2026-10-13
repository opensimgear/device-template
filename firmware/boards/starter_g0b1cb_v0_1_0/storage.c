#include "board.h"
#include "gateway_journal.h"
#include "boot_session.h"
#include "stm32g0xx_hal.h"
#include <string.h>

extern uint8_t __boot_page0[],__boot_page1[];
extern uint8_t __settings_page0[],__settings_page1[];
static unsigned settings_context;
_Static_assert(BOARD_BOOT_CAPACITY==OSG_JOURNAL_CAPACITY,"Journal payload capacity");
static const uint8_t *page(void *context,unsigned index) {
    if(index>1) return NULL;
    if(context) return index ? __settings_page1 : __settings_page0;
    return index ? __boot_page1 : __boot_page0;
}
static bool page_address(void *context,unsigned index,uint32_t *address) {
    if(index>1 || FLASH_SIZE!=128U*1024U) return false;
    *address=(uint32_t)(uintptr_t)page(context,index);
    uint32_t first=context?0x0801E000U:0x0801F000U;
    return *address==first+index*OSG_JOURNAL_PAGE_SIZE;
}
static bool erase_page(void *context,unsigned index) {
    uint32_t address;
    if(!page_address(context,index,&address)) return false;
    if(context && HAL_FLASH_Unlock()!=HAL_OK) return false;
    __HAL_FLASH_CLEAR_FLAG(FLASH_SR_CLEAR);
    /* Page indexes are relative to the active bank configuration, including
     * dual-bank devices. Never derive the final storage pages from a larger
     * STM32G0B1 device's family-wide maximum capacity. */
    FLASH_EraseInitTypeDef erase={.TypeErase=FLASH_TYPEERASE_PAGES,.Banks=FLASH_BANK_1,
        .Page=(address-FLASH_BASE)%FLASH_BANK_SIZE/FLASH_PAGE_SIZE,.NbPages=1};
#if defined(FLASH_DBANK_SUPPORT)
    if(FLASH_BANK_NB==2 && address-FLASH_BASE>=FLASH_BANK_SIZE) erase.Banks=FLASH_BANK_2;
#endif
    uint32_t failed_page=0;bool ok=HAL_FLASHEx_Erase(&erase,&failed_page)==HAL_OK;
    if(context && HAL_FLASH_Lock()!=HAL_OK) ok=false;
    return ok;
}
static bool program(void *context,unsigned index,unsigned offset,const uint8_t data[8]) {
    uint32_t address;
    if(!page_address(context,index,&address) || offset%8 || offset>OSG_JOURNAL_PAGE_SIZE-8) return false;
    if(context && HAL_FLASH_Unlock()!=HAL_OK) return false;
    __HAL_FLASH_CLEAR_FLAG(FLASH_SR_CLEAR);
    uint64_t value; memcpy(&value,data,sizeof value);
    bool ok=HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,address+offset,value)==HAL_OK;
    if(context && HAL_FLASH_Lock()!=HAL_OK) ok=false;
    return ok;
}
static const osg_journal_io_t io={.read=page,.erase=erase_page,.program=program};
static const osg_journal_io_t settings_io={.context=&settings_context,.read=page,.erase=erase_page,.program=program};
const osg_journal_io_t *board_settings_io(void) { return &settings_io; }
uint32_t board_boot_session(void) {
    uint32_t current=0;
    if(FLASH_SIZE!=128U*1024U || HAL_FLASH_Unlock()!=HAL_OK) board_fault();
    __HAL_FLASH_CLEAR_FLAG(FLASH_SR_CLEAR);
    bool ok=osg_boot_session_begin(&io,&current); (void)HAL_FLASH_Lock();
    if(!ok) board_fault();
    return current;
}
