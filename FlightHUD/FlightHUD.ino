
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <FrSkyTelem.h>
#include <LCDBitmap.h>
#include <ctype.h>

// LCD pins: RS  RW  EN  D4  D5  D6  D7
LiquidCrystal lcd(38, 21, 22, 23, 24, 25, 26);
LCDBitmap bitmap(&lcd, 3, 0);
SoftwareSerial fSerial(20, 4, true); //(RX, TX), inverted signaling


void setup() {
    lcd.begin(16,2);
    bitmap.begin();  // Initalize the LCD bitmap
    bitmap.home();  // Move cursor the home position (0,0)

    Serial.begin(9600);
    fSerial.begin(9600);
    fSerial.listen();
  
    bitmap.clear();  // Clear the display

    updateLcd();
}

uint8_t rssi = 0, last_rssi = 0;
uint8_t loaded_pos=0, loaded_mode=0, loaded_batt=0;
TelemPosition telem_pos = { 100, 1023, 1023 }, last_pos;
TelemMode telem_mode = { 3, 100 }, last_mode;
TelemBatt telem_batt = { 3, 102 }, last_batt;

void loop() {
    static uint8_t frsky_buff[30];
    
    if (Serial.available()) {
        while (Serial.available() > 0) {
            fSerial.write(Serial.read());
        }
    } 
    if (fSerial.available()) {
        
        while (fSerial.available() > 0) {
            if (incomingFrskyByte( fSerial.read(), frsky_buff, sizeof(frsky_buff))) {
                handleFrskyPacket(frsky_buff);
            }
        }
    }
    delay(200);
    updateLcd();
}

void updateLcd() {
    static char* modes[] = { "STA", "ACR", "ALT", "AUT", "GUI", "LOI", "RTL", "CIR", "POS", "LND" };
    
    //top left -> flight mode
    if (loaded_mode && telem_mode.mode != last_mode.mode) {
        lcd.setCursor(0, 0);
        char* mode_s = modes[(telem_mode.mode <= 9)? telem_mode.mode : 9];
        if (! telem_mode.armed) {
            for (uint8_t i = 0; mode_s[i]; i++) { lcd.write( tolower(mode_s[ i ]) ); }
        } else { lcd.print(mode_s); }
    }
    //bottom left
    if (loaded_batt && telem_batt.voltage != last_batt.voltage) {
        lcd.setCursor(0, 1);
        lcd.print(telem_batt.voltage);
        uint8_t batt[] = { 0b00011000, 0b01111110, 0b11000011, 0b10000001, 0b10000101, 0b10001101, 0b10001101, 0b10011001, 0b10111001, 0b10110001, 0b10110001, 0b10100001, 0b10000001, 0b10000001, 0b11111111, 0b01111110 };
        for (uint8_t i=0; i < 16; i++)
            for (uint8_t j=0; j < 8; j++)
                bitmap.pixel(11+j, i, ((batt[i]>>(7-j))&1), NO_UPDATE);
        uint8_t y1 = constrain(map(telem_batt.voltage, 120, 100, 0, BITMAP_H-2), 0, BITMAP_H-2);
        if (y1 > 0) { bitmap.rectFill(11, 0, BITMAP_W-1, y1, OFF, NO_UPDATE); }
        bitmap.update();
    }
    if (rssi != last_rssi) {
        uint8_t y1 = constrain(map(rssi, 40, 80, BITMAP_H-1, 0), 0, BITMAP_H-1);
        if (y1 > 0) { bitmap.rectFill(1, 0, 6, y1, OFF, NO_UPDATE); }
        bitmap.rectFill(1, y1, 6, BITMAP_H-1, ON, NO_UPDATE);
        if (y1 < 12) {
            uint8_t db[5] = { 0b00001000,0b00011000,0b00111000,0b01111000 };
            for (uint8_t i=0; i < 4; i++)
                for (uint8_t j=0; j < 6; j++)
                    bitmap.pixel(j+1, i+11, !((db[i]>>(7-j))&1), NO_UPDATE);
        }
        bitmap.update();
    }
    if (loaded_pos && telem_pos.altitude != last_pos.altitude) {
        lcd.setCursor(7, 0);
        lcd.print(telem_pos.altitude/10,DEC); 
        lcd.print("m alti  ");
        lcd.setCursor(7, 1);
        if (telem_pos.sats > 0) {
            lcd.print(telem_pos.dist_home/10,DEC);
            lcd.write("m away  ");
        } else lcd.write("no gps  ");
    }
    
    /*lcd.print(telem_batt.mah / 10.0f); lcd.print("mah");
        //}
    //}*/
    last_rssi = rssi;
}

void handleFrskyPacket(uint8_t* buffer) {
    static uint32_t userbuffend_p = 0;
    static uint8_t userbuff[50];
    

    if (buffer[0] == 0xfe) {  // A1/A2/RSSI values
        rssi = buffer[3];
        //barg.drawValue( rssiGraph, 100);
    } 
    else if (buffer[0] == 0xfd) { // User Data packet
        
        for (int i=0; i < buffer[1]; i++) {
            userbuff[ userbuffend_p ] = buffer[3+i];
            
            uint8_t* pkt = isValidFrskyPacket(userbuff, userbuffend_p);
            if (pkt != NULL) {
                dealWithData(pkt);
                userbuffend_p = 0;
            }
            userbuffend_p++;
            if (userbuffend_p > sizeof(userbuff)) {
                Serial.println("buffer full, resetting.");
                userbuffend_p = 0;
            }
        }
        //dealWithUserData(&buffer[3], buffer[1]);
    }
}

void dealWithData(uint8_t* buffer) {
    if (buffer[1] == TELEM_POS) {
        TelemPosition* t = (TelemPosition*) &buffer[3];
        last_pos = telem_pos;
        telem_pos = *t;
        loaded_pos = 1; 
    } else if (buffer[1] == TELEM_BATT) {
        TelemBatt* t = (TelemBatt*) &buffer[3];
        last_batt = telem_batt;
        telem_batt = *t;
        loaded_batt = 1;
    } else if (buffer[1] == TELEM_MODE) {
        TelemMode* t = (TelemMode*) &buffer[3];
        last_mode = telem_mode;
        telem_mode = *t;
        loaded_mode = 1;
    }
    updateLcd();
}


#define userDataIdle	0
#define userDataStart	1
#define userDataInFrame 2
#define userDataXOR		3

uint8_t incomingFrskyByte(uint8_t c, uint8_t* buffer, uint16_t maxlen) {
    //returns 1 on successful completion
    static int numPktBytes = 0;
    static uint8_t dataState = userDataIdle;
    
    switch (dataState) {

        case userDataStart:
            if (c == 0x7e) break; // Remain in userDataStart if possible 0x7e,0x7e doublet found. 
            buffer[numPktBytes++] = c;
            dataState = userDataInFrame;
            break;
            
        case userDataInFrame:
            if (c == 0x7d) { 
                dataState = userDataXOR; // XOR next byte
                break; 
            }
            if (c == 0x7e) { // end of frame detected
                dataState = userDataIdle;
                return 1;
                break;
            }
            buffer[numPktBytes++] = c;
            break;
            
        case userDataXOR:
            buffer[numPktBytes++] = c ^ 0x20;
            dataState = userDataInFrame;
            break;

        case userDataIdle:
            if (c == 0x7e) {
                numPktBytes = 0;
                dataState = userDataStart;
            }
            break;
    } // switch
    
    if (numPktBytes > maxlen) {
        numPktBytes = 0;
        dataState = userDataIdle;
    }
    return 0;
}












