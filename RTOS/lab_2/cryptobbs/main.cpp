#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>

#include "../include/bbs.h"

static resmgr_connect_funcs_t    connect_funcs;
static resmgr_io_funcs_t         io_funcs;
static iofunc_attr_t             attr;

int io_devctl(resmgr_context_t* ctp, io_devctl_t* msg, iofunc_ocb_t* ocb)
{
	int sts;
	void* data;
	bbs::BBSParams* p;
	int nbytes = 0;
	if ((sts = iofunc_devctl_default(ctp, msg, ocb)) != _RESMGR_DEFAULT)
	{
		return (sts);
	}
	// set pointer to message data location
	data = _DEVCTL_DATA(msg->i);

	switch (msg->i.dcmd)
	{
		case bbs::codes::set:
			p = reinterpret_cast<bbs::BBSParams*>(data);
			bbs::setM(p->p, p->q);
			bbs::setSeed(p->seed);
			break;
		case bbs::codes::get:
			nbytes = sizeof(std::uint32_t);
			bbs::psp();
			*(std::uint32_t*)data = bbs::paritybit();
			break;
	}
	std::memset(&msg->o, 0, sizeof(msg->o));
	msg->o.nbytes = nbytes;
	SETIOV(ctp->iov, &msg->o, sizeof(msg->o) + nbytes);
	return (_RESMGR_NPARTS(1));
}

int main(int argc, char** argv)
{
    /* declare variables we'll be using */
    resmgr_attr_t        	resmgr_attr;
    dispatch_t*           	dpp;
    dispatch_context_t*   	ctp;
    int                  	id;

    /* initialize dispatch interface */
    if((dpp = dispatch_create()) == NULL)
    {
        fprintf(stderr,
                "%s: Unable to allocate dispatch handle.\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    /* initialize resource manager attributes */
    memset(&resmgr_attr, 0, sizeof resmgr_attr);
    resmgr_attr.nparts_max = 1;
    resmgr_attr.msg_max_size = 2048;

    /* initialize functions for handling messages */
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                     _RESMGR_IO_NFUNCS, &io_funcs);

    // Устанавливаем наш обработчик devctl
    io_funcs.devctl = io_devctl;

    /* initialize attribute structure used by the device */
    iofunc_attr_init(&attr, S_IFNAM | 0666, 0, 0);

    /* attach our device name */
    id = resmgr_attach(
            dpp,            /* dispatch handle        */
            &resmgr_attr,   /* resource manager attrs */
            "/dev/cryptobbs",  /* device name            */
            _FTYPE_ANY,     /* open type              */
            0,              /* flags                  */
            &connect_funcs, /* connect routines       */
            &io_funcs,      /* I/O routines           */
            &attr);         /* handle                 */
    if(id == -1)
    {
        fprintf(stderr, "%s: Unable to attach name.\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* allocate a context structure */
    ctp = dispatch_context_alloc(dpp);

    /* start the resource manager message loop */
    while(1)
    {
        if((ctp = dispatch_block(ctp)) == NULL)
        {
            fprintf(stderr, "block error\n");
            return EXIT_FAILURE;
        }
        dispatch_handler(ctp);
    }
    return EXIT_SUCCESS; // never go here
}
