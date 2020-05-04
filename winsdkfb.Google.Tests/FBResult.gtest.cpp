#include "pch.h"

#include "FBResult.h"

#include <winrt/Windows.Foundation.h>
//
//TEST(FBResultTests, CreateInstance)
//{
//	// arrange
//	winsdkfb::FBResult fbResult;
//
//	// act
//
//	// assert
//	EXPECT_NE(fbResult, nullptr);
//}

TEST(FBResultTests, CreateInstanceWithNullptrShouldNotSucceed)
{
	// arrange
	winsdkfb::FBResult fbResult;

	// act

	// assert
	EXPECT_FALSE(fbResult.Succeeded());
}

TEST(FBResultTests, TestCreateInstanceWithErrorShouldHaveError)
{
	// arrange
	auto fbError = winsdkfb::FBError(0, L"type", L"message");

	// act
	auto fbResult = winsdkfb::FBResult(fbError);

	// assert
	EXPECT_FALSE(fbResult.Succeeded());
	EXPECT_EQ(fbResult.ErrorInfo().Type(), L"type");
}
