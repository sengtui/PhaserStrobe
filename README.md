//閃頻輸出程式
/*
rtu uid = 11
支援 registers write / read 和 coil write

encoderResolution 應該是真實 resolution 的兩倍（因為 upper edge & lower edge 各觸發一次），使用 A/B/Z 增量編碼器

GPIO 和 參數矩陣的設定在 parameters.h

參數可以透過 modbus rtu 設定後， 將 writeFlash(0x01) 以coil 設定（寫入值：0xff00) 就可以寫入 flash 變成預設值。
為了方便簡陋的自建 modbus rtu server程式，所有參數都是 uint16, 不管什麼東東都是。 coil / holding registers / registers 都是從零開始同一個位置沒有在分的。
*/

ß
#define UART_ID uart0
#define UART_TX_PIN 0       // PIN1
#define UART_RX_PIN 1       // PIN2
#define UART_DIR_PIN 2      // PIN4
#define BAUD_RATE 9600
#define DATA_BITS 8
#define PARITY UART_PARITY_NONE
#define STOP_BITS 1

#define RTU_SETUP_PIN 3
#define FLASH_TARGET_OFFSET (1024*1024)

#define GPIO_ENCODER_A 10   // PIN14
#define GPIO_ENCODER_B 11   // PIN15
#define GPIO_ENCODER_Z 12   // PIN16
#define GPIO_LED 25         // 內部燈
#define GPIO_CAMARA 14      // PIN19
#define GPIO_RESET 15       // PIN20
typedef struct  
{
    uint16_t rtu_id;                 //0
    uint16_t writeFlash;             //1
    uint16_t camaraPos1;             //2
    uint16_t camaraPos2;             //3
    uint16_t camaraPos3;             //4
    uint16_t camaraPos4;             //5
    uint16_t encoderResolution;     //6
    uint16_t WheelLen;              //7
    uint16_t MPM;                   //8
    uint16_t ledDuration;           //9
    uint16_t setup;                 //a
    uint16_t encoderDiff;           //b
    uint16_t LED_warmup_us;         //c
    uint16_t LED_duration_us;       //d
    uint16_t Camara_delay_us;
    uint16_t RPM;                   //e
    uint16_t AQ0;                   //0xf
    uint16_t AQ1;

    uint16_t garbage[128];
} Parameters_t;

bool writeParam(Parameters_t* Param);
bool initFlash(Parameters_t * Param);