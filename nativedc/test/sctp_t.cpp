#include "gtest/gtest.h"
#include "glog/logging.h"
#include <boost/asio.hpp>
#include "../nativedclib/io.h"
#include "../nativedclib/sctpwrapper.h"
#include "../nativedclib/sctp.h"

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

        LOG(INFO) << "Binding at port " << port << std::endl;

        typedef rtcdc::SCTP_Association<rtcdc::asio_socketwrapper> sctp_assoc_type;
        boost::shared_ptr<sctp_assoc_type> assocbase(new sctp_assoc_type(socketbase));
        assocbase->init(ec);
        ASSERT_TRUE(!ec);

        iosrv_.run();

        assocbase->stop();

    }

}