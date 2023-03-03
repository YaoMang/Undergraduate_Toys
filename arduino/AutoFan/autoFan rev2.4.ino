#include <DHT.h>

#include <LiquidCrystal.h>

#include <TXOnlySerial.h>


#define ENDCODE 0xFF

// PIN SET

    // Display

    const uint8_t pin_led_RS = PIN3;
    const uint8_t pin_led_EN = PIN4;
    const uint8_t pin_led_D4 = PIN5;
    const uint8_t pin_led_D5 = PIN6;
    const uint8_t pin_led_D6 = PIN7;
    const uint8_t pin_led_D7 = 8;

    // Functional

    const uint8_t pin_temp = PIN_A0;
    const uint8_t pin_fan = 9;
    const uint8_t pin_btn = PIN2;
    const uint8_t pin_spl = 10;
    const uint8_t pin_dht = PIN_A2;
    const uint8_t pin_txd = 11;

    // Serial
    TXOnlySerial TX_Serial = TXOnlySerial(pin_txd);

// Global Variables

    // FLASH

    const char speed_hi_S[] PROGMEM = "Hi";
    const char speed_lo_S[] PROGMEM = "Lo";
    const char speed_of_S[] PROGMEM = "Of";
    
    // Temp array, only for initalization of str_dis
    const char speed_in_S[] PROGMEM = "";

char speedDisplayStr[3] = {0};
const char* strPtr = speed_in_S;
const char* strPtr_last;

LiquidCrystal LM016l(
    pin_led_RS, 
    pin_led_EN, 
    pin_led_D4, 
    pin_led_D5,
    pin_led_D6,
    pin_led_D7
    );

DHT dht(pin_dht, DHT11);

constexpr const float convertArg = 150.0/1024.0;

int temp = 0;
int fanspeed = 0;
float humi = 0;

int serialInfo = 1;

uint8_t isApplied = 0;
uint8_t isActive = 1;
uint8_t isSplash = 0;
uint8_t isSent = 0;

unsigned long int timestamp_lastDisplay = 0;
unsigned long int timestamp_lastRead = 0;
unsigned long int timestamp_lastSplash = 0;
unsigned long int timestamp_lastSent = 0;

const int timeInterval_ReadData = 200;
const int timeInterval_Display = 2000;
const int timePeriod_Splash = 10000;
const int timeInterval_Sent = 200;

// Debug Function
/*
 *void SerialSent(int temp, int fanspeed)
 *{ // Neuro-Sama is watching you
 *    Serial.print("Temp:");
 *    Serial.print(temp);
 *    Serial.print("\nFanSpeed:");
 *    Serial.print(fanspeed);
 *}
 */

// Functional Models

void setFanSpeed()
{
    if(isApplied){
        return;
    }

    // set fanspeed, corresponding with $temp
    if(temp > 30){
        fanspeed = 100;
        strPtr = speed_hi_S;
    }
    else if(temp >= 20){
        fanspeed = 50;
        strPtr = speed_lo_S;
    }
    else{
        fanspeed = 0;
        strPtr = speed_of_S;
    }
    analogWrite(pin_fan, fanspeed * 255.0 / 100.0);

    if(strPtr_last != strPtr){
        strcpy_P(speedDisplayStr, strPtr);
        strPtr_last = strPtr;
    }

    // -tok
    isApplied = 1;
    return;
}

void display()
{
    // Set reading intervals and detect mills overflow
    if(millis() - timestamp_lastDisplay >= timeInterval_Display || timestamp_lastDisplay >= millis()){
        // Clear last message
        LM016l.clear();
        // New message print
        LM016l.print("T:");
        LM016l.print(temp);
        LM016l.print("C\t");
        LM016l.print("H:");
        LM016l.print(humi);
        LM016l.print("%");
        LM016l.setCursor(0, 1);
        LM016l.print("Fan:");
        LM016l.print(speedDisplayStr);
        // wating all the display complete
        delay(80);
        // update timestamp
        timestamp_lastDisplay = millis() - 1;
    }
    return;
}

void humifetch()
{
    do{
    humi = dht.readHumidity();
    }while (isnan(humi));
}

void temperatureFetch()
{
    // Set reading intervals and detect mills overflow
    if(millis() - timestamp_lastRead >= timeInterval_ReadData || timestamp_lastRead >= millis()){
        // read and process data from thermal sensor
        int rawData = analogRead(pin_temp);
        float Temp = rawData * convertArg;
        temp = (int)(Temp + 0.5);
        // read humi
        humifetch();
        //Serial.println(humi);
        // update timestamp
        timestamp_lastRead = millis() - 1;
        // tik-
        isApplied = 0;
    }
    return;
}

void serialControl()
{
    if(Serial.available() > 0){
        serialInfo = Serial.read();
        //Serial.println(serialInfo, HEX);

        if(serialInfo == 1){
            isActive = 1;
            LM016l.clear();
            LM016l.print(F("Starting"));
            //Serial.println(F("Starting"));
        }
        if(serialInfo == 0){
            isActive = 0;
            LM016l.clear();
            LM016l.print(F("Stopped"));
            //Serial.println(F("Stopped"));
            analogWrite(pin_fan, 0 * 255.0 / 100.0);
        }
    }
}


// ISR PORT
void _ISR_PORT_01_()
{   
    /*
    if(digitalRead(pin_btn) == LOW){
        delayMicroseconds(16);
        if(digitalRead(pin_btn) == LOW){
            isSplash = 1;
            timestamp_lastSplash = millis();
        }
    }
    */

    //isSplash = 1;
    //timestamp_lastSplash = millis();
}

void colletBtn()
{
    if(digitalRead(pin_btn) == HIGH){
        delay(16);
        if(digitalRead(pin_btn) == HIGH){
            isSplash = 1;
            timestamp_lastSplash = millis() - 1;
        }
    }
}

void splash()
{
    if(isSplash){
        analogWrite(pin_spl, 255);
    }
    else{
        analogWrite(pin_spl, 0);
    }
}

void timer(unsigned long int& last, const unsigned long int& interval, uint8_t& target)
{
    if(millis() - last >= interval || last >= millis()){
        target = 0;
    }
}

void serialSent()
{
    if(!isSent){
        /*
        uint32_t messageSent = 0;
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&messageSent);
        *ptr = static_cast<uint8_t>(temp);
        ++ptr;
        *ptr = static_cast<uint8_t>(humi + 0.5);
        ++ptr;
        *ptr = static_cast<uint8_t>(fanspeed);
        ++ptr;
        *ptr = static_cast<uint8_t>(isActive);
        Serial.write(messageSent);
        */

        // UPPER

        Serial.write(static_cast<uint8_t>(temp));
        Serial.write(static_cast<uint8_t>(humi + 0.5));
        Serial.write(static_cast<uint8_t>(fanspeed));
        Serial.write(static_cast<uint8_t>(isActive));
        Serial.write(static_cast<uint8_t>(ENDCODE));

        // CONTROLLER
        TX_Serial.write(static_cast<uint8_t>(fanspeed));
        TX_Serial.write(static_cast<uint8_t>(isSplash));
        TX_Serial.write(static_cast<uint8_t>(ENDCODE));
    }
}

void patch()
{
    colletBtn();
    timer(timestamp_lastSplash, timePeriod_Splash, isSplash);
    splash();
    timer(timestamp_lastSent, timeInterval_Sent, isSent);
    serialSent();
}

void setup() 
{
    // Serial settings, for HOST computer
    Serial.begin(9600);
    TX_Serial.begin(9600);

    // using LM016l as LCD display
    LM016l.begin(16, 2);

    // button pin & SWITCH
    pinMode(pin_btn, INPUT);
    //attachInterrupt(digitalPinToInterrupt(pin_btn), _ISR_PORT_01_, CHANGE);
    pinMode(pin_spl, OUTPUT);

    // fanspeed ctrl pin initialize
    pinMode(pin_fan, OUTPUT);

    // Humi
    dht.begin();

    // setting analog reference voltage mode, SET TO 1500mV
    analogReference(EXTERNAL);
}

void loop() 
{
    serialControl();
    if(isActive){
        temperatureFetch();
        setFanSpeed();
        display();
    }
    patch();
}