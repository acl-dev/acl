#include "stdafx.h"
#include "req_callback.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////

master_service::master_service()
{
}

master_service::~master_service()
{
}

bool master_service::on_accept(acl::aio_socket_stream* client)
{
    /*
        logger("connect from %s, fd %d", client->get_peer(true),
            client->sock_handle());
    */
    req_callback* callback = new req_callback(client);
    callback->start();
    return true;
}

void master_service::proc_on_init()
{

}

void master_service::proc_on_exit()
{

}

