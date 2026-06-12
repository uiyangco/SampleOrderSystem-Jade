#pragma once
#include "ConsoleUI.h"
#include "../Model/Order.h"
#include "../Model/Sample.h"
#include <vector>

class OrderView {
public:
    explicit OrderView(ConsoleUI& ui) : ui_(ui) {}

    void showReservedList(const std::vector<Order>& orders, const std::vector<Sample>& samples);
    void showConfirmedList(const std::vector<Order>& orders, const std::vector<Sample>& samples);
    void showOrdersByStatus(const std::vector<Order>& orders, const std::vector<Sample>& samples);

    int  promptOrderId(const std::wstring& action);
    bool promptApproveOrReject();

    struct ReserveInput { int sampleId; std::wstring customerName; int quantity; };
    ReserveInput promptForOrder(const std::vector<Sample>& samples);

private:
    ConsoleUI& ui_;
    void printOrderRow(const Order& o, const std::wstring& sampleName);
    std::wstring sampleName(int sampleId, const std::vector<Sample>& samples);
};
