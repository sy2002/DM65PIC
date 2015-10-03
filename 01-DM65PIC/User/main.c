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
    
    
    /* map the pins of the DM65PIC to the columns and rows of the C65 and C64 keyboard
       a very good source of information is chapter 2.1.2 in the following document:
       http://www.zimmers.net/cbmpics/cbm/c65/c65manual.txt */
    
    struct GPIO_Mapping
    {
        GPIO_TypeDef* GPIOx;    /* STM32 GPIO port */
        uint16_t GPIO_Pin;      /* pin within the specified GPIO port */
    };
    
    const char Size_ColMapping_C65 = 9;
    struct GPIO_Mapping Columns_C65[Size_ColMapping_C65] =
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
        
    const char Size_RowMapping_C65 = 11;
    struct GPIO_Mapping Rows_C65[Size_RowMapping_C65] =
    {
        GPIOE, GPIO_Pin_0,      /* R0 */
        GPIOE, GPIO_Pin_1,      /* R1 */
        GPIOE, GPIO_Pin_2,      /* R2 */
        GPIOE, GPIO_Pin_3,      /* R3 */
        GPIOE, GPIO_Pin_4,      /* R4 */
        GPIOE, GPIO_Pin_5,      /* R5 */
        GPIOE, GPIO_Pin_6,      /* R6 */
        GPIOE, GPIO_Pin_7,      /* R7 */
        GPIOE, GPIO_Pin_8,      /* R8 is exclusively used for the CAPS LOCK aka ASCII/DIN key */
        GPIOC, GPIO_Pin_2,      /* K1 special "row 9" used for scanning the CURSOR UP key */
        GPIOC, GPIO_Pin_3       /* K2 special "row 10" used for scanning the CURSOR LEFT key */
    };
    
    struct GPIO_Mapping Restore_C65 =
    {
        GPIOC, GPIO_Pin_15      /* RESTORE key */
    };
        
    const char Size_ColMapping_C64 = 8;
    struct GPIO_Mapping Columns_C64[Size_ColMapping_C64] =
    {
        GPIOD, GPIO_Pin_8,      /* C0 */
        GPIOD, GPIO_Pin_9,      /* C1 */
        GPIOD, GPIO_Pin_10,     /* C2 */
        GPIOD, GPIO_Pin_11,     /* C3 */
        GPIOD, GPIO_Pin_12,     /* C4 */
        GPIOD, GPIO_Pin_13,     /* C5 */
        GPIOD, GPIO_Pin_14,     /* C6 */
        GPIOD, GPIO_Pin_15      /* C7 */
    };
        
    const char Size_RowMapping_C64 = 8;
    struct GPIO_Mapping Rows_C64[Size_RowMapping_C64] =
    {
        GPIOD, GPIO_Pin_0,      /* R0 */
        GPIOD, GPIO_Pin_1,      /* R1 */
        GPIOD, GPIO_Pin_2,      /* R2 */
        GPIOD, GPIO_Pin_3,      /* R3 */
        GPIOD, GPIO_Pin_4,      /* R4 */
        GPIOD, GPIO_Pin_5,      /* R5 */
        GPIOD, GPIO_Pin_6,      /* R6 */
        GPIOD, GPIO_Pin_7       /* R7 */        
    };
    
    struct GPIO_Mapping Restore_C64 =
    {
        GPIOC, GPIO_Pin_14      /* RESTORE key */
    };    
            
    /* joystick mapping at the FPGA's JB port: GPIO => nibble-pos and bit-pos
        nbl #18 : joystick 1 : bit0=up, bit1=down, bit2=left, bit3=right
        nbl #19 : bit0=joy1 fire, bit2=capslock key status, bit3=restore key status
        nbl #20 : joystick 2 : bit0=up, bit1=down, bit2=left, bit3=right
        nbl #21 : bit0=joy2 fire, bit3=reset momentary-action switch status
    
       in C65 mode, which is the default, port #1 and #2 are swapped due to the
       way, how the DM65PIC is located in the MEGA65 body housing; this is why
       in below-mentioned table, joystick #1's left is going to nbl #20 instead of #18
    */  
    const char Size_JoyMapping = 10;    
    struct
    {
        GPIO_TypeDef* GPIOx;
        uint16_t GPIO_Pin;
        char nibble_count;
        char bit_count;
    } Joystick[Size_JoyMapping] =
    {
        GPIOC, GPIO_Pin_6,  20, 2,  /* Joystick #1 LEFT */
        GPIOC, GPIO_Pin_7,  20, 3,  /* Joystick #1 RIGHT */
        GPIOC, GPIO_Pin_4,  20, 0,  /* Joystick #1 UP */
        GPIOC, GPIO_Pin_5,  20, 1,  /* Joystick #1 DOWN */
        GPIOC, GPIO_Pin_12, 21, 0,  /* Joystick #1 BUTTON */
        
        GPIOC, GPIO_Pin_10, 18, 2,  /* Joystick #2 LEFT */
        GPIOC, GPIO_Pin_11, 18, 3,  /* Joystick #2 RIGHT */
        GPIOC, GPIO_Pin_9 , 18, 0,  /* Joystick #2 UP */
        GPIOC, GPIO_Pin_8,  18, 1,  /* Joystick #2 DOWN */
        GPIOC, GPIO_Pin_13, 19, 0   /* Joystick #2 BUTTON */
    };
        
    /* positions of special keys within the matrix */
    const char COL_CSR = 0;
    const char ROW_CSR_UP = 9;
    const char ROW_CSR_LEFT = 10;
    const char ROW_CAPSLOCK = 8;
    
    /* positions of special keys within the nibbles array */
    const char NIBBLE_CSR_DOWN = 1;
    const char BIT_CSR_DOWN = 3;
    const char NIBBLE_CSR_RIGHT = 0;
    const char BIT_CSR_RIGHT = 2;
    const char NIBBLE_RIGHT_SHIFT = 13;
    const char BIT_RIGHT_SHIFT = 0;
    const char NIBBLE_RESTORE = 19;
    const char BIT_RESTORE = 3;
    const char NIBBLE_CAPSLOCK = 19;
    const char BIT_CAPSLOCK = 2;
    
    int i, col, row, nibble_cnt, bit_cnt;
    int tmp_offs, tmp_nbl, tmp_bit;
    
    /* if ever any key of a C64 keyboard has been pressed, then DM64PIC switches into
       a dedicated C64 mode that swaps back the joystick ports to their "natural" (aka as printed on the PBC)
       order because when the DM65PIC is located in a C64 body housing, the ports are placed correctly */
    char C64_Mode = 0;
            
	/* Initialize System */
	SystemInit();
    TM_DELAY_Init();
	TM_SWO_Init();  

	TM_SWO_Printf("DM64PIC firmware running.\n");
    
    /* The matrix scan goes like this: let current flow through the columns and then find out if a key is pressed
       by checking, if the current from the column arrives at a certain row. That means, we configure
       all rows as outputs (no pullup/pulldown resistor) and all columns as inputs (pulldown resistor)
    */
    for (i = 0; i < Size_ColMapping_C65; i++)
        TM_GPIO_Init(Columns_C65[i].GPIOx, Columns_C65[i].GPIO_Pin, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High);    
    for (i = 0; i < Size_RowMapping_C65; i++)
        TM_GPIO_Init(Rows_C65[i].GPIOx, Rows_C65[i].GPIO_Pin, TM_GPIO_Mode_IN, TM_GPIO_OType_PP, TM_GPIO_PuPd_DOWN, TM_GPIO_Speed_High);
    for (i = 0; i < Size_ColMapping_C64; i++)
        TM_GPIO_Init(Columns_C64[i].GPIOx, Columns_C64[i].GPIO_Pin, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High);    
    for (i = 0; i < Size_RowMapping_C64; i++)
        TM_GPIO_Init(Rows_C64[i].GPIOx, Rows_C64[i].GPIO_Pin, TM_GPIO_Mode_IN, TM_GPIO_OType_PP, TM_GPIO_PuPd_DOWN, TM_GPIO_Speed_High);
        
    /* row 8 is exclusively used for CAPS LOCK (aka ASCII/DIN) and has inverse logic
      (pulled to GND when the key is pressed), so use pullup resistor */
    TM_GPIO_Init(Rows_C65[ROW_CAPSLOCK].GPIOx, Rows_C65[ROW_CAPSLOCK].GPIO_Pin, TM_GPIO_Mode_IN, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_High);
    
    /* The RESTORE key is pulled to GND when pressed, so we need a pullup resistor */
    TM_GPIO_Init(Restore_C65.GPIOx, Restore_C65.GPIO_Pin, TM_GPIO_Mode_IN, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_High);
    TM_GPIO_Init(Restore_C64.GPIOx, Restore_C64.GPIO_Pin, TM_GPIO_Mode_IN, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_High);
        
    /* Joysticks are inverse logic, too, therefore pullup resistors are needed for the inputs */
    for (i = 0; i < Size_JoyMapping; i++)
        TM_GPIO_Init(Joystick[i].GPIOx, Joystick[i].GPIO_Pin, TM_GPIO_Mode_IN, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_High);
    
    /* Initialize outputs for communicating with the FPGA via GPIO port JB for debugging    
        JB1  = PG8: clock; data must be valid before rising edge
        JB2  = PG9: start of sequence, set to 1 when the first nibble of a new 128bit sequence is presented
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

    /* convenience defines for a better readability */
    #define FPGA_OUT(__pin, __value) TM_GPIO_SetPinValue(GPIOG, __pin, __value)    
    #define DM_SET_BIT(__nibblepos, __bitpos) nibbles[(__nibblepos)] = nibbles[(__nibblepos)] | (1 << (__bitpos))
    #define DM_CLR_BIT(__nibblepos, __bitpos) nibbles[(__nibblepos)] = nibbles[(__nibblepos)] & (~(1 << (__bitpos)))
    
    while(1)
    {
        /* matrix scan the keyboard */
        nibble_cnt = 0;
        bit_cnt = 0;
        for (col = 0; col < Size_ColMapping_C65; col++)
        {
            /* set all columns to LOW except the currently active one */
            for (i = 0; i < Size_ColMapping_C65; i++)
            {
                TM_GPIO_SetPinValue(Columns_C65[i].GPIOx, Columns_C65[i].GPIO_Pin, (i == col) ? 1 : 0);
                if (i < Size_ColMapping_C64)
                    TM_GPIO_SetPinValue(Columns_C64[i].GPIOx, Columns_C64[i].GPIO_Pin, (i == col) ? 1 : 0);                    
            }
                     
            /* perform standard row scanning as the MEGA65 can handle that in an untranslated (raw) way
               deliberately only scan rows 0..7 as row 8 is only for CAPS LOCK (ASCII/DIN), which needs a special treatment 
               plus: this way of doing it elegantly enables us to scan the C64 rows in parallel */            
            for (row = 0; row < 8; row++)
            {
                /* Commodore 65 */
                if (TM_GPIO_GetInputPinValue(Rows_C65[row].GPIOx, Rows_C65[row].GPIO_Pin) == 1)                    
                {
                    /* a key is pressed, so set the corresponding matrix bit */
                    DM_SET_BIT(nibble_cnt, bit_cnt);
                  	TM_SWO_Printf("C65 Key: col=%i, row=%i, nibble=%i, bit=%i.\n", col, row, nibble_cnt, bit_cnt);
                }
                
                /* Commodore 64 */
                else if (col < Size_ColMapping_C64 ? TM_GPIO_GetInputPinValue(Rows_C64[row].GPIOx, Rows_C64[row].GPIO_Pin) == 1 : 0)
                {    
                    /* detect the C64 mode */
                    if (!C64_Mode)
                    {
                        C64_Mode = 1;
                        TM_SWO_Printf("C64 mode detected. Swapping joystick ports back to normal.\n");
                        
                        for (i = 0; i < Size_JoyMapping / 2; i++)
                        {
                            tmp_offs = (Size_JoyMapping / 2) + i;
                            tmp_nbl = Joystick[i].nibble_count;
                            tmp_bit = Joystick[i].bit_count;
                            Joystick[i].nibble_count = Joystick[tmp_offs].nibble_count;
                            Joystick[i].bit_count = Joystick[tmp_offs].bit_count;
                            Joystick[tmp_offs].nibble_count = tmp_nbl;
                            Joystick[tmp_offs].bit_count = tmp_bit;
                        }
                    }
                    
                    /* a key is pressed, so set the corresponding matrix bit */
                    DM_SET_BIT(nibble_cnt, bit_cnt);
                  	TM_SWO_Printf("C64 Key: col=%i, row=%i, nibble=%i, bit=%i.\n", col, row, nibble_cnt, bit_cnt);                    
                }
                
                /* key is released, so clear the corresponding matrix bit */
                else
                    DM_CLR_BIT(nibble_cnt, bit_cnt);
                
                bit_cnt++;
                
                if (bit_cnt == 4)
                {
                    bit_cnt = 0;
                    nibble_cnt++;
                }

                Delay(1);
            }            
        }
        
        /* C65 only: handle CURSOR UP and CURSOR LEFT: to be emulated as SHIFT+CURSOR DOWN and SHIFT+CURSOR RIGHT */
        for (i = 0; i < 8; i++)
            TM_GPIO_SetPinValue(Columns_C65[i].GPIOx, Columns_C65[i].GPIO_Pin, (i == COL_CSR) ? 1 : 0);                
        if (TM_GPIO_GetInputPinValue(Rows_C65[ROW_CSR_UP].GPIOx, Rows_C65[ROW_CSR_UP].GPIO_Pin) == 1)
        {
            /* CURSOR UP */
            DM_SET_BIT(NIBBLE_CSR_DOWN, BIT_CSR_DOWN);
            DM_SET_BIT(NIBBLE_RIGHT_SHIFT, BIT_RIGHT_SHIFT);
        }
        Delay(1);
        if (TM_GPIO_GetInputPinValue(Rows_C65[ROW_CSR_LEFT].GPIOx, Rows_C65[ROW_CSR_LEFT].GPIO_Pin) == 1)
        {
            /* CURSOR LEFT */
            DM_SET_BIT(NIBBLE_CSR_RIGHT, BIT_CSR_RIGHT);
            DM_SET_BIT(NIBBLE_RIGHT_SHIFT, BIT_RIGHT_SHIFT);            
        }
        Delay(1);
        
        /* handle RESTORE key (inverse logic) */
        if ((TM_GPIO_GetInputPinValue(Restore_C65.GPIOx, Restore_C65.GPIO_Pin) == 0) ||
            (TM_GPIO_GetInputPinValue(Restore_C64.GPIOx, Restore_C64.GPIO_Pin) == 0))
            DM_SET_BIT(NIBBLE_RESTORE, BIT_RESTORE);
        else
            DM_CLR_BIT(NIBBLE_RESTORE, BIT_RESTORE);
        Delay(1);
        
        /* C65 only: handle CAPS LOCK (aka ASCII/DIN) (inverse logic) */
        if (TM_GPIO_GetInputPinValue(Rows_C65[ROW_CAPSLOCK].GPIOx, Rows_C65[ROW_CAPSLOCK].GPIO_Pin) == 0)
            DM_SET_BIT(NIBBLE_CAPSLOCK, BIT_CAPSLOCK);
        else
            DM_CLR_BIT(NIBBLE_CAPSLOCK, BIT_CAPSLOCK);
        Delay(1);
        
        /* handle joysticks */
        for (i = 0; i < Size_JoyMapping; i++)
        {
            if (TM_GPIO_GetInputPinValue(Joystick[i].GPIOx, Joystick[i].GPIO_Pin) == 0)
            {
                DM_SET_BIT(Joystick[i].nibble_count, Joystick[i].bit_count);
              	TM_SWO_Printf("Joystick: i=%i, nibble=%i, bit=%i.\n", i, Joystick[i].nibble_count, Joystick[i].bit_count);
            }
            else
                DM_CLR_BIT(Joystick[i].nibble_count, Joystick[i].bit_count);
            Delay(1);
        }
                        
        /* transmit current keyboard and joystick state to FPGA */
        for (i = 0; i < 32; i++)
        {
            FPGA_OUT(P_CLOCK, 0);                                   /* clock = 0 while data is being assembled */            
            FPGA_OUT(P_START, (i == 0) ? 1 : 0);                    /* start of sequence = 1 at the very first nibble */
            
            FPGA_OUT(P_OUT_B0, (nibbles[i] & 0x01) ? 0 : 1);        /* set data lines and use inverse logic */
            FPGA_OUT(P_OUT_B1, (nibbles[i] & 0x02) ? 0 : 1);
            FPGA_OUT(P_OUT_B2, (nibbles[i] & 0x04) ? 0 : 1);
            FPGA_OUT(P_OUT_B3, (nibbles[i] & 0x08) ? 0 : 1);
                            
            Delay(1);                                               /* wait for everything to settle ... */
            FPGA_OUT(P_CLOCK, 1);                                   /* ... then clock = 1 to trigger the FPGA to read */
            Delay(1);                                               /* give the FPGA's flip/flops some time to read the data */
        }
    }
}
