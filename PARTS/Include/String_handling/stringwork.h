/*
    stringwork.h
*/

#ifndef STRINGWORK_H
#define STRINGWORK_H

    int blockt(char* string, unsigned w);
    char* hex_to_char(uint8_t n);
    void hex_string(char *str, uint8_t *data, uint32_t length);
    void print_hexstr_to_block(char* str);
    
#endif