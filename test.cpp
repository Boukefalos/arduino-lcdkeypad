#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern "C" {
    #include "pb_decode.h"
    #include "pb_encode.h"
    #include "tm1638.pb.h"
}

bool callbackFont(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    uint8_t buffer[256] = {0};
    int length = stream->bytes_left;
    if (length > sizeof(buffer) - 1) {
        return false;
    }
    if (!pb_read(stream, buffer, length)) {
        return false;
    }    
    memset(*arg, 0, sizeof(buffer));
    strncpy((char*) *arg, (char*) buffer, length);
    return true;
}

int main() {
    FILE *fp = fopen("test.buffer", "r");

    int packet_size = fgetc(fp);
    uint8_t in_buffer[256];
    int bytes_read = fread((char*) in_buffer, 1, packet_size, fp);
    if (bytes_read != packet_size) {
        printf("length problem");
        return 1;
    }

    pb_istream_t istream = pb_istream_from_buffer(in_buffer, bytes_read);
    _tm1638_Command command;
    command.setDisplayToString.string.funcs.decode = &callbackFont;
    if (!pb_decode(&istream, tm1638_Command_fields, &command)) {
        printf("decode problem");
        return 1;
    }

    tm1638_SetDisplayToString setDisplayToString = command.setDisplayToString;
    char dots = setDisplayToString.dots;
    char pos = setDisplayToString.dots;
    
    printf("%s\n", setDisplayToString.string.arg);
    return 0;
}