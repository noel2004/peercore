#include "gtest/gtest.h"
#include "glog/logging.h"
#include <boost/asio.hpp>
#include "../nativedclib/io.h"
#include "../nativedclib/sctpwrapper.h"
#include "../nativedclib/sctp.h"
#include "../nativedclib/dctransport.h"

namespace {


    class LMPNSCTPTest : public ::testing::Test
    {

    protected:
        boost::asio::io_service iosrv_;

        //gtest
        void SetUp() override{
            rtcdc::sctp::Module::Init();
        }
        void TearDown() override{
            rtcdc::sctp::Module::Init(true);
        }
    };

    class Server_MessageSink : public rtcdc::DatachannelCoreCall
    {
    public:
        void    onAssocError(const std::string& reason, bool closed) override
        {
            LOG(INFO) << "Assoc Error: " << reason << (closed ? "(fatal)" : "(general)") << std::endl;
        }
        void    onAssocOpened(unsigned short streams[2]/*in/out*/) override
        {
            LOG(INFO) << "Assoc Open [" << streams[0] << "/" << streams[1] << "]" << std::endl;
        }
        void    onAssocClosed() override
        {
            LOG(INFO) << "Assoc closed" << std::endl;
        }
        void    onDCMessage(unsigned short sid, unsigned int ppid,
            const boost::asio::mutable_buffer& buf, void(*freebuffer)(void*)) override
        {
            freebuffer(boost::asio::buffer_cast<void*>(buf));
        }
        void    onCanSendMore() override
        {
            LOG(INFO) << "Assoc send dry" << std::endl;
        }
    };

    TEST_F(LMPNSCTPTest, DISABLED_server)
    {
        rtcdc::asio_socketwrapper  socketbase(iosrv_);
        boost::system::error_code ec;
        socketbase.open(ec);
        ASSERT_TRUE(!ec);

        socketbase.prepare(ec);
        ASSERT_TRUE(!ec);

        auto port = socketbase.get_port(ec);
        ASSERT_TRUE(!ec);

        const unsigned short sctpport = 5000;
        LOG(INFO) << "Binding at UDP port " << port << ", sctp port" << sctpport <<std::endl;

        typedef rtcdc::SCTP_Association<rtcdc::asio_socketwrapper> sctp_assoc_type;
        boost::shared_ptr<sctp_assoc_type> assocbase(new sctp_assoc_type(socketbase));
        assocbase->init(ec);
        ASSERT_TRUE(!ec);

        auto sctpsock = rtcdc::sctp::SocketCore::createSocketCore(assocbase.get(), sctpport);
        ASSERT_TRUE(!!sctpsock);
        auto sink = boost::shared_ptr<Server_MessageSink>(new Server_MessageSink);

        const unsigned short peerport = 1888;
        LOG(INFO) << "please set peer's sctp port to " << peerport << std::endl;
        ASSERT_TRUE(sctpsock->addEntry(peerport, sink));

        iosrv_.run();

        assocbase->stop();

    }

}