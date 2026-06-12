#pragma once
#include "../Model/Repository/IRepository.h"
#include "../Model/Order.h"
#include "../Model/Sample.h"
#include "../Model/ProductionJob.h"

class OrderController {
public:
    OrderController(
        IRepository<Order>&         orderRepo,
        IRepository<Sample>&        sampleRepo,
        IRepository<ProductionJob>& jobRepo)
        : orderRepo_(orderRepo)
        , sampleRepo_(sampleRepo)
        , jobRepo_(jobRepo) {}

    void reserve(int sampleId, const std::wstring& customerName, int quantity);
    void approve(int orderId);
    void reject(int orderId);
    bool processRelease(int orderId);  // false = 재고 부족

private:
    IRepository<Order>&         orderRepo_;
    IRepository<Sample>&        sampleRepo_;
    IRepository<ProductionJob>& jobRepo_;
};
