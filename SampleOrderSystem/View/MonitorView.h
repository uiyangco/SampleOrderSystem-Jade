#pragma once
#include "ConsoleUI.h"
#include "../Model/Order.h"
#include "../Model/Sample.h"
#include <vector>

class MonitorView {
public:
    explicit MonitorView(ConsoleUI& ui) : ui_(ui) {}

    void showDashboard(
        const std::vector<Order>&  orders,
        const std::vector<Sample>& samples);

private:
    ConsoleUI& ui_;
    void showOrderSummary(const std::vector<Order>& orders);
    void showInventory(const std::vector<Order>& orders,
                       const std::vector<Sample>& samples);
};
