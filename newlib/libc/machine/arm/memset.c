/*
 * Copyright (c) 2015 ARM Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <picolibc.h>

#include <stddef.h>

#ifdef __STARLET__
#include <stdint.h>

void set_memory(void* dest, const unsigned char data, size_t len);
__attribute__((target("arm")))
void set_memory_short(void* dest, const unsigned char c, size_t len);
void *memset(void *dest, int character, size_t length);

void set_memory(void* dest, const unsigned char data, size_t len)
{
	if (dest < (void *)0x1800000) 
	{
		for(; len != 0; len--)
		{
			uint32_t* address = (uint32_t*)((uint32_t)dest & (uint32_t)~0x03);
			uint32_t offset = 24 - ((uint32_t)dest & 0x03) * 8;
			*address = (*address & (uint32_t)~(0xFF << offset)) | (data << offset);;
			dest++;
		}
	}
	else 
	{
		for(; len != 0; len--)
		{
			*(uint8_t*)dest = data;
			dest++;
		}
  	}
}

//this is more like a regular memset, but only used at the end of memset..
__attribute__((target("arm")))
void set_memory_short(void* dest, const unsigned char c, size_t len)
{
	uint32_t data = c | (c << 8) | (uint32_t)(c << 16) | (uint32_t)(c << 24);
	register uint32_t data1 __asm("r3") = data;
	register uint32_t data2 __asm("r4") = data;
	register uint32_t data3 __asm("r5") = data;
	uint32_t* address = dest;
	for(uint32_t index = len & 0xFFFFFFF0; index != 0; index -= 0x10)
	{
		__asm__ volatile ("stmia	%[address]!, {%[data],%[data1],%[data2],%[data3]}" 
			: 
			: [address] "r" (address), [data] "r" (data), [data1] "r" (data1), [data2] "r" (data2), [data3] "r" (data3));
	}

	for(uint32_t index = len & 0x0F; index != 0; index -= 4)
	{
		*address = data;
		address++;
	}
}
// memset
// IOS has a completely custom memset to deal with a MEM1 HW bug
// MEM1 accesses of 4 bytes (strb & strh) to MEM1 will thrash 4 bytes
// hence the weird access/seperation of code
void *memset(void *dest, int character, size_t length)
{
	if (length == 0)
		return dest;

	void* destination = dest;
	const uint8_t data = (uint8_t)character & 0xff;
	uint32_t cnt = 0;

	//if destination isn't 4 byte aligned, do a seperate memset until we are aligned 
	if(((uint32_t)dest & 3) != 0)
	{
		cnt = 4 - ((uint32_t)dest & 3);
		if(length <= cnt)
		{
			set_memory(dest, data, length);
			return dest;
		}

		set_memory(dest, data, cnt);
		length -= cnt;
		destination = (void*)((uint32_t)dest & 0xFFFFFFFC) + 4;
	}
	
	//align destination to 16 bytes
	if (length < 0x101 || ((uint32_t)destination & 0xf) == 0)
		cnt = length & 0xFFFFFFFC;
	else
	{
		cnt = 0x10 - ((uint32_t)destination & 0x0F);
		uint32_t dataToCopy = length;
		if(length < cnt)
		{
			dataToCopy = 0;
			cnt = length;
		}

		uint32_t alignedCnt = cnt & 0x0C;
		if(alignedCnt != 0)
		{
			uint16_t alignedData = (uint16_t)(data | (data << 8));
			uint32_t* address = destination;
			for(; 3 < alignedCnt; alignedCnt -= 4)
			{
				*address = (uint32_t)(alignedData | alignedData << 0x10);
				address++;
			}
			cnt = cnt & 3;
		}

		if(cnt != 0)
			set_memory(destination, data, cnt);

		if(dataToCopy == 0)
			return dest;

		length = dataToCopy - 0x10 + ((uint32_t)destination & 0x0F);
		destination = (void*)(((uint32_t)destination & (uint32_t)0xFFFFFFF0) + 0x10);
		cnt = length & 0xFFFFFFF0;
	}

	if(cnt != 0)
	{
		set_memory_short(destination, data, cnt);
		destination += cnt;
		length -= cnt;
	}

	if(length != 0)
		set_memory(destination, data, length);

	return dest;
}

#else

#include <string.h>

/* According to the run-time ABI for the ARM Architecture, this
   function is allowed to corrupt only the integer core register
   permitted to be corrupted by the [AAPCS] (r0-r3, ip, lr, and
   CPSR).

   Therefore, we can't just simply use alias to support the function
   aeabi_memset for the targets with FP register.  Instead, versions
   for these specific targets are written in assembler (in
   aeabi_memset-soft.S).  */

/* NOTE: This ifdef MUST match the one in memset-soft.S.  */
#if __ARM_FP != 0 && !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)

/* Defined in memset-soft.S.  */

#else
#include "../../string/memset.c"
#endif

#endif