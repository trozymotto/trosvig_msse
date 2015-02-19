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
 
// receive_buffer: A ring buffer that we will use to receive bytes on USB_COMM.
// The OrangutanSerial library will put received bytes in to
// the buffer starting at the beginning (receiveBuffer[0]).
// After the buffer has been filled, the library will automatically
// start over at the beginning.
char receive_buffer[32];

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

// send_buffer: A buffer for sending bytes on USB_COMM.
char send_buffer[32];

// Macro for 10ms hard timer
uint32_t __ii;
#define COUNTS_10MS 17700
#define WAIT_10MS {for(__ii=0; __ii < COUNTS_10MS; __ii++);}

// Function prototypes
void led_one(int value);
void led_two(int value);
void toggle_led_one(void);
void toggle_led_two(void);
void check_for_new_bytes_received();
void wait_for_sending_to_finish();
void process_received_byte(char byte);
void setup_timer(void);
void enable_interrupts(int enable);
void update_timer1(void);
void blink_red_led_at_1hz(void);

int main()
{

    unsigned long sw1Timer = 0;
    unsigned long sw2Timer = 0;
    clear();    // clear the LCD
    print("Send serial");
    lcd_goto_xy(0, 1);    // go to start of second LCD row
    print("or press B");

    // Set the baud rate to 9600 bits per second.  Each byte takes ten bit
    // times, so you can get at most 960 bytes per second at this speed.
    serial_set_baud_rate(USB_COMM, 9600);

    // Start receiving bytes in the ring buffer.
    serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));

    // Initialize the LEDs
    DDRD |= (1<<PIN1)|(1<<PIN3)|(1<<PIN5);
    led_one(0);
    led_two(0);
    
    // Setup the timer and enable interrupts
    //setup_timer();
    //enable_interrupts(1);
    
    while(1)
    {
        /*int i = 0;
        int oneSecCnt = 150;
        // Test loop timing for 10ms
        tickCnt1 = get_ticks();
        for(i = 0; i < 1; i++);
        toggle_led_one();
        tickCnt2 = get_ticks();
        timeAdjust = tickCnt2 - tickCnt1;
        tickCnt1 = get_ticks();
        for(i = 0; i < oneSecCnt; i++)
            WAIT_10MS;
        tickCnt2 = get_ticks();  
        snprintf(send_buffer, 32, "Time %lu us, %d loops\r\n", 
                ticks_to_microseconds(tickCnt2-tickCnt1), oneSecCnt);
        serial_send(USB_COMM, send_buffer, 32);
        wait_for_sending_to_finish();
        snprintf(send_buffer, 32, "Adjust %lu us\r\n", 
                ticks_to_microseconds(timeAdjust));
        serial_send(USB_COMM, send_buffer, 32);
        wait_for_sending_to_finish();*/
        
        blink_red_led_at_1hz();
        
        // USB_COMM is always in SERIAL_CHECK mode, so we need to call this
        // function often to make sure serial receptions and transmissions
        // occur.
        serial_check();
        unsigned char button = get_single_debounced_button_press(ANY_BUTTON);
/*        
        if(button == BUTTON_A)
        {
            if(sw1Blink == 1)
            {
                sw1Blink = 0;
                led_two(0);
            #if DEBUG
                snprintf(send_buffer, 32, "Led one off %lu\r\n", get_ms());
                serial_send(USB_COMM, send_buffer, 32);
            #endif
            }
            else
                sw1Blink = 1;
        }
        if(sw1Blink == 1)
        {
            if(get_ms() >= sw1Timer)
            {
            #if DEBUG
                snprintf(send_buffer, 32, "Toggle led one %lu\r\n", get_ms());
                serial_send(USB_COMM, send_buffer, 32);
            #endif
                //Toggle the state of the LED
                toggle_led_one();
                sw1Timer = get_ms() + sw1TimeOut;
            }
        }

        if(button == BUTTON_C)
        {
            if(sw2Blink == 1)
            {
                sw2Blink = 0;
                led_two(0);
            #if DEBUG
                snprintf(send_buffer, 32, "Led two off %lu\r\n", get_ms());
                serial_send(USB_COMM, send_buffer, 32);
            #endif
            }
            else
                sw2Blink = 1;
        }
        if(sw2Blink == 1)
        {
            if(get_ms() >= sw2Timer)
            {
            #if DEBUG
                snprintf(send_buffer, 32, "Toggle led two %lu\r\n", get_ms());
                serial_send(USB_COMM, send_buffer, 32);
            #endif
                //Toggle the state of the LED
                toggle_led_two();
                sw2Timer = get_ms() + sw2TimeOut;
            }
        }
*/
        // Deal with any new bytes received.
        check_for_new_bytes_received();

    }
}


void blink_red_led_at_1hz(void)
{
    int j = 0;
    int oneSecCnt = 150;
    unsigned long tickCnt3 = 0;
    unsigned long tickCnt4 = 0;
    // Test loop timing for 10ms
    tickCnt1 = get_ticks();
    WAIT_10MS;
    tickCnt2 = get_ticks();  
    toggle_led_one();
    for(j = 0; j < oneSecCnt; j++)
        {WAIT_10MS;}
    tickCnt3 = get_ticks(); 
    toggle_led_one();
    for(j = 0; j < oneSecCnt; j++)
        {WAIT_10MS;}
    tickCnt4 = get_ticks(); 
    toggle_led_one();
    snprintf(send_buffer, 32, "10ms: %luus\r\n", 
            ticks_to_microseconds(tickCnt2-tickCnt1));
    serial_send(USB_COMM, send_buffer, 32);
    wait_for_sending_to_finish();
    snprintf(send_buffer, 32, "1s: %luus, %luus\r\n", 
            ticks_to_microseconds(tickCnt3-tickCnt2),
            ticks_to_microseconds(tickCnt4-tickCnt3));
    serial_send(USB_COMM, send_buffer, 32);
    wait_for_sending_to_finish();
    
    // Test loop timing for 10ms
    tickCnt1 = get_ticks();
    for(j = 0; j < 1; j++);
    toggle_led_one();
    tickCnt2 = get_ticks();
    timeAdjust = tickCnt2 - tickCnt1;
    tickCnt1 = get_ticks();
    for(j = 0; j < oneSecCnt; j++)
        {WAIT_10MS;}
    tickCnt2 = get_ticks();  
    snprintf(send_buffer, 32, "Time %lu us, %d loops\r\n", 
            ticks_to_microseconds(tickCnt2-tickCnt1), oneSecCnt);
    serial_send(USB_COMM, send_buffer, 32);
    wait_for_sending_to_finish();
    snprintf(send_buffer, 32, "Adjust %lu us\r\n", 
            ticks_to_microseconds(timeAdjust));
    serial_send(USB_COMM, send_buffer, 32);
    wait_for_sending_to_finish();
}

void led_one(int value)
{
  if(value == 1)
    PORTD |= (1<<PIN3);
  else
    PORTD &= ~(1<<PIN3);
  led1state = value;
}

void toggle_led_one(void)
{
  if(led1state == 1)
    led_one(0);
  else
    led_one(1);
}

void led_two(int value)
{
  if(value == 1)
    PORTD &= ~(1<<PIN1);
  else
    PORTD |= (1<<PIN1);
  led2state = value;
}

void toggle_led_two(void)
{
  if(led2state == 1)
    led_two(0);
  else
    led_two(1);
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
        toggle_led_two();
        //snprintf(send_buffer, 32, "TIMER0 interrupt %lu\r\n", get_ms());
        //serial_send(USB_COMM, send_buffer, 32);
    }
}

