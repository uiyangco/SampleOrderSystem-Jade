#include "OrderController.h"
#include "../Utils.h"
#include <cmath>

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
    Sample sample = *sampleOpt;

    order.updatedAt = Utils::nowWstring();

    if (sample.stock >= order.quantity) {
        sample.stock -= order.quantity;
        order.status  = OrderStatus::CONFIRMED;
        sampleRepo_.update(sample);
        orderRepo_.update(order);
    } else {
        int shortage  = order.quantity - sample.stock;
        int targetQty = static_cast<int>(
            std::ceil(shortage / (sample.yield * 0.9)));
        int totalMin  = sample.avgProductionTime * targetQty;

        ProductionJob job;
        job.orderId      = orderId;
        job.sampleId     = order.sampleId;
        job.targetQty    = targetQty;
        job.producedQty  = 0;
        job.totalMinutes = totalMin;
        job.status       = JobStatus::WAITING;
        jobRepo_.create(job);

        order.status = OrderStatus::PRODUCING;
        orderRepo_.update(order);
    }
}

void OrderController::reject(int orderId) {
    auto orderOpt = orderRepo_.read(orderId);
    if (!orderOpt || orderOpt->status != OrderStatus::RESERVED) return;
    Order order  = *orderOpt;
    order.status = OrderStatus::REJECTED;
    order.updatedAt = Utils::nowWstring();
    orderRepo_.update(order);
}

void OrderController::processRelease(int orderId) {
    auto orderOpt = orderRepo_.read(orderId);
    if (!orderOpt || orderOpt->status != OrderStatus::CONFIRMED) return;
    Order order  = *orderOpt;
    order.status = OrderStatus::RELEASE;
    order.updatedAt = Utils::nowWstring();
    orderRepo_.update(order);
}
