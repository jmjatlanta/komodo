#pragma once
#include <string>

struct RPCDetails
{
    std::string username;
    std::string password;
    uint32_t port;
    uint32_t beamPort;
    uint32_t codaPort;
    RPCDetails(uint32_t defaultPort = 0) : port(defaultPort) {}
    bool IsValid() { return port != 0; }
};

struct RPCParameters
{
    RPCDetails assetchain{7771}; // the RPC details for the asset chain
    RPCDetails komodo{7771}; // the RPC details for the komodo chain
    RPCDetails notary; // the RPC details for the chain that notarizes
};