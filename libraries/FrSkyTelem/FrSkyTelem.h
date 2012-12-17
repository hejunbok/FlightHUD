/*
 * Blah.
 */

#ifndef _FRSKYTELEM_H_
#define _FRSKYTELEM_H_ 
    
#include <stdio.h>
#include <inttypes.h>
#include <Stream.h>

#define TELEM_POS 0xDE
typedef struct {
    uint8_t sats;
    int16_t altitude;
    int16_t dist_home;
} TelemPosition;

#define TELEM_BATT 0xAD
typedef struct {
    uint16_t voltage;
    uint16_t mah;
} TelemBatt;

#define TELEM_MODE 0xFE
typedef struct {
    uint8_t armed;
    uint8_t mode;
} TelemMode;

void sendFrskyPacket(Stream* s, uint8_t type, uint8_t* data, uint8_t len);
uint8_t* isValidFrskyPacket(uint8_t* data, uint8_t len);
void setDebug(Stream* s);

#endif 
