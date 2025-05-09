#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <inttypes.h>
#include <cstdio>
#include <iostream>
#include <vector>

#include "../include/bbs.h"

#define VECTOR_SIZE 30

bool stop_signal = false;

void signalHandler(int signum) 
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    stop_signal = true;
}

int main(int argc, char** argv) 
{
    signal(SIGINT, signalHandler);

    if (argc != 4) 
	{
		std::cerr << "E: missing one or more arguments - seed, p and q." << std::endl;
		return EXIT_FAILURE;
	}

    std::uint32_t seed = static_cast<std::uint32_t>(std::atoi(argv[1]));
	std::uint32_t p = static_cast<std::uint32_t>(std::atoi(argv[2]));
	std::uint32_t q = static_cast<std::uint32_t>(std::atoi(argv[3]));

    // open a connection to the server (fd == coid)
    int fd = open("/dev/cryptobbs", O_RDWR);
    if (fd < 0)
    {
        std::cerr << "E: unable to open server connection: " << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }

    bbs::BBSParams param;
    param.seed = seed;
    param.p = p;
    param.q = q;

    std::cout << "Set generator params" << std::endl;
    std::cout << "Seed: " << seed << ", p: " << p << ", q: " << q << std::endl;
    int error;
    if ((error = devctl(fd, SET_GEN_PARAMS, &param, sizeof(param), NULL)) != EOK)
    {
        fprintf(stderr, "Error setting RTS: %s\n", strerror(error));
        exit(EXIT_FAILURE);
    };

    std::vector <std::uint32_t> psp_vector(VECTOR_SIZE);
    std::uint32_t elem;
    int counter = -1;
    std::cout << "Get elements" << std::endl;
    while (!stop_signal)
    {
        if ((error = devctl(fd, GET_ELEMENT, &elem, sizeof(elem), NULL)) != EOK)
        {
            fprintf(stderr, "Error setting RTS: %s\n",
                    strerror(error));
            exit(EXIT_FAILURE);
        };

        counter++;

        if (counter == VECTOR_SIZE)
		{
			counter = 0;
		}

		psp_vector.at(counter) = elem;
        sleep(1);
    }

    std::cout << "Output vector" << std::endl;
    for (auto &_el: psp_vector)
	{
        std::cout << _el << std::endl;
	}
	
    close(fd);
    std::cout << "ok";

    return EXIT_SUCCESS;
}
