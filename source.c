#include "source.h"
#include <math.h>


// 定義 Parameters 在 Global veriable 方便其他 function 取用。
Parameters_t ParamMem;

// 給 uart IRQ 用的讀取 Buffer
uint8_t rx_buf[256];
int rx_index, rx_watchdog;

//給 encoder IRQ (CORE1) 用的全區變數
bool b0, dir;   //Ｂ相電位，方向
int encoder;    //econder 目前計數
bool hit;       //IRQ 程序發現目標值達到， set 這個線圈
int target;     //下次觸發目標值
int32_t spd_c, spd_c0;    //不更新總計數，算速度用的

// ENCODER 的 IRQ 在 core1 跑，避免和 uart irq 相撞
void on_encoder_irq(uint gpio, uint32_t events)
{
/* A計數器, B方向, Z 歸零
    if(gpio==GPIO_ENCODER_A) gpio_get(GPIO_ENCODER_B)?encoder++:encoder--;
    else if(gpio==GPIO_ENCODER_Z && events==GPIO_IRQ_EDGE_RISE) encoder=0;
    if(target==encoder) hit=true;
*/
 /* AB 相位計數器*/
    b0 = gpio_get(GPIO_ENCODER_B);
    if(gpio==GPIO_ENCODER_A)
    {
        if(events==GPIO_IRQ_EDGE_RISE) dir = !b0;
        if(events==GPIO_IRQ_EDGE_FALL) dir = b0;  
        dir? encoder++:encoder--;
        dir? spd_c++:spd_c--;
        if(encoder<0) encoder+=ParamMem.encoderResolution;
        if(target==encoder) hit=true;
    }
    else if(gpio==GPIO_ENCODER_Z)
    {
        if((events==GPIO_IRQ_EDGE_FALL) ^ dir)
        {
            encoder=0;
            if(target==encoder) hit = true;
        }
    }
/**/
}

/*
 這個是 core1 的 main(), core 1被拿來專門跑 encoder irq，
 主程式 busy waiting 觸發線圈 hit, 發現觸發後進行觸發程序（開燈，等燈亮度達到後觸發快門，曝光時間後關燈關快門。）
 拍完照片後需要冷卻 50mS (AOI 處理能力上限 20 frames/s) 趁這個時間來量 encoder 速度。
*/
void core1_entry()
{
    uint16_t *Param_u16 = (uint16_t*) &ParamMem;

 //   int encoder0;
    int pos=2;
    float diff;

    gpio_set_irq_enabled(GPIO_ENCODER_Z, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled_with_callback(GPIO_ENCODER_A, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, on_encoder_irq);
    dir = true;
    encoder=0;

    // 怕初始值太大會燒燈，限制不能超過 0.1+10ms
    if(ParamMem.LED_warmup_us > 100) ParamMem.LED_warmup_us=5;
    if(ParamMem.LED_duration_us > 10000) ParamMem.LED_duration_us=1000;

    // 剛開始還沒碰到 Z 相歸零，前面幾張照片不拍。
    for(int i=0;i<2;i++) while(!hit) {};

    // BUSY WAITING
    while(1)
    {
        // IRQ 有比對到要拍照的位置
        if(hit)
        {
            //開 LED, 等 LED 電壓 settle
            gpio_put(GPIO_LED, true);
            busy_wait_us(ParamMem.LED_warmup_us);
            //送Trigger 給照相機, 同時紀錄目前 encoder 數
            gpio_put(GPIO_CAMARA, true);
            //等待曝光時間過去後關燈關相機
            busy_wait_us(ParamMem.LED_duration_us);

            gpio_put(GPIO_LED, false);
            gpio_put(GPIO_CAMARA, false);

            //設定下次拍照的位置
            target = Param_u16[pos++] - ParamMem.encoderDiff;
            if(target<0) target=target+ParamMem.encoderResolution;
            if(pos>5)pos=2;
            
            //把剩下賢者時間睡完，因為相機最快只能每秒拍20張，
            sleep_ms(49);
            //賢者時間結束，把trigger重設讓 IRQ 下次到達目標位址的時候再觸發。 
            hit=false;

        }
    }
}

void on_uart_rx(void)
{
    // 把 FIFO 裡頭的資料讀光
    while(uart_is_readable(UART_ID)) rx_buf[rx_index++]=uart_getc(UART_ID);
    // 把看門狗更新，代表有熱騰騰的資料正在進來。
    rx_watchdog=0;
}

// 每 100ms 定時呼叫一次
bool on_repeat_timer(struct repeating_timer *timer)
{
    float f;
    // 儲存碼表
    spd_c0=spd_c;
    // 歸零碼表, 多幾次避免 concurrent conflict
    spd_c=0; spd_c=0;
    // 乘 10 除 encoder 解析度，變成 RPM
    f = (float)spd_c0 * 10.0 / (float)ParamMem.encoderResolution;
    // 乘 10, 變成 RPM (單位 0.1M)
    ParamMem.RPM = (int16_t) (f*10);
    // 乘周長 x10 變成 MPM (單位 0.1M)
    ParamMem.MPM = (int16_t) (f*3.1416);

    // 100mS 跑了 spc_c0, 那麼從觸發到照相機真的拍照總共跑了幾個 pulse 呢？
    ParamMem.encoderDiff = spd_c0*(ParamMem.Camara_delay_us+ParamMem.LED_warmup_us) / 100000;


//printf("%0.3f RPM, %0.3f MPM, encoderDiff %d \n", f, f*3.1416, ParamMem.encoderDiff);    
    return true;
}

bool init()
{
    stdio_init_all();

    // 設定 Encoder 相關資訊
    rx_index=0;
    rx_watchdog=0;
    memset(rx_buf, 0, 256);
    gpio_init(GPIO_LED);
    gpio_set_dir(GPIO_LED, GPIO_OUT);
    gpio_init(GPIO_CAMARA);
    gpio_set_dir(GPIO_CAMARA, GPIO_OUT);
    gpio_init(GPIO_RESET);
    gpio_set_dir(GPIO_RESET, GPIO_IN);
    gpio_init(GPIO_ENCODER_B);
    gpio_set_dir(GPIO_ENCODER_A, GPIO_IN);
    gpio_set_dir(GPIO_ENCODER_B, GPIO_IN);
    gpio_set_dir(GPIO_ENCODER_Z, GPIO_IN);

    //設定 UART, 半雙工還要手動定方向
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, false);
    irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);

    gpio_init(UART_DIR_PIN);
    gpio_set_dir(UART_DIR_PIN, GPIO_OUT);
    gpio_set_pulls(UART_DIR_PIN, false, true);

    //檢查 Flash 是否有資訊，沒有的話設定預設值然後寫到 flash 裡頭。
    initFlash(&ParamMem);

    return true;
}

//這邊是 core0, 專門跑 communication 的，如 modbus_rtu 和 i2c
int main()
{
    
    int i=0;
    char ch;
    int len;
    uint8_t buf[256];
   
    init();
 
    multicore_launch_core1(core1_entry);

// 100mS Timer, 用來計算速度和因為速度造成的補償值
    struct repeating_timer timer;
    spd_c=0;
    add_repeating_timer_ms(-100, on_repeat_timer, NULL, &timer);


    while(1)
    {
        // 規範的停止時間是 3.5字元，用 19200bps 算應該是約 2mS, 等到 4mS 還沒有rx IRQ 就確定字串結束（或是根本還沒開始）
        //  如果 watchdog > 10 (10x0.4 = 4mS), 代表很久沒有 irq, 有兩個可能： 1. 沒通訊， 2. 讀取完成。
        if(rx_watchdog++>10)
        {
            len = rx_index;
            rx_index=0;
            // 有讀到東西， copy 到處理暫存器去。
            if(len>0) printHEX("Received ", rx_buf, len);
            for(int x=0;x<len;x++) buf[x]=rx_buf[x];
            // 沒通訊，重新算 watchdog
            rx_watchdog=0;
            for(int x=0;x<len;x++) buf[x]=rx_buf[x];
//        printf("Encoder: %d, Target: %d (%d, %d, %d, %d) \n", \
//        encoder, target, Param_u16[2],Param_u16[3],Param_u16[4],Param_u16[5] );

        }
        // 執行 RTU 命令
        if(len>0)
        {
            if(buf[0]==(uint8_t)ParamMem.rtu_id)
            {
                rtu_execCmd(UART_ID, buf, len);
            }
            else
            {
                printf("ID mis-matched [0X%02X] expected [0X%02X]\n", buf[0], ParamMem.rtu_id);
                // 如果是 Broadcast 
                if(buf[0]==00)
                {
                    printf("Broadcasting\n");
                    ParamMem.setup = (uint16_t) gpio_get(RTU_SETUP_PIN);
                    rtu_broadcastCmd( UART_ID, buf, len, &ParamMem);
                }
            }
            len = 0;
        }
        
        if(ParamMem.writeFlash==0xff00)
        {
            ParamMem.writeFlash=0;
            printf("Received demand to write flash, [%d]\n", writeParam(&ParamMem));
        }
        sleep_us(400);
    }

}