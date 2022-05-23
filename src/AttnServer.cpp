#include <AttnServer.h>
#include <RPCServer.h>

#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
using boost::format;

#include <iostream>
#include <fstream>
#include <streambuf>

#if (BOOST_VERSION / 100 % 1000) > 74
    #define HAVE_BOOST_JSON
#endif

#ifdef HAVE_BOOST_JSON
    #include <boost/json.hpp>
    #include <boost/json/src.hpp>
#else
    #include <boost/property_tree/ptree.hpp>
    #include <boost/property_tree/json_parser.hpp>
#endif

namespace ripple {
namespace sidechain {

AttnServer::AttnServer(std::string const& config_filename)
{
    loadConfig(config_filename);
}

#ifndef HAVE_BOOST_JSON
    template <class PT_ROOT>
    static void loadChainConfig(ChainConfig &cfg, PT_ROOT const& root)
    {
    }

    template <class PT_ROOT>
    static void loadServerConfig(AttnServerConfig &cfg, PT_ROOT const& root)
    {
    }

    template <class PT_ROOT>
    static void loadSNTPConfig(SNTPServerConfig &cfg, PT_ROOT const& root)
    {
    }

#endif
    
void AttnServer::loadConfig(std::string const& config_filename)
{
#if defined(HAVE_BOOST_JSON)
    std::ifstream t(config_filename);
    std::string const input(std::istreambuf_iterator<char>(t), {});

    namespace json = boost::json;
    boost::json::error_code ec;
    auto doc = boost::json::parse(input, ec);
    if (ec || !doc.is_object())
        throw std::runtime_error(str(format("error parsing config file: %1% - %2%") % config_filename % ec.message()));

    auto const& obj = doc.get_object();
    if (obj.contains("attn_server")) {
    }
    
#else
    namespace pt = boost::property_tree;

    pt::ptree root;
    pt::read_json(config_filename, root);
    auto const& att_root = root.get_child("attn_server");
    
    loadChainConfig(cfg_.mainchain, att_root.get_child("mainchain"));
    loadChainConfig(cfg_.sidechain, att_root.get_child("sidechain"));
    loadServerConfig(cfg_, att_root.get_child("server"));
    loadSNTPConfig(cfg_.sntp_servers, att_root.get_child("sntp_servers"));
#endif
}


void AttnServer::mainLoop()
{
    const int num_threads = 4;
    boost::asio::io_context ioc(num_threads);
    
    RPCServer rpc_server(ioc, *this);

    std::vector<std::thread> v;
    for(int i = 1; i<num_threads; ++i)
        v.emplace_back([&ioc] () { ioc.run(); });
    ioc.run();
}

}
}
