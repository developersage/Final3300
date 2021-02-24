#include <xc.h>
#include <p18f4620.h>
#include "Interrupt.h"
#include "Main.h"

extern int INT0_flag, INT1_flag, INT2_flag, Tach_cnt;

void Init_Interrupt(void)
{
    // put the code to intialize the INT0, INT1, INT2 interrupts                                                
    INTCONbits.INT0IF=0; // Clear INT0IF
    INTCON3bits.INT1IF=0; // Clear INT1IF
    INTCON3bits.INT2IF=0; // Clear INT2IF
    INTCON2bits.INTEDG0=0; // INT0 EDGE falling
    INTCON2bits.INTEDG1=0; // INT1 EDGE falling
    INTCON2bits.INTEDG2=0; // INT2 EDGE rising
    INTCONbits.INT0IE =1; // Set INT0 IE
    INTCON3bits.INT1IE=1; // Set INT1 IE
    INTCON3bits.INT2IE=1; // Set INT2 IE
    INTCONbits.GIE = 1;                             // enable global interrupt
}

void interrupt  high_priority chkisr() 
{
    if (INTCONbits.TMR0IF == 1) T0_ISR();           // Timer Interrupt Routine    
    if (INTCONbits.INT0IF == 1) INT0_ISR();         // check the INT0 interrupt
    if (INTCON3bits.INT1IF == 1) INT1_ISR();        // check the INT1 interrupt
    if (INTCON3bits.INT2IF == 1) INT2_ISR();        // check the INT2 interrupt
}

void INT0_ISR() 
{    
    INTCONbits.INT0IF=0;                            // Clear the interrupt flag
    __delay_ms(5);
    INT0_flag = 1;                                  // set software INT0_flag 
} 

void INT1_ISR() 
{ 
    INTCON3bits.INT1IF=0;                           // Clear the interrupt flag
    __delay_ms(5);
    INT1_flag = 1;                                  // set software INT1_flag 
} 

void INT2_ISR() 
{    
    INTCON3bits.INT2IF=0;                           // Clear the interrupt flag
    __delay_ms(5);
    INT2_flag = 1;                                  // set software INT2_flag 
} 

void T0_ISR()
{ 
    INTCONbits.TMR0IF=0;                            // Clear the interrupt flag 
    T0CONbits.TMR0ON=0;                             // Turn off Timer0 
    TMR0H = 0x00;                                   // copy the code from Wait_Half_Sec()                           
    TMR0H = 0x00;                                           
    SEC_LED = ~SEC_LED;                             // Invert LED
    Tach_cnt = TMR1L;                               // Store TMR1L into Tach Count
    TMR1L = 0;                                      // Clear TMR1L
    T0CONbits.TMR0ON=1;                             // Turn on Timer0  
} 

