#include <cstdint>
#include <stdio.h>
#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>
#include <devctl.h>

#define SET_GEN_PARAMS __DIOT(_DCMD_MISC, 1, bbs::BBSParams)
#define GET_ELEMENT __DIOF(_DCMD_MISC, 2, std::uint32_t)

namespace bbs
{

    struct BBSParams 
	{
        std::uint32_t seed;
        std::uint32_t p;
        std::uint32_t q;
    };

    enum codes 
	{
    	set = SET_GEN_PARAMS,
    	get = GET_ELEMENT
    };

    namespace 
	{
        std::uint64_t n = 0;
        std::uint64_t x = 0;
    }

    void setM(std::uint32_t p, std::uint32_t q) 
	{
        if (p % 4 != 3 || q % 4 != 3) 
		{
            fprintf(stderr, "p and q must be ≡ 3 mod 4\n");
        }
        n = static_cast<std::uint64_t>(p) * q;
    }

    void setSeed(std::uint32_t seed) 
	{
        if (seed == 0 || seed >= n) 
		{
            fprintf(stderr, "Seed must be in (0, n)\n");
        }
        x = seed;
    }

    void psp() {
        if (n == 0 || x == 0) 
		{
            fprintf(stderr, "BBS not initialized\n");
        }
        x = (x * x) % n;
    }

    std::uint32_t paritybit() 
	{
    	std::uint32_t parity = 0;
    	std::uint32_t number = x;
		while (number) 
		{
			parity ^= (number & 1);
			number >>= 1;
		}
		return parity;
    }
}
