#ifndef RPCSERVER_H_INCLUDED
#define RPCSERVER_H_INCLUDED

#include "AttnServer.h"

namespace ripple {

class RPCServer
{
private:

public:
    RPCServer(boost::asio::io_context& io_context, AttnServer &attn_server);

private:
};

}

#endif  // RPCSERVER_H_INCLUDED