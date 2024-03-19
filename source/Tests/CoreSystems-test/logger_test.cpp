#include <gmock/gmock.h>

#include <CoreSystems/BowLogger.h>

class logger_test: public testing::Test
{
public:
};

TEST_F(logger_test, CheckSomeResults)
{
	EXPECT_NO_FATAL_FAILURE(bow::EventLogger::GetInstance().LogTrace("Test"));
	EXPECT_NO_FATAL_FAILURE(bow::EventLogger::GetInstance().LogInfo("Test"));
	EXPECT_NO_FATAL_FAILURE(bow::EventLogger::GetInstance().LogWarning("Test"));
	EXPECT_NO_FATAL_FAILURE(bow::EventLogger::GetInstance().LogError("Test"));
	EXPECT_NO_FATAL_FAILURE(bow::EventLogger::GetInstance().LogAssert(true, __FILE__, __LINE__, "Test"));
    // ...
}
