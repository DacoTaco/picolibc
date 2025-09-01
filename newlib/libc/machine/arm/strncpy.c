#ifdef __STARLET__
#include <string.h>
#include <stdint.h>

// strncpy (with 0 padding if the source is shorter than length)
// IOS has a completely custom strncpy to deal with a MEM1 HW bug
// MEM1 accesses of 4 bytes (strb & strh) to MEM1 will thrash 4 bytes
// hence the weird access/seperation of code
char* strncpy(char *dest, const char *src, size_t maxlen)
{
	// if in mem1, work in uint32_t sized chunks
	if((uint32_t)dest < 0x01800000)
	{
		uint32_t index = 0;
		uint32_t destination = (uint32_t)dest;
		while(index < maxlen)
		{
			uint32_t* address = (uint32_t*)(destination & (uint32_t)~0x03);
			uint32_t offset = 24 - (destination & 0x03) * 8;
			uint32_t value = (*address & (uint32_t)~(0xFF << offset)) | ((src[index]) << offset);
			*address = value;
			destination++;
			if(src[index] == '\0')
				break;
			index++;
		}

		//add padding
		while(index < maxlen)
		{
			uint32_t* address = (uint32_t*)(destination & (uint32_t)~0x03);
			uint32_t offset = 24 - ((uint32_t)destination & 0x03) * 8;
			*address = (*address & (uint32_t)~(0xFF << offset));

			destination++;
			index++;
		}
	}
	// otherwise, normal strncpy
	else
	{
		size_t i = 0;
		for(; i < maxlen && src[i] != '\0'; ++i)
		{
			dest[i] = src[i];
		}

		//padding
		for(; i < maxlen; ++i)
		{
			dest[i] = 0;
		}
	}

	return dest;
}
#else
#include "../../string/strncpy.c"
#endif