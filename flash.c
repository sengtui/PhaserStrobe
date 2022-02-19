#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "hardware/flash.h"
#include "parameters.h"


// Setup the flash memory for parameter starting from 1M (totally 2M flash)
const uint8_t *flash_target_contents = (const uint8_t*)(XIP_BASE + FLASH_TARGET_OFFSET);

extern Parameters_t ParamMem;

// 將 Param 存到 Flash 中，位置從1024K 處開始的 256位元（暫定）
bool writeParam(Parameters_t *Param)
{
    uint8_t * P;
    P=(uint8_t *) Param;
    // 暫停並且儲存 IRQ
    uint32_t ints = save_and_disable_interrupts();
    
    // 刪除 Flash rom 裡頭的資料，從 FLASH_TARGET_OFFSET (1024K) 處開始， FLASH_PAGE_SIZE 是 256bytes
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_PAGE_SIZE);
    
    // 把 Param[] 寫入 flash rom 裡頭
    flash_range_program(FLASH_TARGET_OFFSET, P, FLASH_PAGE_SIZE);
    
    // 恢復並且將儲存的 IRQ 拿出來執行
    restore_interrupts(ints);
    
    // 檢查 Flash 寫入值和讀回值是否相同。
    int mismatch = 0;
    for(int i=0; i< FLASH_PAGE_SIZE; i++) {
        if(P[i] != flash_target_contents[i]) mismatch++;
    }
    if(mismatch !=0)
    {
        printf("Write to flash rom check error: %d mismatches\n", mismatch);
        return false;
    }
    return true;
}

/* 測試 flash 中的特定參數值決定是否這是第一次安裝程式， 然後把預設值寫進去 flash
 建議把預設值都在這邊填上，這樣子以後裝新的 ＰＩＣＯ時只要第一次跑就會把預設值放進 flash 裡頭。
*/
bool initFlash(Parameters_t *Param)
{
    uint8_t *P=(uint8_t*)Param;
    int mismatch=0;
    int i;
    for(i=0;i<5;i++)
    {
        if(flash_target_contents[40+i]!=0x7f) mismatch++;
    }
    if(mismatch!=0)
    {
        printf("Flash seems not updated before, write with default value\n");
        Param->rtu_id=11;
        Param->writeFlash =0;
        Param->camaraPos1 = 20;
        Param->camaraPos2 = 532;
        Param->camaraPos3 = 1044;
        Param->camaraPos4 = 1556;
        Param->encoderResolution = 2048;
        Param->LED_duration_us =100;
        Param->LED_warmup_us = 5;
        Param->Camara_delay_us = 10;
        for(i=0;i<5;i++) P[40+i]=0x7f;
        writeParam(Param);
    }
    // 把 flash rom 裡頭的儲存資料讀回來 Param[]
    for(i=0;i<256;i++) P[i] = *(flash_target_contents+i);
}