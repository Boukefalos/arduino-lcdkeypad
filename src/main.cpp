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

int pong = 0;
String text;
byte pressed = 0;

byte values[256];
uint8_t font[256];

void sendMessage(_tm1638_Message message) {
    uint8_t out_buffer[256];
    pb_ostream_t ostream = pb_ostream_from_buffer(out_buffer, sizeof(out_buffer));
    if (pb_encode_delimited(&ostream, tm1638_Message_fields, &message)) {
        Serial.write(out_buffer, ostream.bytes_written);
        Serial.flush();
    }
}

void sendPong(int id) {
    tm1638_Pong pong = {id};
    tm1638_Message message = tm1638_Message_init_default;
    message.type = tm1638_Message_Type_PONG;
    message.pong = pong;    
    message.has_pong = true;
    sendMessage(message);
}

void sendText(char buffer[]) {
    tm1638_Text text = tm1638_Text_init_default;
    tm1638_Message message = tm1638_Message_init_default;
    message.type = tm1638_Message_Type_TEXT;
    strcpy(message.text.text, buffer);
    message.has_text = true;
    sendMessage(message);
}

void sendText(String string) {
    char buffer[40];
    string.toCharArray(buffer, sizeof(buffer));
    sendText(buffer);
}

void sendButtons(byte pressed) {
    tm1638_Buttons buttons = {pressed};
    tm1638_Message message = tm1638_Message_init_default;
    message.type = tm1638_Message_Type_BUTTONS;
    message.buttons = buttons;
    message.has_buttons = true;
    sendMessage(message);
}

byte parseColor(tm1638_Color color) {
    switch (color) {
        case tm1638_Color_GREEN:
            return TM1638_COLOR_GREEN;
        case tm1638_Color_RED:
            return TM1638_COLOR_RED;
        case tm1638_Color_BOTH:
            return TM1638_COLOR_GREEN + TM1638_COLOR_RED;
        case tm1638_Color_NONE:
            break;
    }
    return TM1638_COLOR_NONE;    
}

bool callbackValues(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    memset(values, 0, sizeof(values));
    if (stream->bytes_left > sizeof(values) - 1) {
        return false;
    }
    if (!pb_read(stream, values, stream->bytes_left)) {
        return false;
    }
    return true;
}

bool callbackFont(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    memset(font, 0, sizeof(font));
    if (stream->bytes_left > sizeof(font) - 1) {
        return false;
    }
    if (!pb_read(stream, font, stream->bytes_left)) {
        return false;
    }
    return true;
}

void setup() {
    tm1638[0] = new TM1638(8, 9, 7);
    tm1638[0]->setupDisplay(true, 1);
    tm1638[0]->setDisplayToString("Ready", 0, 0);
    // tm1638[0]->setDisplayToError();

    Serial.begin(4800);
    Serial.println("");
    //sendText("setup()");
    Serial.println("setup()");
}

void loop() {
    /*if (pong > 0) {        
        sendPong(pong);
        pong = 0;
    }
    if (text.length() > 0) {
        sendText(text);
        text = "";
    }
    byte buttons = tm1638[0]->getButtons();
    if (buttons != pressed) {
        sendButtons(buttons);
        pressed = buttons;
    }*/
}

void serialEvent() {
    if (Serial.available()) {
        int packet_size = Serial.read();
        uint8_t in_buffer[256];
        int bytes_read = Serial.readBytes((char*) in_buffer, packet_size);
        if (bytes_read != packet_size) {
            Serial.println("length problem");
            return;
        }
        pb_istream_t istream = pb_istream_from_buffer(in_buffer, bytes_read);
        _tm1638_Command command;
        if (!pb_decode(&istream, tm1638_Command_fields, &command)) {
            Serial.println("decode problem");
            return;
        }

        // tm1638[0]->setDisplayDigit(command.type / 10, 0, false);
        // tm1638[0]->setDisplayDigit(command.type, 1, false);
        Serial.println("command");
        Serial.println(command.type);
        switch (command.type) {
            case tm1638_Command_Type_PING:
                if (command.has_ping) {
                    tm1638_Ping ping = command.ping;
                    tm1638[0]->setDisplayToDecNumber(ping.id, 0, false);
                    pong = ping.id;
                    }
                break;
            case tm1638_Command_Type_CONSTRUCT:
                    if (command.has_construct) {
                        tm1638_Construct construct = command.construct;
                        switch (construct.module) {
                            case tm1638_Module_TM1638:
                                tm1638[0] = new TM1638((byte) construct.dataPin, (byte) construct.clockPin, (byte) construct.strobePin);
                                break;
                            case tm1638_Module_InvertedTM1638:
                                tm1638[0] = new InvertedTM1638((byte) construct.dataPin, (byte) construct.clockPin, (byte) construct.strobePin);
                                break;
                            case tm1638_Module_TM1640:
                                tm1640 = new TM1640((byte) construct.dataPin, (byte) construct.clockPin);
                                break;
                    }
                }
                break;
            case tm1638_Command_Type_CLEAR_DISPLAY:
                tm1638[0]->clearDisplay();
                break;
            case tm1638_Command_Type_CLEAR_DISPLAY_DIGIT:
                if (command.has_clearDisplayDigit) {
                    tm1638_ClearDisplayDigit clearDisplayDigit = command.clearDisplayDigit;
                    tm1638[0]->clearDisplayDigit(clearDisplayDigit.pos, clearDisplayDigit.dot);
                }
                break;
            case tm1638_Command_Type_SET_DISPLAY:
                if (command.has_setDisplay) {
                    tm1638_SetDisplay setDisplay = command.setDisplay;
                    setDisplay.values.funcs.decode = &callbackValues;
                    if (pb_decode(&istream, tm1638_SetDisplay_fields, &setDisplay)) {
                        tm1638[0]->setDisplay(values);
                    }
                }
                break;
            case tm1638_Command_Type_SET_DISPLAY_DIGIT:
                if (command.has_setDisplayDigit) {
                    tm1638_SetDisplayDigit setDisplayDigit = command.setDisplayDigit;
                    byte digit = setDisplayDigit.digit;
                    byte pos = setDisplayDigit.pos;
                    bool dot = setDisplayDigit.dot;
                    bool has_font = setDisplayDigit.has_font;
                    if (has_font) {
                        setDisplayDigit.font.funcs.decode = &callbackFont;
                        if (pb_decode(&istream, tm1638_SetDisplayDigit_fields, &setDisplayDigit)) {
                            tm1638[0]->setDisplayDigit(digit, pos, dot, font);
                        }
                    } else {
                        tm1638[0]->setDisplayDigit(digit, pos, dot);
                    }
                }
                break;
            case tm1638_Command_Type_SET_DISPLAY_TO_BIN_NUMBER:
                if (command.has_setDisplayToNumber) {
                    tm1638_SetDisplayToNumber setDisplayToNumber = command.setDisplayToNumber;
                    byte number = setDisplayToNumber.number;
                    byte dots = setDisplayToNumber.dots;
                    bool has_font = setDisplayToNumber.has_font;
                    if (has_font) {
                        setDisplayToNumber.font.funcs.decode = &callbackFont;
                        if (pb_decode(&istream, tm1638_SetDisplayToNumber_fields, &setDisplayToNumber)) {
                            tm1638[0]->setDisplayToBinNumber(number, dots, font);
                        }
                    } else {
                        tm1638[0]->setDisplayToBinNumber(number, dots);
                    }
                }
                break;
            case tm1638_Command_Type_SET_DISPLAY_TO_DEC_NUMBER:
                if (command.has_setDisplayToNumber) {
                    tm1638_SetDisplayToNumber setDisplayToNumber = command.setDisplayToNumber;
                    signed int number = setDisplayToNumber.number;
                    byte dots = setDisplayToNumber.dots;
                    bool leadingZeros = setDisplayToNumber.leadingZeros;
                    bool has_font = setDisplayToNumber.has_font;
                    if (has_font) {
                        setDisplayToNumber.font.funcs.decode = &callbackFont;
                        if (pb_decode(&istream, tm1638_SetDisplayToNumber_fields, &setDisplayToNumber)) {
                            if (number < 0) {
                                tm1638[0]->setDisplayToSignedDecNumber(number, dots, leadingZeros, font);
                            } else {
                                tm1638[0]->setDisplayToDecNumber(number, dots, leadingZeros, font);
                            }
                        }
                    } else {
                        if (number < 0) {
                            tm1638[0]->setDisplayToSignedDecNumber(number, dots, leadingZeros);
                        } else {
                            tm1638[0]->setDisplayToDecNumber(number, dots, leadingZeros);
                        }
                    }
                }
                break;
            case tm1638_Command_Type_SET_DISPLAY_TO_HEX_NUMBER:
                if (command.has_setDisplayToNumber) {
                    tm1638_SetDisplayToNumber setDisplayToNumber = command.setDisplayToNumber;
                    signed int number = setDisplayToNumber.number;
                    byte dots = setDisplayToNumber.dots;
                    bool leadingZeros = setDisplayToNumber.leadingZeros;
                    tm1638[0]->setDisplayToHexNumber(number, dots, leadingZeros);
                }
                break;
            case tm1638_Command_Type_SET_DISPLAY_TO_ERROR:
                tm1638[0]->setDisplayToError();
                break;
            case tm1638_Command_Type_SET_DISPLAY_TO_STRING:
                if (command.has_setDisplayToString) {
                    text = "SET_DISPLAY_TO_STRING";
                    tm1638_SetDisplayToString setDisplayToString = command.setDisplayToString;
                    pb_decode(&istream, tm1638_SetDisplayToString_fields, &setDisplayToString);
                    byte dots = setDisplayToString.dots;
                    byte pos = setDisplayToString.dots;
                    tm1638[0]->setDisplayToString(setDisplayToString.string, dots, pos);                  
                    bool has_font = setDisplayToString.has_font;
                    if (has_font) {
                        setDisplayToString.font.funcs.decode = &callbackFont;
                        if (pb_decode(&istream, tm1638_SetDisplayToString_fields, &setDisplayToString)) {
                            tm1638[0]->setDisplayToString(setDisplayToString.string, dots, pos, font);
                        }
                    } else { // setDisplayToString.string
                        tm1638[0]->setDisplayToString("test...", dots, pos);
                    }
                }
                break;
            case tm1638_Command_Type_SET_LED:
                if (command.has_setLED) {
                    tm1638_SetLED setLED = command.setLED;
                    byte color = parseColor(setLED.color);                    
                    tm1638[0]->setLED(color, setLED.pos);
                }
                break;
            case tm1638_Command_Type_SET_LEDS:
                if (command.has_setLEDs) {
                    tm1638_SetLEDs setLEDs = command.setLEDs;
                    tm1638[0]->setLEDs(setLEDs.led);
                }
                break;
            case tm1638_Command_Type_SETUP_DISPLAY:
                if (command.has_setupDisplay) {
                    tm1638_SetupDisplay setupDisplay = command.setupDisplay;
                    tm1638[0]->setupDisplay(setupDisplay.active, setupDisplay.intensity);
                }
                break;
        }
    }
}