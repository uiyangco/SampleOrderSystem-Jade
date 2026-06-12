#include "OrderController.h"
#include "../Utils.h"
#include <cmath>
#include <algorithm>

void OrderController::reserve(int sampleId, const std::wstring& customerName, int quantity) {
    if (!sampleRepo_.read(sampleId)) return;

    Order o;
    o.sampleId     = sampleId;
    o.customerName = customerName;
    o.quantity     = quantity;
    o.status       = OrderStatus::RESERVED;
    o.createdAt    = Utils::nowWstring();
    o.updatedAt    = o.createdAt;
    orderRepo_.create(o);
}

void OrderController::approve(int orderId) {
    auto orderOpt = orderRepo_.read(orderId);
    if (!orderOpt || orderOpt->status != OrderStatus::RESERVED) return;
    Order order = *orderOpt;

    auto sampleOpt = sampleRepo_.read(order.sampleId);
    if (!sampleOpt) return;
    const Sample& sample = *sampleOpt;

    // 큐 고려 유효 가용량 계산
    auto allOrders = orderRepo_.readAll();
    auto allJobs   = jobRepo_.readAll();

    int committedFromStock = 0;  // CONFIRMED 주문이 선점한 재고
    int committedFromJobs  = 0;  // PRODUCING 주문이 선점한 생산 예정량
    int plannedProduction  = 0;  // WAITING/RUNNING 잡의 예정 생산량

    for (const auto& o : allOrders) {
        if (o.sampleId != order.sampleId) continue;
        if (o.status == OrderStatus::CONFIRMED) committedFromStock += o.quantity;
        if (o.status == OrderStatus::PRODUCING) committedFromJobs  += o.quantity;
    }
    for (const auto& j : allJobs) {
        if (j.sampleId != order.sampleId) continue;
        if (j.status == JobStatus::WAITING || j.status == JobStatus::RUNNING)
            plannedProduction += j.targetQty;
    }

    int effective = sample.stock - committedFromStock + plannedProduction - committedFromJobs;
    int shortage  = (std::max)(0, order.quantity - (std::max)(0, effective));

    int targetQty = 0;
    if (shortage > 0 && sample.yield > 0.0) {
        targetQty = static_cast<int>(
            std::ceil(static_cast<double>(shortage) / (sample.yield * 0.9)));
    }

    ProductionJob job;
    job.orderId      = orderId;
    job.sampleId     = order.sampleId;
    job.shortage     = shortage;
    job.targetQty    = targetQty;
    job.producedQty  = 0;
    job.totalMinutes = sample.avgProductionTime * targetQty;
    job.status       = JobStatus::WAITING;
    jobRepo_.create(job);

    order.status    = OrderStatus::PRODUCING;
    order.updatedAt = Utils::nowWstring();
    orderRepo_.update(order);
}

void OrderController::reject(int orderId) {
    auto orderOpt = orderRepo_.read(orderId);
    if (!orderOpt || orderOpt->status != OrderStatus::RESERVED) return;
    Order order  = *orderOpt;
    order.status = OrderStatus::REJECTED;
    order.updatedAt = Utils::nowWstring();
    orderRepo_.update(order);
}

bool OrderController::processRelease(int orderId) {
    auto orderOpt = orderRepo_.read(orderId);
    if (!orderOpt || orderOpt->status != OrderStatus::CONFIRMED) return false;
    Order order = *orderOpt;

    auto sampleOpt = sampleRepo_.read(order.sampleId);
    if (!sampleOpt) return false;
    Sample sample = *sampleOpt;

    if (sample.stock < order.quantity) return false;  // 재고 부족

    sample.stock -= order.quantity;
    sampleRepo_.update(sample);

    order.status    = OrderStatus::RELEASE;
    order.updatedAt = Utils::nowWstring();
    orderRepo_.update(order);
    return true;
}
