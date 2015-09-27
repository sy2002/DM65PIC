/*
 *  FIRMWARE FOR DM65PIC (Dieter's MEGA65 Peripheral Interface Controller)
 *
 *  Keyboard, Joystick and Expansion Port interface for the MEGA65
 *  running on a Nexys 4 DDR FPGA
 * 
 *  done in September 2015 by sy2002 (http://www.sy2002.de)
 *  using Tilen Majerle's STM32F4 libraries (http://stm32f4-discovery.com)
 */

/* Include core modules */
#include "stm32f4xx.h"

/* Include STM32F4 libraries */
#include "defines.h"
#include "tm_stm32f4_swo.h"
#include "tm_stm32f4_gpio.h"
#include "tm_stm32f4_delay.h"

int main(void)
{
    /* complete state of the keyboard split into nibbles according to the FPGA transfer protocol */
    unsigned char nibbles[32] =
    {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00        
    };
    
    
    /* map the pins of the DM65PIC to the columns and rows of the C65 keyboard */
    
    struct GPIO_Mapping
    {
        GPIO_TypeDef* GPIOx;
        uint16_t GPIO_Pin;
    };
    
    struct GPIO_Mapping Columns_C65[9] =
    {
        GPIOE, GPIO_Pin_9,      /* C0 */
        GPIOE, GPIO_Pin_10,     /* C1 */
        GPIOE, GPIO_Pin_11,     /* C2 */
        GPIOE, GPIO_Pin_12,     /* C3 */
        GPIOE, GPIO_Pin_13,     /* C4 */
        GPIOE, GPIO_Pin_14,     /* C5 */
        GPIOE, GPIO_Pin_15,     /* C6 */
        GPIOC, GPIO_Pin_0,      /* C7 */
        GPIOC, GPIO_Pin_1       /* C8 */
    };
    
    struct GPIO_Mapping Rows_C65[11] =
    {
        GPIOE, GPIO_Pin_0,      /* R0 */
        GPIOE, GPIO_Pin_1,      /* R1 */
        GPIOE, GPIO_Pin_2,      /* R2 */
        GPIOE, GPIO_Pin_3,      /* R3 */
        GPIOE, GPIO_Pin_4,      /* R4 */
        GPIOE, GPIO_Pin_5,      /* R5 */
        GPIOE, GPIO_Pin_6,      /* R6 */
        GPIOE, GPIO_Pin_7,      /* R7 */
        GPIOE, GPIO_Pin_8,      /* R8 */
        GPIOC, GPIO_Pin_2,      /* K1 special "row 9" used for scanning the CURSOR UP key */
        GPIOC, GPIO_Pin_3       /* K2 special "row 10" used for scanning the CURSOR LEFT key */
    };
    
    
    const char COL_CSR = 0;
    const char ROW_CSR_UP = 9;
    const char ROW_CSR_LEFT = 10;
    const char NIBBLE_CSR_DOWN = 1;
    const char BIT_CSR_DOWN = 3;
    const char NIBBLE_RIGHT_SHIFT = 13;
    const char BIT_RIGHT_SHIFT = 0;
    
    int i, col, row, nibble_cnt, bit_cnt;
            
	/* Initialize System */
	SystemInit();
    TM_DELAY_Init();
	TM_SWO_Init();  

	TM_SWO_Printf("DM64PIC firmware running.\n");
    
    /* The matrix scan goes like this: let current flow through the columns and then find out if a key is pressed
       by checking, if the current from the column arrives at a certain row. That means, we configure
       all rows as outputs (no pullup/pulldown resistor) and all columns as inputs (pulldown resistor)
    */
    for (i = 0; i < 8; i++)
        TM_GPIO_Init(Columns_C65[i].GPIOx, Columns_C65[i].GPIO_Pin, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High);    
    for (i = 0; i < 10; i++)
        TM_GPIO_Init(Rows_C65[i].GPIOx, Rows_C65[i].GPIO_Pin, TM_GPIO_Mode_IN, TM_GPIO_OType_PP, TM_GPIO_PuPd_DOWN, TM_GPIO_Speed_High);
     
    /* Initialize outputs for communicating with the FPGA via GPIO port JB     
        JB1  = PG8: clock; data must be valid before rising edge
        JB2  = PG9: start of sequece, set to 1 when the first nibble of a new 128bit sequence is presented
        JB3  = PG10: bit0 of output data nibble
        JB4  = PG11: bit1 of output data nibble
        JB7  = PG12: bit2 of output data nibble
        JB8  = PG13: bit3 of output data nibble
        JB9  = PG14: bit 0 of input bit pair
        JB10 = PG15: bit 1 of input bit pair
    */
  
    #define P_CLOCK     GPIO_Pin_8
    #define P_START     GPIO_Pin_9
    #define P_OUT_B0    GPIO_Pin_10
    #define P_OUT_B1    GPIO_Pin_11
    #define P_OUT_B2    GPIO_Pin_12
    #define P_OUT_B3    GPIO_Pin_13
    #define P_IN_B0     GPIO_Pin_14
    #define P_IN_B1     GPIO_Pin_15
       
    /* defines for debug port JA
    #define P_CLOCK     GPIO_Pin_0
    #define P_START     GPIO_Pin_1
    #define P_OUT_B0    GPIO_Pin_2
    #define P_OUT_B1    GPIO_Pin_3
    #define P_OUT_B2    GPIO_Pin_4
    #define P_OUT_B3    GPIO_Pin_5
    #define P_IN_B0     GPIO_Pin_6
    #define P_IN_B1     GPIO_Pin_7
    */
    
    #define FPGA_OUT(__pin, __value) TM_GPIO_SetPinValue(GPIOG, __pin, __value)
    
    /* initialize the pins for the FPGA GPIO communication */
    TM_GPIO_Init(GPIOG, P_CLOCK,  TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High);
    TM_GPIO_Init(GPIOG, P_START,  TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High);
    TM_GPIO_Init(GPIOG, P_OUT_B0, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High);
    TM_GPIO_Init(GPIOG, P_OUT_B1, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High);
    TM_GPIO_Init(GPIOG, P_OUT_B2, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High);
    TM_GPIO_Init(GPIOG, P_OUT_B3, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High);    
/*
    TM_GPIO_Init(GPIOG, P_IN_B0,  TM_GPIO_Mode_IN,  TM_GPIO_OType_PP, TM_GPIO_PuPd_DOWN,   TM_GPIO_Speed_High);
    TM_GPIO_Init(GPIOG, P_IN_B1,  TM_GPIO_Mode_IN,  TM_GPIO_OType_PP, TM_GPIO_PuPd_DOWN,   TM_GPIO_Speed_High);
*/

    
    while(1)
    {
        /* matrix scan the keyboard */
        nibble_cnt = 0;
        bit_cnt = 0;
        for (col = 0; col < 8; col++)
        {
            /* set all columns to LOW except the currently active one */            
            for (i = 0; i < 8; i++)
                TM_GPIO_SetPinValue(Columns_C65[i].GPIOx, Columns_C65[i].GPIO_Pin, (i == col) ? 1 : 0);
                     
            /* perform standard row scanning as the MEGA65 can handle that in an untranslated (raw) way */
            for (row = 0; row < 8; row++)
            {
                if (TM_GPIO_GetInputPinValue(Rows_C65[row].GPIOx, Rows_C65[row].GPIO_Pin) == 1)
                    nibbles[nibble_cnt] = nibbles[nibble_cnt] | (1 << bit_cnt);
                else
                    nibbles[nibble_cnt] = nibbles[nibble_cnt] & (~(1 << bit_cnt));
                
                bit_cnt++;
                
                if (bit_cnt == 4)
                {
                    bit_cnt = 0;
                    nibble_cnt++;
                }

                Delay(1);
            }            
        }
        
        /* handle CURSOR UP and CURSOR LEFT: to be emulated as SHIFT+CURSOR DOWN and SHIFT+CURSOR RIGHT */
        for (i = 0; i < 8; i++)
            TM_GPIO_SetPinValue(Columns_C65[i].GPIOx, Columns_C65[i].GPIO_Pin, (i == COL_CSR) ? 1 : 0);                
        if (TM_GPIO_GetInputPinValue(Rows_C65[ROW_CSR_UP].GPIOx, Rows_C65[ROW_CSR_UP].GPIO_Pin) == 1)
        {
            /* CURSOR UP */
            nibbles[NIBBLE_CSR_DOWN] = nibbles[NIBBLE_CSR_DOWN] | (1 << BIT_CSR_DOWN);
            nibbles[NIBBLE_RIGHT_SHIFT] = nibbles[NIBBLE_RIGHT_SHIFT] | (1 << BIT_RIGHT_SHIFT);
        }
                
        /* transmit current keyboard state to FPGA */
        for (i = 0; i < 32; i++)
        {
            FPGA_OUT(P_CLOCK, 0);
            
            FPGA_OUT(P_START, (i == 0) ? 1 : 0);
            FPGA_OUT(P_OUT_B0, (nibbles[i] & 0x01) ? 0 : 1);
            FPGA_OUT(P_OUT_B1, (nibbles[i] & 0x02) ? 0 : 1);
            FPGA_OUT(P_OUT_B2, (nibbles[i] & 0x04) ? 0 : 1);
            FPGA_OUT(P_OUT_B3, (nibbles[i] & 0x08) ? 0 : 1);
                            
            Delay(32);            
            FPGA_OUT(P_CLOCK, 1);            
            Delay(32);
        }
    }
}
