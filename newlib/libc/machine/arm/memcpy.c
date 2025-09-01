/* Copyright (c) 2015 ARM Ltd.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
       * Redistributions of source code must retain the above copyright
	 notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above copyright
	 notice, this list of conditions and the following disclaimer in the
	 documentation and/or other materials provided with the distribution.
       * Neither the name of the Linaro nor the
	 names of its contributors may be used to endorse or promote products
	 derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  */

/* The structure of the following #if #else #endif conditional chain
   must match the chain in memcpy.S.  */

#include <picolibc.h>

#ifdef __STARLET__
#include <stddef.h>
#include <stdint.h>

__attribute__((target("arm")))
void *memcpy(void *dest, const void *src, size_t len);

// IOS has a completely custom memcpy to deal with a MEM1 HW bug
// MEM1 accesses of 4 bytes (strb & strh) to MEM1 will thrash 4 bytes
// hence the weird access/seperation of code
__attribute__((target("arm")))
void *memcpy(void *dest, const void *src, size_t len)
{	
	if(len == 0)
		return dest;
	
	//these are normal for ARM (r0 - r2 = function arguments)
	//however, to make sure we have the values in these registers we will redefine them here
	//nintendo's memcpy is very optimised and custom written and its lovely lol
	register size_t length __asm("r3") = len;
	register void *source __asm("r2") = (void*)src;
	register void *destination __asm("r1") = dest;
	register void *ret __asm("r0") = dest;
	
	if((((uint32_t)destination | (uint32_t)source) & 0x03) == 0)
	{
		__asm__ volatile ("\
			#Save the register values that GCC might have used before the asm \n\
			stmdb		sp!,{r4,r5,r6,r7,r8,r9,r10,r11} \n\
			#check if we can do a 0x10 copy paste \n\
			cmp			%[length], #0x0F \n\
			bls			memcpy_end \n\
			#check if we can do a 0x20 copy/paste \n\
			cmp			%[length], #0x1F \n\
			bls			memcpy_By16 \n\
			memcpy_by32: \n\
			ldmia		%[source]!,{r4,r5,r6,r7,r8,r9,r10,r11} \n\
			stmia		%[destination]!,{r4,r5,r6,r7,r8,r9,r10,r11} \n\
			sub			%[length],%[length], #0x20 \n\
			cmp			%[length], #0x1F \n\
			bls			memcpy_By16 \n\
			b			memcpy_by32 \n\
			memcpy_By16: \n\
			cmp			%[length], #0x0F \n\
			bls			memcpy_end \n\
			ldmia		%[source]!,{r4,r5,r6,r7} \n\
			stmia		%[destination]!,{r4,r5,r6,r7} \n\
			sub			%[length],%[length], #0x10 \n\
			b			memcpy_By16 \n\
			memcpy_end: \n\
			ldmia		sp!,{r4,r5,r6,r7,r8,r9,r10,r11} \n"
			:
			: [source] "r" (source), [destination] "r" (destination), [length] "r" (length));

		//ok, nintendo's beauty has run, now we are left to copy 4 bytes at a time
		while(length >= 4)
		{
			*(uint32_t*)destination = *(uint32_t*)source;
			destination += 4;
			source += 4;
			length -= 4;
		}
	}

	//and then there is the MEM1 issue, which means single bytes they also need to be copied by 4 bytes
	if (destination < (void *)0x1800000) 
	{
		for(; length != 0; length--)
		{
			uint32_t* address = (uint32_t*)((uint32_t)dest & (uint32_t)~0x03);
			uint32_t offset = 24 - ((uint32_t)dest & 0x03) * 8;
			uint8_t data = *(uint8_t*)source;
			*address = (*address & (uint32_t)~(0xFF << (offset & 0xFF))) | (data << (offset & 0xFF));
			destination++;
			source++;
		}
	}
	else 
	{
		while(length != 0)
		{
			*(uint8_t*)destination = *(uint8_t*)source;
			destination++;
			source++;
			length--;
		}
	}

	return ret;
}
#else
#include "machine/acle-compat.h"

#if (defined (__OPTIMIZE_SIZE__) || defined (PREFER_SIZE_OVER_SPEED))
#define MEMCPY_FALLBACK
#elif (__ARM_ARCH >= 7 && __ARM_ARCH_PROFILE == 'A' \
       && defined (__ARM_FEATURE_UNALIGNED))
/* Defined in memcpy-armv7a.S.  */
#elif __ARM_ARCH_ISA_THUMB == 2 && !__ARM_ARCH_ISA_ARM
/* Defined in memcpy-armv7m.S.  */
#else
#define MEMCPY_FALLBACK
#endif

#ifdef MEMCPY_FALLBACK
# include "../../string/memcpy.c"

void *__aeabi_memcpy4 (void *__restrict dest, const void * __restrict source, size_t n)
	_ATTRIBUTE ((alias ("memcpy"), weak));

void *__aeabi_memcpy8 (void * __restrict dest, const void * __restrict source, size_t n)
	_ATTRIBUTE ((alias ("memcpy"), weak));

void *__aeabi_memcpy (void * __restrict dest, const void * __restrict source, size_t n)
	_ATTRIBUTE ((alias ("memcpy"), weak));

#endif

#endif