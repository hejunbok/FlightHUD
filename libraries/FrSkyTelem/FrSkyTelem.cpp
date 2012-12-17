
#include "FrSkyTelem.h"
#include <Stream.h>

Stream* debug;

void sendFrskyPacket(Stream* s, uint8_t type, uint8_t* data, uint8_t len) {
    s->write(0x93);
    s->write(type);
    s->write(len);
    uint8_t chk = type; chk += len;
    for (uint8_t i = 0; i < len; i++) {
        chk += data[i];
        s->write(data[i]);
    }
    s->write(~chk);
}


uint8_t* isValidFrskyPacket(uint8_t* data, uint8_t len) {
    for (uint8_t i=0; i<(len - 3); i++) {
        if (data[i] == 0x93) { //do we have the start byte?
            uint8_t* pkt = &data[i];
            //0=start, 1=type, 2=len, 3-len=data, 3+len=chk
            uint8_t pkt_data_len = pkt[2];
            if ((pkt_data_len + i + 3) <= len) { //do we have enough recieved data?
                uint8_t chk = pkt[1];
                chk += pkt[2];
                for (uint8_t p=0; p < pkt_data_len; p++) {
                    chk += pkt[3+p];
                }
                if (! (((~chk) & 0xFF) - pkt[3 + pkt_data_len])) {
                    //if (debug) { debug->println("chk PASSED"); }
                    return pkt; //return type
                }
            }
        }
    
    }
    return NULL; //no success.
}

void setDebug(Stream* s) {
    debug = s;
}