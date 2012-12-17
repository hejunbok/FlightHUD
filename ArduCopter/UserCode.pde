// agmatthews USERHOOKS
#include <SoftwareSerial.h>
#include <FrSkyTelem.h>

SoftwareSerial fSerial(A3, A4, true); //(RX, TX), inverted signaling
#define PIEZO_PIN AN1
uint8_t last_mode = 0, last_armed = 0;
uint32_t userCount = 0;
uint32_t buzz_end_time = 0;


void userhook_init() {
    fSerial.begin(9600);
        
    pinMode(PIEZO_PIN,OUTPUT);
    blocking_beep(); //quick beep
}

void userhook_50Hz() { //every 20ms
    if (userCount == 0) { piezo_beep(10); } //beep when really running
        
    if (userCount % 50 == 0) { //check every 2 seconds
        if (battery_voltage1 < g.low_voltage && battery_voltage1 > (g.low_voltage/2)) {
            piezo_beep(10);
        } else if (g.battery_monitoring == 4 && current_total1 > g.pack_capacity) {
            piezo_beep(10);
        }
                                              
        uint16_t batt = (uint16_t)(battery_voltage1 * 10);
        uint16_t curr = (uint16_t)(current_total1 * 10);
        TelemBatt t = { batt, curr };
        
        sendFrskyPacket(&fSerial, TELEM_BATT, (uint8_t*) &t, sizeof(TelemBatt));
    }
    
    if (last_mode != control_mode || (userCount % 50 == 0)) { //1 seconds
        TelemMode p = { (motors.armed() & 0xFF), (control_mode & 0xFF) };
        sendFrskyPacket(&fSerial, TELEM_MODE, (uint8_t*) &p, sizeof(TelemMode));
    }
    
    if (userCount % 25  == 0) { //1/2 second
        int16_t altitude = (current_loc.alt - home.alt); //cm above round
        int16_t home = home_distance; //cm
        TelemPosition t = { g_gps->num_sats & 0xFF, altitude, home };
        sendFrskyPacket(&fSerial, TELEM_POS, (uint8_t*) &t, sizeof(TelemPosition));
    }
    
    /*ahrs.roll,
    ahrs.pitch,
    ahrs.yaw,
    omega.x,
    omega.y,
    omega.z);
    current_loc.lat,                // in 1E7 degrees
    current_loc.lng,                // in 1E7 degrees
    g_gps->altitude * 10,             // millimeters above sea level
    (current_loc.alt - home.alt) * 10,           // millimeters above ground
    g_gps->ground_speed * rot.a.x,  // X speed cm/s
    g_gps->ground_speed * rot.b.x,  // Y speed cm/s
    g_gps->ground_speed * rot.c.x,
    home_distance // in cm
    g_gps->num_sats*/
    
    if (last_mode != control_mode) { beep(); }
    if (last_armed != motors.armed()) { blocking_beep(); }
    
    //end async buzzer
    if (userCount > buzz_end_time) { digitalWrite(PIEZO_PIN,HIGH); }
    userCount++;
    last_armed = motors.armed();
    last_mode = control_mode;
}


void blocking_beep() {
    digitalWrite(PIEZO_PIN,LOW);
    delay(100);
    digitalWrite(PIEZO_PIN,HIGH);
}
void beep() { piezo_beep(5); }
void piezo_beep(uint8_t t) {
    digitalWrite(PIEZO_PIN,LOW);
    buzz_end_time = userCount + 5; //100ms
}


