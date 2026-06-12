#pragma once
#include <algorithm>

// 경과 시간으로부터 생산된 개수를 계산한다 (순수 함수 — 테스트 가능).
// elapsedMin  : 생산 시작 후 경과된 분
// perUnitMin  : 단위 1개 생산에 필요한 분 (avgProductionTime)
// targetQty   : 목표 생산 수량
inline int calcProducedQty(double elapsedMin, double perUnitMin, int targetQty) {
    if (perUnitMin <= 0.0) return targetQty;
    int qty = static_cast<int>(elapsedMin / perUnitMin);
    return (std::min)(qty, targetQty);
}
