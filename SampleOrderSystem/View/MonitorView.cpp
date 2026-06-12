#include "MonitorView.h"
#include <algorithm>

void MonitorView::showOrderSummary(const std::vector<Order>& orders) {
    int reserved = 0, confirmed = 0, producing = 0, release = 0;
    for (const auto& o : orders) {
        switch (o.status) {
        case OrderStatus::RESERVED:  ++reserved;  break;
        case OrderStatus::CONFIRMED: ++confirmed; break;
        case OrderStatus::PRODUCING: ++producing; break;
        case OrderStatus::RELEASE:   ++release;   break;
        default: break;
        }
    }
    ui_.printLine(L"▶ 주문 현황", ConsoleUI::CYAN);
    ui_.printLine(L"  RESERVED  : " + std::to_wstring(reserved),  ConsoleUI::YELLOW);
    ui_.printLine(L"  CONFIRMED : " + std::to_wstring(confirmed), ConsoleUI::GREEN);
    ui_.printLine(L"  PRODUCING : " + std::to_wstring(producing), ConsoleUI::BLUE);
    ui_.printLine(L"  RELEASE   : " + std::to_wstring(release),   ConsoleUI::GRAY);
}

void MonitorView::showInventory(const std::vector<Order>& orders,
                                 const std::vector<Sample>& samples) {
    ui_.printSeparator();
    ui_.printLine(L"▶ 재고 현황", ConsoleUI::CYAN);
    ui_.printLine(L"  시료\t\t재고\t주문대기\t상태", ConsoleUI::CYAN);
    ui_.printSeparator();

    for (const auto& s : samples) {
        int pending = 0;
        for (const auto& o : orders) {
            if (o.sampleId != s.id) continue;
            if (o.status == OrderStatus::RESERVED  ||
                o.status == OrderStatus::CONFIRMED ||
                o.status == OrderStatus::PRODUCING)
                pending += o.quantity;
        }

        std::wstring stateLabel;
        WORD color;
        if (s.stock == 0) {
            stateLabel = L"[고갈]"; color = ConsoleUI::RED;
        } else if (s.stock < pending) {
            stateLabel = L"[부족]"; color = ConsoleUI::YELLOW;
        } else {
            stateLabel = L"[여유]"; color = ConsoleUI::GREEN;
        }

        ui_.printLine(
            L"  " + s.name + L"\t\t" +
            std::to_wstring(s.stock) + L"\t" +
            std::to_wstring(pending) + L"\t\t" +
            stateLabel, color);
    }
}

void MonitorView::showDashboard(
    const std::vector<Order>&  orders,
    const std::vector<Sample>& samples) {
    ui_.printHeader(L"모니터링");
    showOrderSummary(orders);
    showInventory(orders, samples);
}
