#include "Order.h"
#include "../Utils.h"

JsonValue Order::toJson() const {
    JsonValue j = JsonValue::makeObject();
    j["id"]           = JsonValue(id);
    j["sampleId"]     = JsonValue(sampleId);
    j["customerName"] = JsonValue(Utils::wstringToUtf8(customerName));
    j["quantity"]     = JsonValue(quantity);
    j["status"]       = JsonValue(Utils::wstringToUtf8(orderStatusToWstring(status)));
    j["createdAt"]    = JsonValue(Utils::wstringToUtf8(createdAt));
    j["updatedAt"]    = JsonValue(Utils::wstringToUtf8(updatedAt));
    return j;
}

Order Order::fromJson(const JsonValue& j) {
    Order o;
    o.id           = j["id"].getInt();
    o.sampleId     = j["sampleId"].getInt();
    o.customerName = Utils::utf8ToWstring(j["customerName"].getString());
    o.quantity     = j["quantity"].getInt();
    o.status       = orderStatusFromString(j["status"].getString());
    o.createdAt    = Utils::utf8ToWstring(j["createdAt"].getString());
    o.updatedAt    = Utils::utf8ToWstring(j["updatedAt"].getString());
    return o;
}
