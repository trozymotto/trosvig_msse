#include <pololu/orangutan.h>
//#include <pololu/OrangutanPushbuttons.h>
#include <stdio.h>

/*
 * Ryan Trosvig (3-1-2015)
 * MSSE Lab 1: 
 * 
 * Processor speed is 20MHz
 *
 */
 
 // Enable or disable debug output
 #define DEBUG 0
 
 #define BUFFER_LEN 32
 
 enum bool{
     FALSE,
     TRUE
 };
 
 enum led_type{
     GREEN,
     YELLOW,
     RED,
     NUM_LEDS
 };
 
enum bool led_status[NUM_LEDS];
 
// receive_buffer: A ring buffer that we will use to receive bytes on USB_COMM.
// The OrangutanSerial library will put received bytes in to
// the buffer starting at the beginning (receiveBuffer[0]).
// After the buffer has been filled, the library will automatically
// start over at the beginning.
char receive_buffer[BUFFER_LEN];

// receive_buffer_position: This variable will keep track of which bytes in the receive buffer
// we have already processed.  It is the offset (0-31) of the next byte
// in the buffer to process.
unsigned char receive_buffer_position = 0;

#define TIMEOUT_LOW_LIMIT 100
#define TIMER1_LOW_LIMIT 1000

// Local variables
unsigned long timeOutBump = 50;
unsigned long sw1TimeOut = 500;
unsigned long sw2TimeOut = 500;
int sw1Blink = 0;
int sw2Blink = 0;
int led1state = 0;
int led2state = 0;
uint16_t timer1_bump = 500;
uint16_t timer1_top = 50000;
uint16_t timer1_divisor = 2;
unsigned long tickCnt1 = 0;
unsigned long tickCnt2 = 0;
unsigned long timeAdjust = 0;
enum bool bTimerZero = 0;

// send_buffer: A buffer for sending bytes on USB_COMM.
char send_buffer[BUFFER_LEN];

// Macro for 10ms hard timer
volatile uint32_t __ii;
#define COUNTS_10MS 5565
#define WAIT_10MS {for(__ii=0; __ii < COUNTS_10MS; __ii++);}

// Function prototypes
void initialize(void);
void SetLed(enum led_type led, enum bool value);
void ToggleLed(enum led_type led);
void check_for_new_bytes_received();
void wait_for_sending_to_finish();
void process_received_byte(char byte);
void setup_timer(void);
void enable_interrupts(int enable);
void update_timer1(void);
void blink_red_led_at_1hz(void);
void clear_comm_buffer(void);


void initialize()
{
    // Print out my slogan
    clear();    
    print("MSSE 2015");
    lcd_goto_xy(0, 1);    // go to start of second LCD row
    print("trozymotto");

    // Set the baud rate to 9600 bits per second.  Each byte takes ten bit
    // times, so you can get at most 960 bytes per second at this speed.
    serial_set_baud_rate(USB_COMM, 9600);

    // Start receiving bytes in the ring buffer.
    serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));

    // Initialize the ports
    DDRD = (1<<PIN5);           // Green LED
    DDRA = (1<<PIN0)|(1<<PIN2); // Yellow, Red LED
    
    SetLed(GREEN, 0);
    SetLed(YELLOW, 0);
    SetLed(RED, 0);
    
    // Setup the timer and enable interrupts
    setup_timer();
    enable_interrupts(1);
}

int main()
{

    initialize();
    
    while(1)
    {
        
        blink_red_led_at_1hz();
        
        if(bTimerZero)
        {
            bTimerZero = FALSE;
            ToggleLed(GREEN);
        }
        
        // USB_COMM is always in SERIAL_CHECK mode, so we need to call this
        // function often to make sure serial receptions and transmissions
        // occur.
        serial_check();

        // Deal with any new bytes received.
        check_for_new_bytes_received();

    }
}

void clear_comm_buffer(void)
{
    volatile int i = 0;
    for(i =0; i < BUFFER_LEN; i++)
        send_buffer[i] = 0;
}

void blink_red_led_at_1hz(void)
{
    volatile int j = 0;
    int oneSecCnt = 100;
    unsigned long tickCnt3 = 0;
    // Test overhead timing
    tickCnt1 = get_ticks();
    for(j = 0; j < 1; j++);
    tickCnt2 = get_ticks();
    timeAdjust = tickCnt2 - tickCnt1;
    // Test loop timing for 10ms
    tickCnt1 = get_ticks();
    WAIT_10MS;
    tickCnt2 = get_ticks();  
    ToggleLed(RED);
    for(j = 0; j < oneSecCnt; j++)
        {WAIT_10MS;}
    tickCnt3 = get_ticks(); 
    ToggleLed(RED);
    for(j = 0; j < oneSecCnt; j++)
        {WAIT_10MS;}
    
    // Send out a serial message to indicate the timing
    clear_comm_buffer();
    snprintf(send_buffer, 32, "10ms: %luus, 1s: %luus\r\n", 
            ticks_to_microseconds(tickCnt2-tickCnt1-timeAdjust),
            ticks_to_microseconds(tickCnt3-tickCnt2));
    serial_send(USB_COMM, send_buffer, 32);
    wait_for_sending_to_finish();
}

void SetLed(enum led_type led, enum bool value)
{
    if(value == TRUE)
    {
        switch(led)
        {
        case GREEN:
            PORTD |= (1<<PIN5);
            break;
        case YELLOW:
            PORTA |= (1<<PIN0);
            break;
        case RED:
            PORTA |= (1<<PIN2);
            break;
        
        default:
            break;
        }
        led_status[led] = TRUE;
    }
    else
    {
        switch(led)
        {
        case GREEN:
            PORTD &= ~(1<<PIN5);
            break;
        case YELLOW:
            PORTA &= ~(1<<PIN0);
            break;
        case RED:
            PORTA &= ~(1<<PIN2);
            break;
        default:
            break;
        }
        led_status[led] = FALSE;
    }
    led_status[led] = value;
}

void ToggleLed(enum led_type led)
{
    if(led_status[led] == TRUE)
    {
        switch(led)
        {
        case GREEN:
            PORTD &= ~(1<<PIN5);
            break;
        case YELLOW:
            PORTA &= ~(1<<PIN0);
            break;
        case RED:
            PORTA &= ~(1<<PIN2);
            break;
        default:
            break;
        }
        led_status[led] = FALSE;
    }
    else
    {
        switch(led)
        {
        case GREEN:
            PORTD |= (1<<PIN5);
            break;
        case YELLOW:
            PORTA |= (1<<PIN0);
            break;
        case RED:
            PORTA |= (1<<PIN2);
            break;
        default:
            break;
        }
        led_status[led] = TRUE;
    }
}

// wait_for_sending_to_finish:  Waits for the bytes in the send buffer to
// finish transmitting on USB_COMM.  We must call this before modifying
// send_buffer or trying to send more bytes, because otherwise we could
// corrupt an existing transmission.
void wait_for_sending_to_finish()
{
    while(!serial_send_buffer_empty(USB_COMM))
    serial_check();        // USB_COMM port is always in SERIAL_CHECK mode
}

// process_received_byte: Responds to a byte that has been received on
// USB_COMM.  If you are writing your own serial program, you can
// replace all the code in this function with your own custom behaviors.
void process_received_byte(char byte)
{
    clear();        // clear LCD
    print("RX: ");
    print_character(byte);
    lcd_goto_xy(0, 1);    // go to start of second LCD row

    switch(byte)
    {
        // If the character 'H' or 'h' is received, show help text.
        case 'H':
        case 'h':
            green_led(TOGGLE);
            memcpy_P(send_buffer, PSTR("+ to increase - to decrease\r\n"), 32);
            serial_send(USB_COMM, send_buffer, 32);
            print("help text");
            break;

        // If the character '+' is received, Increment the time.
        case '+':
            //increment time
            if(sw1Blink)
            {
                sw1TimeOut += timeOutBump;
                snprintf(send_buffer, 32, "+ led one time %lu\r\n", sw1TimeOut);
                serial_send(USB_COMM, send_buffer, 32);
            }
            if(sw2Blink)
            {    
                sw2TimeOut += timeOutBump;
                snprintf(send_buffer, 32, "+ led two time %lu\r\n", sw2TimeOut);
                serial_send(USB_COMM, send_buffer, 32);
            }
            if(timer1_top+timer1_bump < 0xFFFF)
            {
                timer1_top -= timer1_bump;
                update_timer1();
            }
            print("+ Time!");
            break;
                         
        // If the character '+' is received, Increment the time.
        case '-':
            print("- Time!");
            //decrement time
            if(sw1Blink && sw1TimeOut > TIMEOUT_LOW_LIMIT)
            {
                sw1TimeOut -= timeOutBump;
                snprintf(send_buffer, 32, "- led one time %lu\r\n", sw1TimeOut);
                serial_send(USB_COMM, send_buffer, 32);
            }
            if(sw2Blink && sw2TimeOut > TIMEOUT_LOW_LIMIT)
            {    
                sw2TimeOut -= timeOutBump;
                snprintf(send_buffer, 32, "- led two time %lu\r\n", sw2TimeOut);
                serial_send(USB_COMM, send_buffer, 32);
            }
            if(timer1_top-timer1_bump > TIMER1_LOW_LIMIT)
            {
                timer1_top -= timer1_bump;
                update_timer1();
            }
            break;
            
        // If any other character is received, change its capitalization and
        // send it back.
        default:
            wait_for_sending_to_finish();
            send_buffer[0] = byte ^ 0x20;
            serial_send(USB_COMM, send_buffer, 1);
            print("TX: ");
            print_character(send_buffer[0]);
            break;
    }
}

void check_for_new_bytes_received()
{
    while(serial_get_received_bytes(USB_COMM) != receive_buffer_position)
    {
        // Process the new byte that has just been received.
        process_received_byte(receive_buffer[receive_buffer_position]);

        // Increment receive_buffer_position, but wrap around when it gets to
        // the end of the buffer. 
        if (receive_buffer_position == sizeof(receive_buffer)-1)
        {
            receive_buffer_position = 0;
        }
        else
        {
            receive_buffer_position++;
        }
    }
}


void setup_timer(void)
{
    // Setup timer 0 for a 1ms tick time
    // 20MHz / 
    // 0x80 COM0A1: Clear OC0A on Compare Match
    // 0x02 WGM01: CTC
    TCCR0A = 0x82;
    TCCR0B = 0x03;
    OCR0A = 0xFF; // Max overflow clear

    // PD5 PWM output
    // Mode 14 wgm, clock select, ocr 1a to half of ocr
    TCCR1A = 0x82;
    TCCR1B = 0x1D;
    update_timer1();

}

void update_timer1(void)
{
    // Clock is Hz, divisor is 1024 = 
    ICR1 = timer1_top;
    OCR1A = timer1_top/timer1_divisor; 
}

void enable_interrupts(int enable)
{
    switch(enable)
    {
    case 1:
        TIMSK0 = 0x02; // Enable Timer 0 A interrupt
        sei(); // Global interrupt enable
        break;
    case 0:
        cli(); // Global interrupt disable
        break;
    default:
        break;
    }
}

// This is the interrupt vector for
// Timer 0 Compare A
ISR(TIMER0_COMPA_vect)
{
    static int count = 0;
    
    count++;
    if( count > 1000 )
    {
        count = 0;
        bTimerZero = TRUE;
    }
}

