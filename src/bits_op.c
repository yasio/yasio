#include "bits_op.h"

static const unsigned char bits_wmask_table[8][8] = {
	{0xFE/*11111110*/},
	{0xFD/*11111101*/, 0xFC/*11111100*/},
	{0xFB/*11111011*/, 0xF9/*11111001*/, 0xF8/*11111000*/},
	{0xF7/*11110111*/, 0xF3/*11110011*/, 0xF1/*11110001*/, 0xF0/*11110000*/},
	{0xEF/*11101111*/, 0xE7/*11100111*/, 0xE3/*11100011*/, 0xE1/*11100001*/, 0xE0/*11100000*/},
	{0xDF/*11011111*/, 0xCF/*11001111*/, 0xC7/*11000111*/, 0xC3/*11000011*/, 0xC1/*11000001*/, 0xC0/*11000000*/},
	{0xBF/*10111111*/, 0x9F/*10011111*/, 0x8F/*10001111*/, 0x87/*10000111*/, 0x83/*10000011*/, 0x81/*10000001*/, 0x80/*10000000*/},
	{0x7F/*01111111*/, 0x3F/*00111111*/, 0x1F/*00011111*/, 0x0F/*00001111*/, 0x07/*00000111*/, 0x03/*00000011*/, 0x01/*00000001*/, 0x00/*00000000*/}
};

static const unsigned char bits_rmask_table[8] = {
	0x01/*00000001*/,
	0x03/*00000011*/,
	0x07/*00000111*/,
	0x0F/*00001111*/,
	0x1F/*00011111*/,
	0x3F/*00111111*/,
	0x7F/*01111111*/,
	0xFF/*11111111*/
};

void set_bits_value(unsigned char value, unsigned int bits, unsigned int pos, void* dest)
{
	if(pos > 7 || (pos + 1) < bits || bits > 8)
	{
		return;
	}

	*( (unsigned char*)dest ) = ( 
		( *( (unsigned char*)dest ) & bits_wmask_table[pos][bits - 1] ) 
		| 
		( value << (pos + 1 - bits) )
		);
}

unsigned char get_bits_value(unsigned char source, unsigned int bits, unsigned int pos)
{
	if(pos > 7 || (pos + 1) < bits || bits > 8)
	{
		return 0;
	}
	
	return ( (source & bits_rmask_table[pos]) >> (pos + 1 - bits) );
}

