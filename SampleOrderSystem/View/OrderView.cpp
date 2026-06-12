#include "OrderView.h"
#include <algorithm>

std::wstring OrderView::sampleName(int sampleId, const std::vector<Sample>& samples) {
    auto it = std::find_if(samples.begin(), samples.end(),
        [sampleId](const Sample& s) { return s.id == sampleId; });
    return it != samples.end() ? it->name : L"(알수없음)";
}

void OrderView::printOrderRow(const Order& o, const std::wstring& name) {
    WORD color = ConsoleUI::WHITE;
    switch (o.status) {
    case OrderStatus::RESERVED:  color = ConsoleUI::YELLOW; break;
    case OrderStatus::CONFIRMED: color = ConsoleUI::GREEN;  break;
    case OrderStatus::PRODUCING: color = ConsoleUI::BLUE;   break;
    case OrderStatus::RELEASE:   color = ConsoleUI::GRAY;   break;
    case OrderStatus::REJECTED:  color = ConsoleUI::RED;    break;
    }
    ui_.printLine(
        L"  #" + std::to_wstring(o.id) + L"\t" +
        name + L"\t고객:" + o.customerName +
        L"\t수량:" + std::to_wstring(o.quantity) +
        L"\t[" + orderStatusToWstring(o.status) + L"]", color);
}

void OrderView::showReservedList(const std::vector<Order>& orders,
                                  const std::vector<Sample>& samples) {
    ui_.printHeader(L"접수된 주문 (RESERVED)");
    bool any = false;
    for (const auto& o : orders) {
        if (o.status != OrderStatus::RESERVED) continue;
        printOrderRow(o, sampleName(o.sampleId, samples));
        any = true;
    }
    if (!any) ui_.printInfo(L"접수된 주문이 없습니다.");
}

void OrderView::showConfirmedList(const std::vector<Order>& orders,
                                   const std::vector<Sample>& samples) {
    ui_.printHeader(L"출고 대기 주문 (CONFIRMED)");
    bool any = false;
    for (const auto& o : orders) {
        if (o.status != OrderStatus::CONFIRMED) continue;
        printOrderRow(o, sampleName(o.sampleId, samples));
        any = true;
    }
    if (!any) ui_.printInfo(L"출고 대기 주문이 없습니다.");
}

void OrderView::showOrdersByStatus(const std::vector<Order>& orders,
                                    const std::vector<Sample>& samples) {
    ui_.printHeader(L"주문 현황");
    ui_.printLine(L"  ID\t시료\t고객\t수량\t상태", ConsoleUI::CYAN);
    ui_.printSeparator();
    for (const auto& o : orders) {
        if (o.status == OrderStatus::REJECTED) continue;
        printOrderRow(o, sampleName(o.sampleId, samples));
    }
}

int OrderView::promptOrderId(const std::wstring& action) {
    return ui_.inputInt(L"  " + action + L"할 주문 ID (0=뒤로): ");
}

int OrderView::promptApproveOrReject() {
    ui_.printLine(L"  1. 승인  2. 거절  0. 뒤로", ConsoleUI::WHITE);
    return ui_.inputInt(L"  선택: ");
}

OrderView::ReserveInput OrderView::promptForOrder(const std::vector<Sample>& samples) {
    ui_.printHeader(L"시료 주문");
    for (const auto& s : samples)
        ui_.printLine(L"  [" + std::to_wstring(s.id) + L"] " + s.name +
                      L"  재고:" + std::to_wstring(s.stock), ConsoleUI::WHITE);
    ui_.printSeparator();
    ReserveInput in;
    in.sampleId     = ui_.inputInt(L"  시료 ID (0=뒤로): ");
    if (in.sampleId == 0) return in;
    in.customerName = ui_.inputLine(L"  고객명: ");
    in.quantity     = ui_.inputInt(L"  수량: ");
    return in;
}
