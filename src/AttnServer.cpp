#include <AttnServer.h>
#include <RPCServer.h>

#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
using boost::format;

#include <boost/iostreams/stream.hpp>

#include <iostream>
#include <fstream>
#include <streambuf>

#if 0 && (BOOST_VERSION / 100 % 1000) > 74
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

AttnServer::AttnServer(std::string const& fname)
{
    loadConfig(fname);
}

#ifndef HAVE_BOOST_JSON
    template <class PT_ROOT>
    static void loadChainConfig(std::string const& fname, ChainType chaintype,
                                ChainConfig &cfg, PT_ROOT const& root)
    {
        cfg.chaintype = chaintype;
        cfg.ip = root.get<std::string>("ip");
        cfg.port = root.get<uint16_t>("port_ws");
        cfg.door_account_str = root.get<std::string>("door_account");
        auto acct = parseBase58<AccountID>(cfg.door_account_str);
        if (acct)
            cfg.door_account = *acct;
        else {
            auto err = str(format(": %1% - Invalid door_account value \"%2%\"") % fname % cfg.door_account_str);
            throw std::runtime_error(err);
        }
    }

    template <class PT_ROOT>
    static void loadServerConfig(std::string const& fname, AttnServerConfig &cfg, PT_ROOT const& root)
    {
        auto pk = parseBase58<PublicKey>(TokenType::AccountPublic, root.get<std::string>("public_key"));
        
        if (pk) {
            cfg.our_public_key = *pk;
        } else {
            auto err = str(format(": %1% - invalid public key") % fname);
            throw std::runtime_error(err);
        }

        auto key_str = root.get<std::string>("secret_key");
        auto sk = parseBase58<SecretKey>(TokenType::AccountSecret, key_str);
    
        if (!sk) {
            std::optional<Seed> seed = parseRippleLibSeed(key_str);
            if (!seed)
                seed = parseBase58<Seed>(key_str);
            if (seed)
            {
                // this doesn't seem right... we should probably derive the public key from the secret key?
                // TODO: we don't know the key type
                sk = generateKeyPair(KeyType::ed25519, *seed).second;
            }
        }

        if (sk) {
            cfg.our_secret_key = *sk;
        } else {
            auto err = str(format(": %1% - invalid secret key") % fname);
            throw std::runtime_error(err);
        }
        
        cfg.port_peer = root.get<uint16_t>("port_peer");
        cfg.port_ws = root.get<uint16_t>("port_ws");

        cfg.ssk_keys_str = root.get<std::string>("__ssl_key");
        cfg.ssk_cert_str = root.get<std::string>("__ssl_cert");
    }

    template <class PT_ROOT>
    static void loadSNTPConfig(std::string const& fname, SNTPServerConfig &cfg, PT_ROOT const& root)
    {
    }

#endif
    
std::string AttnServer::process_rpc_request(std::string_view data)
{
    namespace pt = boost::property_tree;
    pt::ptree request;
    boost::iostreams::array_source as(data.data(), data.size());
    boost::iostreams::stream<boost::iostreams::array_source> is(as);
    
    pt::read_json(is, request);
    
    std::string error_msg;
    uint32_t error_code = 0;
    pt::ptree result;
    
    try
    {
        
    }
    catch(std::exception const& e)
    {
        error_code = 1;
        error_msg = e.what();
    }

    result.put("error_code", error_code);
    result.put("error_msg", error_msg.c_str());
    result.push_back(request.front());
    
    pt::ptree resp;
    resp.add_child("result", result);

    std::stringstream ss;
    pt::json_parser::write_json(ss, resp);
    return ss.str();
}

void AttnServer::loadConfig(std::string const& fname)
{
#if defined(HAVE_BOOST_JSON)
    std::ifstream t(fname);
    std::string const input(std::istreambuf_iterator<char>(t), {});

    namespace json = boost::json;
    boost::json::error_code ec;
    auto doc = boost::json::parse(input, ec);
    if (ec || !doc.is_object()) {
        auto err = str(format("error parsing config file: %1% - %2%") % fname % ec.message());
        throw std::runtime_error(err);
    }

    auto const& obj = doc.get_object();
    if (obj.contains("attn_server")) {
    }
    
#else
    namespace pt = boost::property_tree;

    pt::ptree root;
    pt::read_json(fname, root);
    auto const& att_root = root.get_child("attn_server");
    
    loadChainConfig(fname, ChainType::mainChain, cfg_.mainchain, att_root.get_child("mainchain"));
    loadChainConfig(fname, ChainType::sideChain, cfg_.sidechain, att_root.get_child("sidechain"));
    loadServerConfig(fname, cfg_, att_root.get_child("server"));
    loadSNTPConfig(fname, cfg_.sntp_servers, att_root.get_child("sntp_servers"));
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
