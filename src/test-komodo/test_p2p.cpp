#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <p2p/p2p.h>

namespace TestP2P {


TEST(TestP2P, ctor)
{
    // build the parameters
    P2PParameters p2pParams;
    CChain cchain;
    {
        // call constructor
        P2P p2p(p2pParams, ChainStatus(&cchain, Params(CBaseChainParams::Network::REGTEST)));
    }
    // destructor called, shutdown clean and complete.
}

TEST(TestP2P, two_nodes)
{
    // context for node 1
    std::shared_ptr<P2P> p2p1;
    CChain activeChain1; // the active chain for p2p1
    P2PParameters p2pParameters1;
    boost::thread_group threadGroup1;
    CScheduler scheduler1;
    // context for node 2    
    std::shared_ptr<P2P> p2p2;
    CChain activeChain2;
    P2PParameters p2pParameters2;
    boost::thread_group threadGroup2;
    CScheduler scheduler2;
    {
        // get p2p1 up and running
        p2pParameters1.port = 10001;
        p2p1 = std::make_shared<P2P>( p2pParameters1, ChainStatus(&activeChain1, Params()));
        p2p1->StartNode(threadGroup1, scheduler1);
    }
    {
        p2pParameters2.port = 10002;
        // get p2p2 up and running and talking to p2p1
        p2p2 = std::make_shared<P2P>( p2pParameters2, ChainStatus(&activeChain2, Params()));
        p2p2->AddOneShot("127.0.0.1:10001");
        p2p2->StartNode(threadGroup2, scheduler2);
    }
    // see if one is talking to the other
    std::this_thread::sleep_for(std::chrono::seconds(30));

    // shut down threads
    threadGroup1.interrupt_all();
    threadGroup1.join_all();
    threadGroup2.interrupt_all();
    threadGroup2.join_all();
}

} // namespace TestP2P