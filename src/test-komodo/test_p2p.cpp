#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <p2p/p2p.h>

namespace TestP2P {


TEST(TestP2P, ctor)
{
    // build the parameters
    P2PParameters p2pParams;
    {
        // call constructor
        P2P p2p(p2pParams, Params(CBaseChainParams::Network::REGTEST));
    }
    // destructor called, shutdown clean and complete.
}

TEST(TestP2P, TwoNodes)
{
    // logging
    //fPrintToConsole = true;
    // context for node 1
    std::shared_ptr<P2P> p2p1;
    P2PParameters p2pParameters1;
    boost::thread_group threadGroup1;
    CScheduler scheduler1;
    // context for node 2    
    std::shared_ptr<P2P> p2p2;
    P2PParameters p2pParameters2;
    boost::thread_group threadGroup2;
    CScheduler scheduler2;
    {
        // get p2p1 up and running
        p2pParameters1.port = 10001;
        CScheduler::Function serviceLoop = boost::bind(&CScheduler::serviceQueue, &scheduler1);
        threadGroup1.create_thread(boost::bind(&TraceThread<CScheduler::Function>, "scheduler", serviceLoop));        
        p2p1 = std::make_shared<P2P>( p2pParameters1, Params());
        p2p1->StartNode(threadGroup1, scheduler1);
        CAddress addr( CService("127.0.0.1:10001") );
        std::string errorString;
        p2p1->BindListenPort(addr, errorString, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    ASSERT_EQ(p2p1->GetNumberConnected(), 0);
    {
        p2pParameters2.port = 10002;
        // get p2p2 up and running and talking to p2p1
        CScheduler::Function serviceLoop = boost::bind(&CScheduler::serviceQueue, &scheduler2);
        threadGroup2.create_thread(boost::bind(&TraceThread<CScheduler::Function>, "scheduler", serviceLoop));
        p2p2 = std::make_shared<P2P>( p2pParameters2, Params());
        p2p2->AddNode("127.0.0.1:10001");
        p2p2->StartNode(threadGroup2, scheduler2);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // See if the two nodes are connected
    ASSERT_EQ(p2p1->GetNumberConnected(), 1);
    ASSERT_EQ(p2p2->GetNumberConnected(), 1);
    // shut down threads
    threadGroup2.interrupt_all();
    threadGroup2.join_all();
    threadGroup1.interrupt_all();
    threadGroup1.join_all();
}

TEST(TestP2P, AddRemoveNode)
{
    P2PParameters p2pParameters;
    P2P p2p(p2pParameters, Params());
    std::string n1("127.0.0.1:1234");
    std::string n2("127.0.0.1:1235");
    // add
    ASSERT_TRUE(p2p.AddNode(n1));
    ASSERT_TRUE(p2p.AddNode(n2));
    ASSERT_FALSE(p2p.AddNode(n1));
    ASSERT_EQ(p2p.GetAddedNodes().size(), 2);
    // remove
    ASSERT_TRUE(p2p.RemoveNode(n1));
    ASSERT_FALSE(p2p.RemoveNode(n1));
    ASSERT_EQ(p2p.GetAddedNodes().size(), 1);
    ASSERT_TRUE(p2p.RemoveNode(n2));
    ASSERT_EQ(p2p.GetAddedNodes().size(), 0);
}

} // namespace TestP2P