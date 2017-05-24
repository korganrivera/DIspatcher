/*
	A way to determine the host machine's endian-ness,
	using a union instead of preprocessor macros.
*/
#include <stdio.h>
#include <stdint.h>

int main(void){
	int16_t n = 1;
	puts(*((int8_t*)&n) ? "Little Endian." : "Big Endian.");	
}


