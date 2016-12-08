// test.cpp : Defines the entry point for the console application.
//

#include "gtest/gtest.h"
#include "glog/logging.h"

GTEST_API_ int main(int argc, char **argv)
{
    google::InitGoogleLogging(argv[0]);
	FLAGS_alsologtostderr = true;
	FLAGS_colorlogtostderr = true;
	FLAGS_v = 4;
	FLAGS_minloglevel = google::INFO;

    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();

    return ret;
}

