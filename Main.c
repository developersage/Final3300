
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <math.h>
#include <p18f4620.h>
#include <usart.h>
#include <string.h>

#include "Main.h"
#include "I2C_Support.h"
#include "I2C_Soft.h"
#include "TFT_ST7735.h"
#include "Interrupt.h"
#include "Main_Screen.h"

#pragma config OSC      =   INTIO67
#pragma config BOREN    =   OFF
#pragma config WDT      =   OFF
#pragma config LVP      =   OFF
#pragma config CCP2MX   =   PORTBE

void Initialize_Screen(void); 
void Update_Screen(void);
void Do_Init(void);
float read_volt();
int get_duty_cycle(int,int);
int get_RPM(); 
void Monitor_Fan();
void Turn_Off_Fan();
void Turn_On_Fan();
unsigned int get_full_ADC();
void Get_Temp(void);
void Update_Volt(void);
void Test_Alarm(void);
void Activate_Buzzer();
void Deactivate_Buzzer();

void Main_Screen(void);
void Do_Setup(void);
void do_update_pwm(char);
void Set_RGB_Color(char color);

char buffer[31]         = " ECE3301L Fall'20 L12\0";
char *nbr;
char *txt;
char tempC[]            = "+25";
char tempF[]            = "+77";
char time[]             = "00:00:00";
char date[]             = "00/00/00";
char alarm_time[]       = "00:00:00";
char Alarm_SW_Txt[]     = "OFF";
char Fan_SW_Txt[]       = "OFF";                // text storage for Heater Mode
char Fan_Set_Temp_Txt[] = "075F";
char Volt_Txt[]         = "0.00V";              // text storage for Volt     
char DC_Txt[]           = "000";                // text storage for Duty Cycle value
char RTC_ALARM_Txt[]    = "0";                  //
char RPM_Txt[]          = "0000";               // text storage for RPM

char setup_time[]       = "00:00:00";
char setup_date[]       = "01/01/00";
char setup_alarm_time[] = "00:00:00"; 
char setup_fan_text[]   = "075F";

signed int DS1621_tempC, DS1621_tempF;

int INT0_flag, INT1_flag, INT2_flag, Tach_cnt;
int ALARMEN;
int FANEN;
int alarm_mode, MATCHED, color;
unsigned char second, minute, hour, dow, day, month, year, old_sec;
unsigned char alarm_second, alarm_minute, alarm_hour, alarm_date;
unsigned char setup_alarm_second, setup_alarm_minute, setup_alarm_hour;
unsigned char setup_second, setup_minute, setup_hour, setup_day, setup_month, setup_year;
unsigned char setup_fan_temp = 75;
float volt;
int duty_cycle;
int rpm;

int Tach_cnt = 0;		

void putch (char c)
{   
    while (!TRMT);       
    TXREG = c;
}

void init_UART()
{
    OpenUSART (USART_TX_INT_OFF & USART_RX_INT_OFF & USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH, 25);
    OSCCON = 0x70;
}

void Init_ADC()
{
    ADCON0 = 0x01;  //set ADCON0 to 0000 0001 (A/D converter module is enabled)
    ADCON1 = 0x0E;  //set ADCON1 to 0000 1110 (AN1~AN12 to digital and AN0 to analog)
    ADCON2 = 0xA9;  //set ADCON2 to 1010 1001 (ADFM right justified, 12 TAD, Fosc/8)
}

void Init_IO()
{
    TRISA = 0x11;  // set PORTA all output except AN0/RA0 and RA4 for RTC_ALARM#
    TRISB = 0x07;  // set PORTB all output except RB0, RB1, and RB2 (three push buttons)
    TRISC = 0x01;  // set PORTC all output except RC0 (TACH PULSE)
    TRISD = 0x00;  // set PORTD all output
    TRISE = 0x07;  // set PORTE bit0 ~ bit3 to input and everything else output
    // add code to initialize the IO ports - Make to set the proper direction of the I/O pins
}

void Do_Init()                                      // Initialize the ports 
{ 
    init_UART();                                    // Initialize the uart
    OSCCON = 0x70;                                  // Set oscillator to 8 MHz 
    Init_ADC();
    Init_IO();
    RBPU = 0;
       
    TMR1L = 0x00;       //<-- put code here to program Timer 1 as in counter mode. Copy the code from                             
    T1CON = 0x03;       //<-- the Get_RPM() function
 
    T0CON = 0x03;       //<-- Program the Timer 0 to operate in time mode to do an interrupt every 500 msec  
    TMR0L = 0xDB;
    TMR0H = 0x0B;       //<-- copy the code from Wait_Half_Sec()                           
    
    INTCONbits.TMR0IF = 0;  //<-- Clear the interrupt flag        
    T0CONbits.TMR0ON = 1;   //<-- Enable the timer 0         
    INTCONbits.TMR0IE = 1;  //<-- enable timer 0 interrupt        
    Init_Interrupt();       //<-- initialize the other interupts
    
    I2C_Init(100000);               
    DS1621_Init();

} 

void main()
{
    Do_Init();                          // Initialization    

    txt = buffer;     

    Initialize_Screen();

    old_sec = 0xff;
    Turn_Off_Fan();
    ALARMEN = 0;
//  DS3231_Write_Initial_Alarm_Time();                  // uncommented this line if alarm time was corrupted    
    DS3231_Read_Time();                                 // Read time for the first time
    DS3231_Read_Alarm_Time();                           // Read alarm time for the first time
    DS3231_Turn_Off_Alarm();

    while(TRUE)
    { 
        if (enter_setup == 0)         // If setup switch is LOW...
        {
            Main_Screen();              // stay on main screen.
        }
        else                            // Else,
        {
            Do_Setup();                 // Go to setup screen.
        }
    }
}

void Main_Screen()
{
    if (INT0_flag == 1) //if push button 1 is pressed,
    {
        INT0_flag = 0;  //stabilize interrupted flag back to 0
        Turn_Off_Fan();  //turn off the fan
    }
    if (INT1_flag == 1) //if push button 2 is pressed,
    {
        INT1_flag = 0;  //stabilize interrupted flag back to 0
        Turn_On_Fan();  //turn on the fan.
    }
    if (INT2_flag == 1) //if push button 1 is pressed,
    {
        INT2_flag = 0;  //stabilize interrupted flag back to 0
        ALARMEN = !ALARMEN;  //toggle alarm sw - Use variable ALARMEN
    }
    DS3231_Read_Time();  //call DS3231_Read_Time() function

    if (old_sec != second)
    {
        old_sec = second;
        
        Get_Temp(); //get the temperature
        read_volt(); //read the current volt  
        
        //check if the fan is enabled. If so, call Monitor_Fan() to handle the fan
        if (FANEN)  //if FANEN is 1, 
        {
            Monitor_Fan();  //call Monitor_fan() function
        }

        Test_Alarm();  // call routine to handle the alarm  function

        printf ("%02x:%02x:%02x %02x/%02x/%02x ",hour,minute,second,month,day,year);
        printf ("duty cycle = %d  RPM = %d ", duty_cycle, rpm); 
        Update_Screen();
    }    
}

void Do_Setup()
{
    //if the two switches are both off, call setup_time function.
    if (!setup_sel1 && !setup_sel0){
        Setup_Time();
        
    //if the bit2 switch is on and bit3 switch is off, call setup_alarm_time function
    }else if(!setup_sel1 && setup_sel0){
        Setup_Alarm_Time();
    
    //if the bit3 switch is on, call setup_temp_fan function
    }else if(setup_sel1){
        Setup_Temp_Fan();
    }
}

void Get_Temp(void)
{
    DS1621_tempC = DS1621_Read_Temp();              // Read temp

    if ((DS1621_tempC & 0x80) == 0x80)
    {
        DS1621_tempC = 0x80 - (DS1621_tempC & 0x7f);
        DS1621_tempF = 32 - DS1621_tempC * 9 /5;
        printf ("Temperature = -%dC or %dF\r\n", DS1621_tempC, DS1621_tempF);
        DS1621_tempC = 0x80 | DS1621_tempC;            
    }
    else
    {
        DS1621_tempF = DS1621_tempC * 9 /5 + 32;
        printf ("Temperature = %dC or %dF\r\n", DS1621_tempC, DS1621_tempF);            
    }
}

void Monitor_Fan()
{
    duty_cycle = get_duty_cycle(DS1621_tempF, setup_fan_temp);
    do_update_pwm(duty_cycle);
    rpm = get_RPM();    
}

float read_volt()
{   
    //read the light sensor light's voltage and store it into the variable 'volt'
	int nStep = get_full_ADC(); // calculates the # of steps for analog conversion
    volt = nStep * 5 /1024.0; // gets the voltage in Volts, using 5V as reference s instead of 4, also divide by 1024
	return (volt);
}

int get_duty_cycle(int temp, int set_temp)
{	
    //duty cycle is twice the difference of set_temp - temp
    int dc = (int) (2 * (set_temp - temp)); 
    
    // make sure that duty cycle never go beyond 100 and less than 0
    if (dc > 100){
        return 100; //return the duty cycle value
    }else if (dc < 0){
        return 0;
    }else{
        return dc; //return the duty cycle value
    }
}

int get_RPM()
{
    return (Tach_cnt * 60);
    // return the rpm which is simply based on the variable tach_cnt which is automatically measured
    // in the T0ISR(). tach_cnt is rps)
}


void Turn_Off_Fan()
{
    duty_cycle = 0;  //set duty cycle to 0
    do_update_pwm(duty_cycle);  //update the fan speed of 0% of duty cycle
    rpm = 0;  //set rotation per minute to 0.
    FANEN = 0;  //set FANEN to 0
    FANEN_LED = 0;  //set FAN enable LED to 0
}

void Turn_On_Fan()
{
    FANEN = 1;  //set FANEN to 0
    FANEN_LED = 1;  //set FAN enable LED to 1
}

void do_update_pwm(char duty_cycle) 
{ 
    float dc_f;  //declare float variable
    int dc_I;  //declare int variable
    PR2 = 0b00000100 ; // set the frequency for 25 Khz
    T2CON = 0b00000111 ; //
    dc_f = ( 4.0 * duty_cycle / 20.0) ; // calculate factor of duty cycle versus a 25 Khz signal
    dc_I = (int) dc_f; // get the integer part
    if (dc_I > duty_cycle) dc_I++; // round up function
    CCP1CON = ((dc_I & 0x03) << 4) | 0b00001100;
    CCPR1L = (dc_I) >> 2;
}

unsigned int get_full_ADC()
{
    unsigned int result;
    ADCON0bits.GO=1;                     // Start Conversion
    while(ADCON0bits.DONE==1);           // wait for conversion to be completed
    result = (ADRESH * 0x100) + ADRESL;  // combine result of upper byte and
                                         // lower byte into result
    return result;                       // return the result.
}

void Activate_Buzzer()
{
    //PR2 = 0b11111001; 
	//T2CON = 0b00000101; 
	CCPR2L = 0b01001010;
	CCP2CON = 0b00111100;
}

void Deactivate_Buzzer()
{
    CCP2CON = 0x0;
	PORTBbits.RB3 = 0;
}

void Test_Alarm()
{ 
    // Case 1: switch is turned on but alarm_mode is not on
    if (ALARMEN && !alarm_mode){ 
        alarm_mode = 1; //Turn on the alarm_mode
        DS3231_Turn_On_Alarm(); //Call DS3231_Turn_On_Alarm() function
        
    // Case 2: switch is turned off but alarm mode is already on
    }else if(!ALARMEN && alarm_mode){
        alarm_mode = 0; //Turn off the alarm_mode
        MATCHED = 0; //Turn off MATCHED variable
        DS3231_Turn_Off_Alarm(); //Call DS3231_Turn_Off_Alarm() function
        Set_RGB_Color(0); //Set the RGB LED to no color
        Deactivate_Buzzer(); //Deactivate the buzzer
        color = 0; //Reset the color variable
        
    // Case 3: switch is on and alarm_mode is on.
    }else if(ALARMEN && alarm_mode){
        
        //If the time between the actual time and the alarm time are same
        if(!RTC_ALARM_NOT){
            MATCHED = 1; //Set MATCHED to 1 for RGB LED color changing
            Activate_Buzzer(); //Activate buzzer
        }
        if(MATCHED){
            if(volt > 2.5){ // if in Night Mode for photo resistor,
                Deactivate_Buzzer(); //Deactivate the buzzer
                Set_RGB_Color(0); //Set the RGB LED to no color
                MATCHED = 0;
                DS3231_Turn_Off_Alarm();  // reset RTC alarm
                DS3231_Turn_On_Alarm();
            }else{
                Set_RGB_Color(++color); //Change the RGB color every second
            }
        }
    }
}

void Set_RGB_Color(char color)
{
    RGB_RED = color &0x01;  // AND bits with 0001
    RGB_GREEN = (color >> 1) &0x01;  //shift bits to right once and AND with 0001
    RGB_BLUE = (color >> 2) &0x01;  //shift bits to right twice and AND with 0001
}

