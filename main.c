// adapted from https://e2e.ti.com/support/microcontrollers/msp430/f/166/p/612333/2254742?pi318617=49https://e2e.ti.com/support/microcontrollers/msp430/f/166/p/612333/2254742?pi318617=49
#include <msp430g2253.h>

#define TCS34725_ADDRESS          (0x29)    /* 7-bit slave address */
#define TCS34725_COMMAND_BIT      (0x80)    /* Command register */
#define TCS34725_ENABLE           (0x00)    /* Enable register */
#define TCS34725_ENABLE_AEN       (0x02)    /* RGBC Enable - Writing 1 activates the ADC, 0 disables it */
#define TCS34725_ENABLE_PON       (0x01)    /* Power on - Writing 1 activates the internal oscillator, 0 disables it */
#define TCS34725_ATIME            (0x01)    /* Integration time */
#define TCS34725_CONTROL          (0x0F)    /* Set the gain level for the sensor */
#define TCS34725_CDATAL		  (0x14)    /* Register address for clear data low byte */
#define RXD        		  (BIT1)    /* UART RXD pin */ 
#define TXD        		  (BIT2)    /* UART TXD pin */ 
#define BUTTON     		  (BIT3)    /* Button S2 */

unsigned int TXByte;

void write(unsigned char Address, unsigned char Content){ // function to write content to a given register address of the TCS34725
    while (UCB0CTL1 & UCTXSTP); // wait for stop sequence to be cleared
    UCB0CTL1 |= UCTR + UCTXSTT; // set to transmit mode and send start sequence
    while(!(IFG2 & UCB0TXIFG)){ } // wait for TX buffer to be ready for new data 
    IFG2 &= ~UCB0TXIFG; // clear USCI_B0 TX int flag

    UCB0TXBUF = (0x80 | Address);  // send command byte (tells which register to write to in the subsequent step)
    while(!(IFG2 & UCB0TXIFG)){ } // wait for TX buffer to be ready for new data

    IFG2 &= ~UCB0TXIFG; // clear USCI_B0 TX int flag

    UCB0TXBUF = Content; // set content of register
    while(!(IFG2 & UCB0TXIFG)){ } // wait for TX buffer to be ready for new data

    IFG2 &= ~UCB0TXIFG; // clear USCI_B0 TX int flag

    UCB0CTL1 |= UCTXSTP; // send stop sequence
}



void UART_TX(char *tx_data,  char ArrayLength) // function to send an array of bytes over UART
{
    unsigned int i=0;
    while(ArrayLength--) // as long as the last byte hasn't been sent yet
    {
        while (! (IFG2 & UCA0TXIFG)); // wait for TX buffer to be ready for new data
        UCA0TXBUF = *tx_data; // put next byte to send in TX buffer
        tx_data++; // increment through the array of bytes to transmit
    }
}



void main(void){

    WDTCTL = WDTPW + WDTHOLD; // disable watchdog

    DCOCTL = CALDCO_8MHZ; //DCO setting = 8MHz
    BCSCTL1 = CALBC1_8MHZ; //DCO setting = 8MHz

    // configure Pins for I2C
    P1SEL |= BIT6 + BIT7; // pin init
    P1SEL2 |= BIT6 + BIT7; // pin init

    UCB0CTL1 = UCSWRST;  // set SW reset, pause operation for setup
    UCB0CTL0 = UCMODE_3 + UCMST + UCSYNC; // I2C master mode
    UCB0CTL1 |= UCSSEL_2; // use SMCLK
    UCB0BR0 = 12; // 100 kHz
    UCB0I2CSA = TCS34725_ADDRESS; // set slave address
    
    IFG2 &= ~UCB0TXIFG; // clear USCI_B0 TX int flag
    UCB0CTL1 &= ~UCSWRST; // clear SW reset, resume operation

    // also set up UART (all except UART timing)

    P1DIR &= ~BUTTON; // ensure button is input (sets a 0 in P1DIR register at location BIT3)
    P1OUT |=  BUTTON; // set button S2 to output mode
    P1REN |=  BUTTON; // enable pullup resistor on button

    P1SEL |= RXD + TXD; // select TX and RX functionality for P1.1 & P1.2
    P1SEL2 |= RXD + TXD; //

    UCA0CTL1 |= UCSSEL_2; // have USCI use System Master Clock: AKA core clk 1MHz

    UCA0BR0 = 104; // 1MHz 9600
    UCA0BR1 = 0; //

    UCA0MCTL = UCBRS0; // modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST; // start USCI state machine

    // initialize the sensor

    __delay_cycles(500000); //wait for slave to initialize

    write(TCS34725_ATIME, 0xff);  // set integration time to 2.4mS
    write(TCS34725_CONTROL, 0x00);  // set RGBC gain control to 1X gain
    write(TCS34725_ENABLE, TCS34725_ENABLE_PON); // turn TCS34725 power on

    __delay_cycles(500000); // wait >> 3 ms

    write(TCS34725_ENABLE, (TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN));  // enable ADCs

    __delay_cycles(500000); // wait >> 3 ms


    // enter RGBC measurement and transmission loop

    while(1){ // loop infinitely
        if(!((P1IN & BUTTON)==BUTTON)) // was button pressed?  
        {
            //send the start sequence
            BCSCTL1 = CALBC1_1MHZ; // set DCO to 1 MHz for UART            
            DCOCTL = CALDCO_1MHZ; // set DCO to 1 MHz for UART
            int Start_Data[7] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};   // create array containing start sequence
            UART_TX(Start_Data, 14); // send start sequence over UART

            //now switch back to I2C timing
            DCOCTL = CALDCO_8MHZ; //DCO setting = 8MHz
            BCSCTL1 = CALBC1_8MHZ; //DCO setting = 8MHz

            int k;
            for( k = 1; k < 21; k = k + 1 ){ // measure 30 times and send all of that!
                while (UCB0CTL1 & UCTXSTP); // stop condition sent?
                UCB0CTL1 |= UCTR + UCTXSTT; //UCTR sets it to transmit mode
                while(!(IFG2 & UCB0TXIFG)){ } // wait for TX buffer to be ready for new data
                IFG2 &= ~UCB0TXIFG; // clear USCI_B0 TX int flag

                UCB0TXBUF = (0xA0 | TCS34725_CDATAL); //send the auto-increment version of the command byte: use A0 instead of 80
                while(!(IFG2 & UCB0TXIFG)){ } // wait for TX buffer to be ready for new data

                IFG2 &= ~UCB0TXIFG;// clear USCI_B0 TX int flag

                UCB0CTL1 &= ~UCTR; // clear UCTR (this sets the I2C module to receive mode)
                UCB0CTL1 |= UCTXSTT; // send repeated start condition
                while (UCB0CTL1 & UCTXSTT); // start condition sent?

                while(!(IFG2 & UCB0RXIFG)){ } // wait for RX buffer to be ready for new data


                int a; // define a counter
                int R_Data[9] = {0x80, 0, 0, 0, 0, 0, 0, 0, 0}; // Rx data array of length 7: the first byte is a placeholder

                for( a = 1; a < 10; a = a + 1 ){ // as long as there are still bytes to receive
                    while(!(IFG2 & UCB0RXIFG)){ } // wait for UCBORXIFG to be set
                    R_Data[a] = UCB0RXBUF;	// put each byte in as they come
                }

                UCB0CTL1 |= UCTXSTP; // send stop condition

                __delay_cycles(500000); // wait >> 3 ms

                // now set up UART timing
                BCSCTL1 = CALBC1_1MHZ; // set DCO to 1 MHz
                DCOCTL = CALDCO_1MHZ; // set DCO to 1 MHz

	        int b;
	        for( b = 1; b < 11; b = b + 1 ){ // do the following ten times
                    UART_TX(R_Data, 18); // send array of received data to computer
                    __delay_cycles(100000); // debounce button so signal is not sent multiple times
                }
            }
        }
    }
}

