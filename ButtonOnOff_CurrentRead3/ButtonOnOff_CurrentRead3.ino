//
// Simple example showing how to set the Sleepy Pi to wake on button press
// and then power up the Raspberry Pi. To switch the RPi off press the button
// again. If the button is held down the Sleepy Pi will cut the power to the
// RPi regardless of any handshaking.
//
// + Prints out the Current Consumption of the RPi to the Serial Monitor
//   every half sec
//

// **** INCLUDES *****
#include "SleepyPi2.h"
#include <Time.h>
#include <LowPower.h>
#include <PCF8523.h>
#include <Wire.h>

#define kBUTTON_POWEROFF_TIME_MS   2000
#define kBUTTON_FORCEOFF_TIME_MS   8000

const char *monthName[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

tmElements_t tm;

// States
typedef enum {
    eWAIT = 0,
    eBUTTON_PRESSED,
    eBUTTON_HELD,
    eBUTTON_RELEASED
} eBUTTONSTATE;

typedef enum {
    ePI_OFF = 0,
    ePI_BOOTING,
    ePI_ON,
    ePI_SHUTTING_DOWN
} ePISTATE;

const int LED_PIN = 13;

volatile bool  buttonPressed = false;
eBUTTONSTATE   buttonState = eBUTTON_RELEASED;
ePISTATE       pi_state = ePI_OFF;
bool state = LOW;
unsigned long  time, timePress;

void button_isr()
{
    // A handler for the Button interrupt.
    buttonPressed = true;
}

bool getTime(const char *str)
{
    int Hour, Min, Sec;

    if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
    tm.Hour = Hour;
    tm.Minute = Min;
    tm.Second = Sec;
    return true;
}

bool getDate(const char *str)
{
    char Month[12];
    int Day, Year;
    uint8_t monthIndex;

    if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
    for (monthIndex = 0; monthIndex < 12; monthIndex++) {
        if (strcmp(Month, monthName[monthIndex]) == 0) break;
    }
    if (monthIndex >= 12) return false;
    tm.Day = Day;
    tm.Month = monthIndex + 1;
    tm.Year = CalendarYrToTm(Year);
    return true;
}

void print2digits(int number)
{
    if (number >= 0 && number < 10) {
        Serial.write('0');
    }
    Serial.print(number);
}

void printTimeNow()
{
    // Read the time
    DateTime now = SleepyPi.readTime();

    Serial.print('[');
    Serial.print(now.month());
    Serial.print('/');
    Serial.print(now.day());
    Serial.print('/');
    Serial.print(now.year(), DEC);
    Serial.print(' ');
    print2digits(now.hour());
    Serial.print(':');
    print2digits(now.minute());
    Serial.print(':');
    print2digits(now.second());
    Serial.print("] ");

    return;
}


//void alarm_isr()
//{
// A handler for the Alarm interrupt.
//}

void setup()
{
    // Configure "Standard" LED pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN,LOW);		// Switch off LED

    SleepyPi.enablePiPower(false);
    SleepyPi.enableExtPower(false);

    // Allow wake up triggered by button press
    attachInterrupt(1, button_isr, LOW);    // button pin

    // Initialize serial communication:
    Serial.begin(9600);
    Serial.println("Start..");
    delay(50);

    SleepyPi.rtcInit(true);

    // Default the clock to the time this was compiled.
    // Comment out if the clock is set by other means
    // ...get the date and time the compiler was run
    if (getDate(__DATE__) && getTime(__TIME__)) {
        // and configure the RTC with this info
        SleepyPi.setTime(DateTime(F(__DATE__), F(__TIME__)));
    }

    printTimeNow();
}

void loop()
{
    bool   pi_running;
    float  pi_current;

    // Enter power down state with ADC and BOD module disabled.
    // Wake up when wake button is pressed.
    // Once button is pressed stay awake.
    pi_running = SleepyPi.checkPiStatus(true);  // Cut Power if we detect Pi not running
    if(pi_running == false) {
        SleepyPi.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    }

    // Button State changed
    if(buttonPressed == true) {
        detachInterrupt(1);
        buttonPressed = false;
        switch(buttonState) {
        case eBUTTON_RELEASED:
            // Button pressed
            timePress = millis();
            pi_running = SleepyPi.checkPiStatus(false);
            if(pi_running == false) {
                // Switch on the Pi
                SleepyPi.enablePiPower(true);
                SleepyPi.enableExtPower(true);
            }
            buttonState = eBUTTON_PRESSED;
            digitalWrite(LED_PIN,HIGH);
            attachInterrupt(1, button_isr, HIGH);
            break;
        case eBUTTON_PRESSED:
            // Button Released
            unsigned long buttonTime;
            time = millis();
            buttonState = eBUTTON_RELEASED;
            pi_running = SleepyPi.checkPiStatus(false);
            if(pi_running == true) {
                // Check how long we have held button for
                buttonTime = time - timePress;
                if(buttonTime > kBUTTON_FORCEOFF_TIME_MS) {
                    // Force Pi Off
                    SleepyPi.enablePiPower(false);
                    SleepyPi.enableExtPower(false);
                } else if (buttonTime > kBUTTON_POWEROFF_TIME_MS) {
                    // Start a shutdown
                    SleepyPi.piShutdown();
                    SleepyPi.enableExtPower(false);
                } else {
                    // Button not held off long - Do nothing
                }
            } else {
                // Pi not running
            }
            digitalWrite(LED_PIN,LOW);
            attachInterrupt(1, button_isr, LOW);    // button pin
            break;
        default:
            break;
        }
    } else {
        printTimeNow();

        pi_current = SleepyPi.rpiCurrent();
        Serial.print("Current: ");
        Serial.print(pi_current);
        Serial.println(" mA");

        digitalWrite(LED_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(250);                    // wait for a quarter second
        digitalWrite(LED_PIN, LOW);    // turn the LED off by making the voltage LOW
        delay(250);                    // wait for a quarter second
    }
}
