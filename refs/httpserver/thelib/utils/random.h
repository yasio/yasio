#ifndef _NEX_RANDOM_H_
#define _NEX_RANDOM_H_
//
// Copyright (c) 2002 - Equamen - All Rights Reserved
//
// Permission is granted to copy, use, modify, sell and distribute
// this source code provided this copyright notice appears in all copies.
// This code is provided "as is" without express or implied
// warranty, with no claim to its suitability for any purpose.
//
// this and other source code and software.
//
// Original Coyright (c) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura
//
// Functions for MT19937, with initialization improved 2002/2/10.
// Coded by Takuji Nishimura and Makoto Matsumoto.
// This is a faster version by taking Shawn Cokus's optimization,
// Matthe Bellew's simplification, Isaku Wada's real version.
// C++ version by Lyell Haynes (Equamen)
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. The names of its contributors may not be used to endorse or promote
//    products derived from this software without specific prior written
//    permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

namespace thelib
{
	namespace twister_defines
	{
		// Period parameters
		static const int N=624;
		static const int M=397;
		static const unsigned int MATRIX_A=0x9908b0df; // constant vector a
		static const unsigned int UMASK=0x80000000; // most significant w-r bits
		static const unsigned int LMASK=0x7fffffff; // least significant r bits

		struct N_MINUS_M { enum {val = N-M }; };
		struct N_MINUS_M_PLUS_1 { enum {val = N_MINUS_M::val + 1 }; };
	}

	class mersenne_twister
	{
	public:
             mersenne_twister() : m_left(1) { init(time(NULL));	}

		explicit mersenne_twister(unsigned int p_seed) : m_left(1) {	init(p_seed);	}

		mersenne_twister(unsigned int *p_init_key, unsigned int p_key_length) : m_left(1)
		{
			int i=1, j=0, k = (twister_defines::N > p_key_length ? twister_defines::N : p_key_length);

			init(19650218UL);

			for (; k; --k)
			{
				m_state[i] = (m_state[i] ^ ((m_state[i-1] ^ (m_state[i-1] >> 30)) * 1664525UL)) + p_init_key[j] + j; // non linear
				m_state[i] &= 0xffffffffUL; // for WORDSIZE > 32 machines
				i++; j++;
				if (i>=twister_defines::N)
				{
					m_state[0] = m_state[twister_defines::N-1];
					i=1;
				}
				if (j>=p_key_length) {	j=0; }
			}

			for (k=twister_defines::N-1; k; k--)
			{
				m_state[i] = (m_state[i] ^ ((m_state[i-1] ^ (m_state[i-1] >> 30)) * 1566083941UL)) - i; // non linear
				m_state[i] &= 0xffffffffUL; // for WORDSIZE > 32 machines
				i++;
				if (i>=twister_defines::N)
				{
					m_state[0] = m_state[twister_defines::N-1];
					i=1;
				}
			}

			m_state[0] = 0x80000000UL; // MSB is 1; assuring non-zero initial array
		}

		~mersenne_twister() {}

		inline int genrand_int32()
		{
			unsigned int y;

			if (--m_left == 0)
			  { next_state();	}

			y = *m_next++;

			// Tempering
			y ^= (y >> 11);
			y ^= (y << 7) & 0x9d2c5680UL;
			y ^= (y << 15) & 0xefc60000UL;
			y ^= (y >> 18);

			return y;
		}

		// generates a random number on [0,0x7fffffff]-interval
		inline int genrand_int31() {	return genrand_int32() >> 1; }

		// generates a random number on [0,1]-real-interval
		inline double genrand_real1()
		{		// divided by 2^32-1
			return (double)genrand_int32() * (1.0/4294967295.0);
		}

		inline double genrand_real2()
		{
			// divided by 2^32
			return (double)genrand_int32() * (1.0/4294967296.0);
		}

		inline double genrand_real3()
		{
			// divided by 2^32
			return ((double)genrand_int32() + 0.5) * (1.0/4294967296.0);
		}

		// generates a random number on [0,1) with 53-bit resolution
		double genrand_res53()
		{
			unsigned int a=genrand_int32()>>5, b=genrand_int32()>>6;
			return(a*67108864.0+b)*(1.0/9007199254740992.0);
		}

	private:
		inline void init(unsigned int p_seed)
		{
			int j;
			m_state[0]= p_seed & 0xffffffffUL;
			for (j=1; j<twister_defines::N; j++)
			{
				m_state[j] = (1812433253UL * (m_state[j-1] ^ (m_state[j-1] >> 30)) + j);
				// See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier.
				// In the previous versions, MSBs of the seed affect
				// only MSBs of the array state[].
				// 2002/01/09 modified by Makoto Matsumoto
				m_state[j] &= 0xffffffffUL;  // for >32 bit machines
			}
		}

		inline void next_state()
		{
			unsigned int *p=m_state;
			int j;

			m_left = twister_defines::N;
			m_next = m_state;

			for (j = twister_defines::N_MINUS_M::val; --j; ++p)
			{
				*p = (p[twister_defines::M] ^ TWIST(p[0], p[1]));
			}

			for (j=twister_defines::M; --j; p++)
			{
				*p = (p[twister_defines::N_MINUS_M::val] ^ TWIST(p[0], p[1]));
			}

			*p = (p[twister_defines::N_MINUS_M::val] ^ TWIST(p[0], m_state[0]));
		}

		inline unsigned int MIXBITS(unsigned int u, unsigned int v)
		{
			return ((u & twister_defines::UMASK) | (v & twister_defines::LMASK));
		}

		inline unsigned int TWIST(unsigned int u, unsigned int v)
		{
			return ((MIXBITS(u, v) >> 1) ^ (v&1UL ? twister_defines::MATRIX_A : 0UL));
		}

		unsigned int m_state[twister_defines::N];
		int m_left;
		unsigned int *m_next;
	};

	template<typename T> inline T randf()
	{
		static mersenne_twister s_twister(time(NULL));
		return (T)s_twister.genrand_real1();
	}

    inline unsigned int rand_unsigned()
	{
		static mersenne_twister s_twister(time(NULL));
		return (unsigned int)s_twister.genrand_int32();
	}

    inline int rand_signed()
	{
		static mersenne_twister s_twister(time(NULL));
		return s_twister.genrand_int32();
	}
}

/* value range: [min, max) and [0, 0x7ffffffff) */
inline int rrand(int min, int max)  
{
    return ( thelib::rand_unsigned() % ((max) - (min)) + (min) );
}

/* value range: [min, max) */
inline int rrands(int min, int max)
{
    int original = thelib::rand_unsigned();
    if(original > 0)
    {
        return ( original % ((max) - (min)) + (min) );
    }
    else {
        return ( original % ((max) - (min)) + (max) - 1 ); 
    }
}

/* value range: [0.f, 1.f) */
inline float randf(void)
{
    return ( thelib::randf<float>() );
}

#endif

