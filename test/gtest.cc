
#include <gtest/gtest.h>

// Test compatability for integration at KSPT
// If project lives as open source, this should be reworked/removed
#ifdef HAVE_MEOS_TEST
#include "src/gtest-all.cc"

int
main (int argc, char *argv[])
{
    ::testing::InitGoogleTest (&argc, argv);
    return RUN_ALL_TESTS();
}

#endif
