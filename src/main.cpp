#include <stdint.h>
#include "Arduino.h"

#include <TM1638.h>
#include <InvertedTM1638.h>
#include <TM1640.h>

extern "C" {
    #include "pb_decode.h"
    #include "pb_encode.h"
    #include "tm1638.pb.h"
}

TM1640 *tm1640;
TM1638 *tm1638[6];
TM16XX *module;

int t = 123;
int id = 0;
byte pressed = 0;
_tm1638_Construct construct;

void setup() {
    Serial.begin(9600);
    tm1638[0] = new TM1638(8, 9, 7);
    tm1638[0]->setupDisplay(true, 1);
    tm1638[0]->setLED(TM1638_COLOR_RED, 2);
    tm1638[0]->setDisplayToDecNumber(111, 0);
}

void sendButtons(int id, byte buttons) {
    /*_tm1638_Buttons x = {id, buttons};
    uint8_t out_buffer[256];
    pb_ostream_t ostream = pb_ostream_from_buffer(out_buffer, sizeof(out_buffer));
    if (pb_encode_delimited(&ostream, tm1638_Buttons_fields, &x)) {
        Serial.write(out_buffer, ostream.bytes_written);
    }*/
}

void sendEcho(int id, char message[]) {
    _tm1638_Echo echo = {id, true};
    strncpy(echo.message, message, 40);
    uint8_t out_buffer[256];
    pb_ostream_t ostream = pb_ostream_from_buffer(out_buffer, sizeof(out_buffer));
    if (pb_encode_delimited(&ostream, tm1638_Echo_fields, &echo)) {
        Serial.write(out_buffer, ostream.bytes_written);
    }
}

void loop() {
    byte buttons = tm1638[0]->getButtons();
    if (buttons != pressed) {
        //sendButtons(0, buttons);
        sendEcho(buttons, "Buttons!");
        pressed = buttons;
    }
}

void serialEvent() {
    if (Serial.available()) {
        int packet_size = Serial.read();
        uint8_t in_buffer[256];
        int bytes_read = Serial.readBytes((char*) in_buffer, packet_size);
        if (bytes_read != packet_size) {
            return;
        }
        pb_istream_t istream = pb_istream_from_buffer(in_buffer, bytes_read);
        _tm1638_Command command;
        if (!pb_decode(&istream, tm1638_Command_fields, &command)) {
            return;
        }

        switch (command.type) {
            case tm1638_Command_Type_PING:
                if (command.has_ping) {
                    _tm1638_Ping ping = command.ping;
                    sendEcho(ping.id, "Pong");
                    tm1638[0]->setDisplayToDecNumber(ping.id, 0);
                }
                break;
            case tm1638_Command_Type_CONSTRUCT:
                construct = command.construct;
                id = construct.id;
                switch (construct.module) {
                    case tm1638_Module_TM1638:
                        tm1638[0] = new TM1638((byte) construct.dataPin, (byte) construct.clockPin, (byte) construct.strobePin, construct.activateDisplay, (byte) construct.intensity);
                        tm1638[0]->setupDisplay(true, 1);
                        break;
                    case tm1638_Module_InvertedTM1638:
                        tm1638[0] = new InvertedTM1638((byte) construct.dataPin, (byte) construct.clockPin, (byte) construct.strobePin, construct.activateDisplay, (byte) construct.intensity);
                        break;
                    case tm1638_Module_TM1640:
                        tm1640 = new TM1640((byte) construct.dataPin, (byte) construct.clockPin, construct.activateDisplay, (byte) construct.intensity);
                        break;
                }
                break;
            case tm1638_Command_Type_SET_LED:
                if (command.has_setLed) {
                    _tm1638_SetLed setLed = command.setLed;
                    byte color;
                    switch (setLed.color) {
                        case tm1638_Color_GREEN:
                            color = TM1638_COLOR_GREEN;
                            break;
                        case tm1638_Color_RED:
                            color = TM1638_COLOR_RED;
                            break;
                        case tm1638_Color_BOTH:
                            color = TM1638_COLOR_GREEN + TM1638_COLOR_RED;
                            break;
                        case tm1638_Color_NONE:
                            color = TM1638_COLOR_NONE;
                            break;
                    }
                    tm1638[0]->setLED(color, setLed.pos);
                }
                break;         
        }
    }
}