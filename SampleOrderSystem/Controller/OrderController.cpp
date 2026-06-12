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

    // 유효 가용량 = 현재재고 - (CONFIRMED+PRODUCING 주문이 선점한 수량)
    auto allOrders = orderRepo_.readAll();

    int committed = 0;
    for (const auto& o : allOrders) {
        if (o.sampleId != order.sampleId) continue;
        if (o.status == OrderStatus::CONFIRMED || o.status == OrderStatus::PRODUCING)
            committed += o.quantity;
    }

    int effective = sample.stock - committed;
    int shortage  = (std::max)(0, order.quantity - (std::max)(0, effective));

    int targetQty = 0;
    if (shortage > 0) {
        if (sample.yield > 0.0)
            targetQty = static_cast<int>(
                std::ceil(static_cast<double>(shortage) / (sample.yield * 0.9)));
        else
            targetQty = shortage;  // yield 미설정 시 1:1 생산
    }

    if (shortage == 0) {
        order.status    = OrderStatus::CONFIRMED;
        order.updatedAt = Utils::nowWstring();
        orderRepo_.update(order);
        return;
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
