#include "FreeRTOS.h"
#include "debug.h"
#include "timers.h"
#include "deck.h"
#include "led.h"
#include "log.h"

#define RFID_INPUT_PIN DECK_GPIO_TX2
#define RFID_INDICATOR_LED LED_BLUE_L

static uint16_t RFIDValue;
static bool RFIDIndicatorLEDOn;
static uint16_t RFIDIndicatorDelay;

static xTimerHandle RFIDIndicatorTimer;
static void RFIDIndicatorBlink(xTimerHandle time)
{
    //solid light if landed: directly on the landing pad is over 3500
    if (RFIDValue > 3500)
    {
        ledSet(RFID_INDICATOR_LED, true);
    }
    //blinking otherwise
    else
    {
        ledSet(RFID_INDICATOR_LED, RFIDIndicatorLEDOn);
        //flip LED state
        RFIDIndicatorLEDOn ^= 1;
    }

    //Update period of the timer
    xTimerChangePeriod(time, M2T(RFIDIndicatorDelay), 10);
    xTimerStart(time, 50);
}

static xTimerHandle RFIDPollTimer;
static void RFIDPoll(xTimerHandle time)
{
    RFIDValue = analogRead(RFID_INPUT_PIN);
    //DEBUG_PRINT("RFID: %d\n", (int)RFIDValue);

    //The background reading for the RFID sensor is about 200,
    //maximum reading (on top of RFID reader) about 3500
    //strengthProportion should be between 0 and 1
    float strengthProportion = RFIDValue / 4000.0f;
    //The stronger the RFID reading, the shorter the delay.
    //The LED will blink faster the closer to the station the deck is.
    RFIDIndicatorDelay = (uint16_t)(50 / strengthProportion);
}

static void RFIDInit(DeckInfo *info)
{
    //Using INPUT_PULLDOWN will cause the Lighthouse deck to be incompatible,
    //and INPUT_PULLUP could possibly fry the Lighthouse FPGA
    pinMode(RFID_INPUT_PIN, INPUT);
    RFIDIndicatorDelay = 1000;

    RFIDPollTimer = xTimerCreate("RFID Poll", M2T(10), pdTRUE, NULL, RFIDPoll);
    xTimerStart(RFIDPollTimer, 250);
    //pdFALSE: don't loop because the timer has to change its delay
    RFIDIndicatorTimer = xTimerCreate("RFID Indicator", M2T(RFIDIndicatorDelay), pdFALSE, NULL, RFIDIndicatorBlink);
    xTimerStart(RFIDIndicatorTimer, 250);
}

static const DeckDriver RFIDDeck = {
    .vid = 0x17,
    .pid = 0x01,
    .name = "gmrRFID",
    .usedGpio = DECK_USING_TX2 | DECK_USING_RX2,
    .init = RFIDInit,
};

DECK_DRIVER(RFIDDeck);

LOG_GROUP_START(rfid)
LOG_ADD(LOG_UINT16, value, &RFIDValue)
LOG_GROUP_STOP(rfid)