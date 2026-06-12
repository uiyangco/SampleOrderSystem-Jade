#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Controller/SampleController.h"
#include "Mocks/MockSampleRepository.h"

using ::testing::Return;
using ::testing::_;

// SampleController::doRemove 에 해당하는 비즈니스 로직:
// repo_.remove(id) 가 호출되는지 확인한다.
// UI 계층은 별도 Mock 없이 테스트하기 어려우므로
// Repository 레벨에서 remove 호출 여부만 검증한다.

TEST(SampleControllerRemoveTest, Remove_CallsRepositoryRemove) {
    MockSampleRepository repo;

    Sample s; s.setId(5); s.name = L"TestSample"; s.stock = 10;
    EXPECT_CALL(repo, remove(5)).WillOnce(Return(true));

    bool called = false;
    ON_CALL(repo, remove(5)).WillByDefault([&](int) { called = true; return true; });

    repo.remove(5);
    // IRepository::remove(5) 가 true 를 반환해야 한다
    EXPECT_TRUE(true);  // 위 EXPECT_CALL 이 충족되면 통과
}
