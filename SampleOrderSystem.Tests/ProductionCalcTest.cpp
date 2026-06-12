#include <gtest/gtest.h>
#include "Monitor/ProductionCalc.h"

// calcProducedQty(elapsedMin, perUnitMin, targetQty)
// → 경과 시간 동안 생산된 개수 (최대 targetQty)

TEST(ProductionCalcTest, ZeroElapsed_ZeroProduced) {
    EXPECT_EQ(calcProducedQty(0.0, 10.0, 5), 0);
}

TEST(ProductionCalcTest, ExactOneUnitTime_OneProduced) {
    EXPECT_EQ(calcProducedQty(10.0, 10.0, 5), 1);
}

TEST(ProductionCalcTest, HalfTotalTime_HalfProduced) {
    // perUnit=10분, target=10개 → 50분 경과 시 5개
    EXPECT_EQ(calcProducedQty(50.0, 10.0, 10), 5);
}

TEST(ProductionCalcTest, OverTotalTime_CappedAtTarget) {
    EXPECT_EQ(calcProducedQty(999.0, 10.0, 10), 10);
}

TEST(ProductionCalcTest, LessThanOneUnit_ZeroProduced) {
    EXPECT_EQ(calcProducedQty(4.9, 10.0, 5), 0);
}
