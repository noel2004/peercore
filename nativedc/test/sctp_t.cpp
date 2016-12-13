#include "gtest/gtest.h"
#include "glog/logging.h"
#include <boost/asio.hpp>
#include <boost/function.hpp>
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
        boost::function<void()> onAssocOpened_;

        void    onAssocError(const std::string& reason, bool closed) override
        {
            LOG(INFO) << "Assoc Error: " << reason << (closed ? "(fatal)" : "(general)") << std::endl;
        }
        void    onAssocOpened(unsigned short streams[2]/*in/out*/) override
        {
            LOG(INFO) << "Assoc Open [" << streams[0] << "/" << streams[1] << "]" << std::endl;
            if (onAssocOpened_)onAssocOpened_();
        }
        void    onAssocClosed() override
        {
            LOG(INFO) << "Assoc closed" << std::endl;
        }
        void    onDCMessage(unsigned short sid, unsigned int ppid,
            const boost::asio::mutable_buffer& buf, void(*freebuffer)(void*)) override
        {
            LOG(INFO) << "Recv message size "<<boost::asio::buffer_size(buf) << std::endl;
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

        auto sock = rtcdc::sctp::SocketCore::createSocketCore(assocbase.get(), sctpport);
        ASSERT_TRUE(!!sock);

        {
            auto sink = boost::shared_ptr<Server_MessageSink>(new Server_MessageSink);
            auto festablish = [sock]() {sock->establish(); };
            sink->onAssocOpened_ = [festablish, this]() {iosrv_.post(festablish);};

            const unsigned short peerport = 1888;
            LOG(INFO) << "please set peer's sctp port to " << peerport << std::endl;
            ASSERT_TRUE(sock->addEntry(peerport, sink));

            iosrv_.run();
            sock->addEntry(0, sink);
        }

        assocbase->stop();

        sock.reset();
        assocbase.reset();

        boost::asio::deadline_timer dt(iosrv_);
        dt.expires_from_now(boost::posix_time::seconds(10000));
        dt.wait();
    }

}