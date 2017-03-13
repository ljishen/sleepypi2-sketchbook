//
// Simple example showing how to set the Sleepy Pi to wake on button press
// and then power up the Raspberry Pi. To switch the RPi off press the button
// again. If the button is held down the Sleepy Pi will cut the power to the
// RPi regardless of any handshaking.
//
// + Prints out the Current Consumption of the RPi to the Serial Monitor
//   every 1/4 secs
//

// **** INCLUDES *****
#include "SleepyPi2.h"
#include <Time.h>
#include <Timezone.h>
#include <LowPower.h>
#include <PCF8523.h>
#include <Wire.h>


#define kBUTTON_POWEROFF_TIME_MS   2000
#define kBUTTON_FORCEOFF_TIME_MS   8000

// See https://github.com/JChristensen/Timezone for more details
TimeChangeRule myDST = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 7 hours
TimeChangeRule mySTD = {"PST", First, Sun, Nov, 2, -480};     //Standard time = UTC - 8 hours
Timezone myTZ(myDST, mySTD);

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

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

//Print an integer in "00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintI00(int val)
{
    if (val < 10) Serial.print('0');
    Serial.print(val, DEC);
    return;
}

//Print an integer in ":00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintDigits(int val)
{
    Serial.print(':');
    if(val < 10) Serial.print('0');
    Serial.print(val, DEC);
}

//Function to print time with time zone
void printTime(time_t t, char *tz)
{
    Serial.print('[');
    Serial.print(dayShortStr(weekday(t)));
    Serial.print(' ');
    Serial.print(monthShortStr(month(t)));
    Serial.print(' ');
    sPrintI00(day(t));
    Serial.print(' ');
    sPrintI00(hour(t));
    sPrintDigits(minute(t));
    sPrintDigits(second(t));
    Serial.print(' ');
    Serial.print(tz);
    Serial.print(' ');
    Serial.print(year(t));
    Serial.print("] ");
}

uint32_t  readRTCTime()
{
    DateTime now = SleepyPi.readTime();
    return now.unixtime();
}


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

    setSyncProvider(readRTCTime);   // the function to get the time from the RTC
    if (timeStatus() != timeSet)
        Serial.println("Unable to sync with the RTC");
    else
        Serial.println("RTC has set the system time");
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
        time_t local = myTZ.toLocal(now(), &tcr);
        printTime(local, tcr -> abbrev);

        pi_current = SleepyPi.rpiCurrent();
        Serial.print(pi_current);
        Serial.println(" mA");

        delay(250);
    }
}
