#ifndef ATTNSERVER_H_INCLUDED
#define ATTNSERVER_H_INCLUDED

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <ripple/protocol/Sign.h>

namespace boost {
namespace asio {
    class io_context;
}
}

namespace ripple {

struct AttnServerConfig
{
    PublicKey public_key;
    SecretKey secret_key;

    std::vector<PublicKey> peer_public_keys;
    std::vector<SecretKey> peer_secret_key;
    std::vector<std::string> sntp_servers;

    std::string db_path;

};

class AttnServer
{
private:
    AttnServerConfig cfg_;

public:
    AttnServer();

    void mainLoop();

private:
    void loadConfig();
};

}

#endif  // ATTNSERVER_H_INCLUDED
