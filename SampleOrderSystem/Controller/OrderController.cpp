#include "OrderController.h"
#include "../Utils.h"

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

    // 항상 생산 → PRODUCING (재고 충분 여부 무관)
    ProductionJob job;
    job.orderId      = orderId;
    job.sampleId     = order.sampleId;
    job.targetQty    = order.quantity;
    job.producedQty  = 0;
    job.totalMinutes = sample.avgProductionTime * order.quantity;
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
