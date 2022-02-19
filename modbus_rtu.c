#include "parameters.h"
#include "modbus_rtu.h"
#include "hardware/irq.h"

extern Parameters_t ParamMem;

void printHEX(char* header, uint8_t *buf, int len)
{
    printf("%s [ ", header);
    for(int i=0;i<len; i++) printf("0X%02X ", buf[i]);
    printf("] %d Bytes\n", len);
}

// 送進來的字串（必須要包含CRC), 處理過後和字串尾端的 CRC 比較是否相符。
bool checkCRC(uint8_t *buffer, uint16_t len)
{
    uint8_t crc_hi = 0xFF; /* high CRC byte initialized */
    uint8_t crc_lo = 0xFF; /* low CRC byte initialized */
    uint16_t buffer_length;

    // 字串長度減去 CRC 字元長度2
    buffer_length=len-2;

    unsigned int i; /* will index into CRC lookup */

    /* pass through message buffer */
    while (buffer_length--) {
        i = crc_hi ^ *buffer++; /* calculate the CRC  */
        crc_hi = crc_lo ^ table_crc_hi[i];
        crc_lo = table_crc_lo[i];
    }
    
    if((*buffer++== crc_hi)){
        if(*buffer== crc_lo) return 1;
    }
    return 0;
}

// 送進來的字串（必須要含 CRC 位置）處理過後把 CRC 填入位置。
bool updateCRC(uint8_t *buffer, uint16_t len)
{
    uint8_t crc_hi = 0xFF; /* high CRC byte initialized */
    uint8_t crc_lo = 0xFF; /* low CRC byte initialized */
    uint16_t buffer_length;

    //字串太短，怎麼樣都要四個字呀！！
    if(len < 4) return false;

    // 字串長度減去 CRC 字元長度2
    buffer_length=len-2;

    unsigned int i; /* will index into CRC lookup */

    /* pass through message buffer */
    while (buffer_length--) {
        i = crc_hi ^ *buffer++; /* calculate the CRC  */
        crc_hi = crc_lo ^ table_crc_hi[i];
        crc_lo = table_crc_lo[i];
    }
    
    *buffer= crc_hi;
    buffer++;
    *buffer= crc_lo;
    return true;
}

 
int rtu_execCmd(uart_inst_t *uart, uint8_t *buf, int len){
    uint8_t cmd = buf[1];
    uint16_t addr = buf[2]<<8 | buf[3];
    uint16_t range = buf[4]<<8 | buf[5];
    uint8_t bytesMore= buf[6];
    uint8_t *Param_u8 = (uint8_t*)&ParamMem;
    uint16_t *Param_u16 = (uint16_t*)&ParamMem;

    int ret;

    if(!checkCRC(buf, len)) printf("CRC Error\n");

    if(addr>256)
    {
        printf("addr error %d > 256\n", addr);
        return(-1);
    }
    
    if(len<6)
    {
        printf("length error %d\n", len);
        return(-1);
    }

    switch(cmd)
    {
        case 0x03:  // Read registers
            printf("Reading registers 0X%04X, length 0X%04X...\n", addr, range);
            buf[1]= 0x03;
            bytesMore=range+range;
            buf[2]= bytesMore;
            for(int i=0;i<bytesMore;i++)
            {
                buf[3+i]= Param_u8[addr+addr+i]; 
            }
            ret = bytesMore+5;
            updateCRC(buf, ret);
            break;
        case 0x05: // Write coil (for flash write)
            printf("Writes a coil, only support addr 0x003F->0XFF00 (write flash)\n");
        case 0x06: // Write single register
            printf("Writes register 0X%04X, value:0X%04X", addr, range);
            Param_u16[addr]=range;
            ret = 8;
            updateCRC(buf, ret);
            break;
        case 0x10:  // Write registers;
            printf("Writes multiple registers 0X%04X, len: 0X$04X", addr, range);
            if(len<9+bytesMore)
            {
                printf("writing length mistake, command length=%d while range=%d\n", len, range);
                return(-1);
            }

            memcpy(Param_u8+addr+addr, (uint8_t*)buf+7, bytesMore);
//            for(int i=0; i< bytesMore;i++)
//            {
//                memcpy(Param_u8+addr+addr, buf[7], bytesMore);
//                Param_u8[addr+addr+i]= buf[7+i];
//            }
            ret = 8;
            updateCRC(buf,ret);
            break;
        //    return rtu_writeRegister(addr, range, buf+6);
        default:  // Do nothing.
            printf("Illegal function or function not supported\n");
            ret = 0;
            break;
    }
    if(ret>0)
    {
        sleep_ms(10);
        printHEX("Sending reply: ", buf, ret);
        gpio_put(UART_DIR_PIN, true);
        sleep_ms(1);
        for(int i=0;i<ret;i++) {
            uart_putc_raw(uart, buf[i]);
        }
        gpio_put(UART_DIR_PIN, false);
        return ret;
    }
}
int rtu_broadcastCmd(uart_inst_t *uart, uint8_t *buf, int len, Parameters_t *Param){

    uint8_t cmd = buf[1];
    uint16_t addr = buf[2]<<8 | buf[3];
    uint16_t range = buf[4]<<8 | buf[5];
    uint8_t *Param_u8 = (uint8_t*)Param;
    int pass;

    pass = false;
    switch(cmd){
        case 0x05:  // write register
        case 0x06:
                if(cmd==0 && !Param->setup) break;
            pass = true;
            break;
 //           return rtu_readRegisters(buf, len);
        case 0x10:  // Write single register;
        case 0x03:
        default:  // Do nothing.
            printf("Illegal function or function not supported\n");
            break;
    }
    if(pass) return(rtu_execCmd(uart, buf, len));
}
