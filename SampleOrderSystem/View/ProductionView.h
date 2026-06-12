#pragma once
#include "ConsoleUI.h"
#include "../Model/ProductionJob.h"
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include <vector>

class ProductionView {
public:
    explicit ProductionView(ConsoleUI& ui) : ui_(ui) {}

    void showProductionStatus(
        const std::vector<ProductionJob>& jobs,
        const std::vector<Order>&         orders,
        const std::vector<Sample>&        samples,
        double                            timeScaleSecPerMin);

private:
    ConsoleUI& ui_;
    std::wstring findSampleName(int sampleId, const std::vector<Sample>& s);
    std::wstring findCustomerName(int orderId, const std::vector<Order>& o);
};
