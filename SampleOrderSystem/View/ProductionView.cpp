#include "ProductionView.h"
#include "../Utils.h"
#include <algorithm>
#include <cmath>

std::wstring ProductionView::findSampleName(int id, const std::vector<Sample>& samples) {
    auto it = std::find_if(samples.begin(), samples.end(),
        [id](const Sample& s) { return s.id == id; });
    return it != samples.end() ? it->name : L"(알수없음)";
}

std::wstring ProductionView::findCustomerName(int orderId, const std::vector<Order>& orders) {
    auto it = std::find_if(orders.begin(), orders.end(),
        [orderId](const Order& o) { return o.id == orderId; });
    return it != orders.end() ? it->customerName : L"(알수없음)";
}

void ProductionView::showProductionStatus(
    const std::vector<ProductionJob>& jobs,
    const std::vector<Order>&         orders,
    const std::vector<Sample>&        samples,
    double                            timeScaleSecPerMin) {

    ui_.printHeader(L"생산 라인");

    // ── 현재 생산 중 ─────────────────────────────────────────────────────────
    ui_.printLine(L"▶ 현재 생산 중", ConsoleUI::CYAN);
    bool hasRunning = false;
    for (const auto& job : jobs) {
        if (job.status != JobStatus::RUNNING) continue;
        hasRunning = true;
        double elapsedSec = 0.0;
        if (job.startedAtMs > 0)
            elapsedSec = (Utils::nowMs() - job.startedAtMs) / 1000.0;
        double elapsedMin = elapsedSec / timeScaleSecPerMin;
        int pct = job.totalMinutes > 0
            ? static_cast<int>(std::min(elapsedMin / job.totalMinutes * 100.0, 100.0))
            : 0;

        // 로딩바: [████████░░░░░░░░░░░░] 40%
        constexpr int kBarWidth = 20;
        int filled = pct * kBarWidth / 100;
        std::wstring bar = L"[";
        for (int i = 0; i < kBarWidth; ++i)
            bar += (i < filled) ? L"█" : L"░";
        bar += L"]";

        int orderQty = 0;
        auto orderIt = std::find_if(orders.begin(), orders.end(),
            [&job](const Order& o) { return o.id == job.orderId; });
        if (orderIt != orders.end()) orderQty = orderIt->quantity;

        ui_.printLine(
            L"  주문#" + std::to_wstring(job.orderId) +
            L"  시료:" + findSampleName(job.sampleId, samples) +
            L"  고객:" + findCustomerName(job.orderId, orders) +
            L"  주문량:" + std::to_wstring(orderQty) + L"개" +
            L"  부족분:" + std::to_wstring(job.shortage) + L"개" +
            L"  실생산량:" + std::to_wstring(job.targetQty) + L"개" +
            L"  " + bar + L" " + std::to_wstring(pct) + L"%",
            ConsoleUI::GREEN);
    }
    if (!hasRunning) ui_.printLine(L"  (생산 중인 작업 없음)", ConsoleUI::GRAY);

    // ── 대기 큐 ──────────────────────────────────────────────────────────────
    ui_.printSeparator();
    ui_.printLine(L"▶ 생산 대기 큐 (FIFO)", ConsoleUI::CYAN);
    int queueNum = 1;
    bool hasWaiting = false;
    for (const auto& job : jobs) {
        if (job.status != JobStatus::WAITING) continue;
        hasWaiting = true;
        int waitOrderQty = 0;
        auto waitOrderIt = std::find_if(orders.begin(), orders.end(),
            [&job](const Order& o) { return o.id == job.orderId; });
        if (waitOrderIt != orders.end()) waitOrderQty = waitOrderIt->quantity;

        ui_.printLine(
            L"  [" + std::to_wstring(queueNum++) + L"] " +
            L"주문#" + std::to_wstring(job.orderId) +
            L"  시료:" + findSampleName(job.sampleId, samples) +
            L"  고객:" + findCustomerName(job.orderId, orders) +
            L"  주문량:" + std::to_wstring(waitOrderQty) + L"개" +
            L"  부족분:" + std::to_wstring(job.shortage) + L"개" +
            L"  실생산량:" + std::to_wstring(job.targetQty) + L"개" +
            L"  총시간:" + std::to_wstring(job.totalMinutes) + L"분",
            ConsoleUI::YELLOW);
    }
    if (!hasWaiting) ui_.printLine(L"  (대기 중인 작업 없음)", ConsoleUI::GRAY);
}
