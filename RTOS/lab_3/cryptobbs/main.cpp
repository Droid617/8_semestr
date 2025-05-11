#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <cinttypes>
#include <map>
#include <mutex>
#include <io/io.h>
#include <devctl.h>

#include "../include/bbs.h"
/*
 *  определяем константу THREAD_POOL_PARAM_T чтобы отключить предупреждения
 *  компилятора при использовании функций семейства dispatch_*()
 */
#define THREAD_POOL_PARAM_T dispatch_context_t

#include <sys/iofunc.h>
#include <sys/dispatch.h>

// client session context
typedef struct
{
    int client_id;
    bbs::BBSParams params;
    uint32_t seed;
} client_session_t;

static resmgr_connect_funcs_t    connect_funcs;
static resmgr_io_funcs_t         io_funcs;
static iofunc_attr_t             attr;

std::map<int, client_session_t*> sessions;
std::mutex sessions_mtx; // for sessions access

client_session_t* create_session(int client_id)
{
    std::lock_guard<std::mutex> lock(sessions_mtx);

    if (sessions.find(client_id) != sessions.end())
    {
        return sessions[client_id];
    }

    client_session_t* session = new client_session_t;
    session->client_id = client_id;

    session->params.p = 0;
    session->params.q = 0;
    session->params.seed = 0;
    session->seed = 0;

    sessions[client_id] = session;
    return session;
}

void destroy_session(int client_id)
{
    std::lock_guard<std::mutex> lock(sessions_mtx);

    auto it = sessions.find(client_id);
    if (it != sessions.end())
    {
        delete it->second;
        sessions.erase(it);
    }
}

int io_open(resmgr_context_t *ctp, io_open_t *msg, iofunc_attr_t *attr, void *extra)
{
    int client_id = msg->connect.ioflag & _IO_FLAG_RD;
    if (client_id == 0)
    {
        client_id = ctp->rcvid;
    }

    create_session(client_id);

    return iofunc_open_default(ctp, msg, attr, extra);
}

int io_close_ocb(resmgr_context_t *ctp, void *reserved, iofunc_ocb_t *ocb)
{
    int client_id = ctp->rcvid;

    destroy_session(client_id);

    return iofunc_close_ocb_default(ctp, reserved, ocb);
}

int io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, iofunc_ocb_t *ocb)
{
    int sts;
    void* data;
    bbs::BBSParams* p;
    int nbytes = 0;

    int client_id = ctp->rcvid;

    client_session_t* session = create_session(client_id);
    if (!session)
    {
        return ENOMEM;
    }

    if ((sts = iofunc_devctl_default(ctp, msg, ocb)) != _RESMGR_DEFAULT)
    {
        return (sts);
    }

    data = _DEVCTL_DATA(msg->i);

    switch (msg->i.dcmd)
    {
        case bbs::codes::set:
            p = reinterpret_cast<bbs::BBSParams*>(data);
            session->params = *p;
            bbs::setM(session->params.p, session->params.q);
            bbs::setSeed(session->params.seed);
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

int main(int argc, char **argv)
{
    thread_pool_attr_t   pool_attr;
    resmgr_attr_t        resmgr_attr;
    dispatch_t           *dpp;
    thread_pool_t        *tpp;
    dispatch_context_t   *ctp;
    int                  id;

    /* инициализация интерфейса диспетчеризации */
    if((dpp = dispatch_create()) == NULL)
    {
        fprintf(stderr,
                "%s: Unable to allocate dispatch handle.\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    /* инициализация атрибутов АР - параметры IOV */
    memset(&resmgr_attr, 0, sizeof resmgr_attr);
    resmgr_attr.nparts_max = 1;
    resmgr_attr.msg_max_size = 2048;

    /* инициализация структуры функций-обработчиков сообщений */
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                     _RESMGR_IO_NFUNCS, &io_funcs);
    connect_funcs.open = io_open;
    io_funcs.close_ocb = io_close_ocb;
    io_funcs.devctl = io_devctl;

    /* инициализация атрибутов устройства*/
    iofunc_attr_init(&attr, S_IFNAM | 0666, 0, 0);

    /* прикрепление к точке монтирования в пространстве имён путей */
    id = resmgr_attach(
            dpp,               /* хэндл интерфейса диспетчеризации */
            &resmgr_attr,      /* атрибуты АР */
            "/dev/cryptobbs",  /* точка монтирования */
            _FTYPE_ANY,        /* open type              */
            0,                 /* флаги                  */
            &connect_funcs,    /* функции установления соединения */
            &io_funcs,         /* функции ввода-вывода   */
            &attr);            /* хэндл атрибутов устройства */
    if(id == -1)
    {
        fprintf(stderr, "%s: Unable to attach name.\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* инициализация атрибутов пула потоков */
    memset(&pool_attr, 0, sizeof pool_attr);
    pool_attr.handle = dpp;
    pool_attr.context_alloc = dispatch_context_alloc;
    pool_attr.block_func = dispatch_block;
    pool_attr.unblock_func = dispatch_unblock;
    pool_attr.handler_func = dispatch_handler;
    pool_attr.context_free = dispatch_context_free;
    pool_attr.lo_water = 2;
    pool_attr.hi_water = 4;
    pool_attr.increment = 1;
    pool_attr.maximum = 50;

    /* инициализация пула потоков */
    if((tpp = thread_pool_create(&pool_attr,
                                 POOL_FLAG_EXIT_SELF)) == NULL)
    {
        fprintf(stderr, "%s: Unable to initialize thread pool.\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    /* запустить потоки, блокирующая функция */
    thread_pool_start(tpp);
    /* здесь вы не окажетесь, грустно */
    return EXIT_SUCCESS;
}
