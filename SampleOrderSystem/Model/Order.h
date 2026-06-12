#pragma once
#include <string>
#include "../json/JsonValue.h"

enum class OrderStatus {
    RESERVED,
    CONFIRMED,
    PRODUCING,
    RELEASE,
    REJECTED
};

inline std::wstring orderStatusToWstring(OrderStatus s) {
    switch (s) {
    case OrderStatus::RESERVED:  return L"RESERVED";
    case OrderStatus::CONFIRMED: return L"CONFIRMED";
    case OrderStatus::PRODUCING: return L"PRODUCING";
    case OrderStatus::RELEASE:   return L"RELEASE";
    case OrderStatus::REJECTED:  return L"REJECTED";
    }
    return L"UNKNOWN";
}

inline OrderStatus orderStatusFromString(const std::string& s) {
    if (s == "CONFIRMED") return OrderStatus::CONFIRMED;
    if (s == "PRODUCING") return OrderStatus::PRODUCING;
    if (s == "RELEASE")   return OrderStatus::RELEASE;
    if (s == "REJECTED")  return OrderStatus::REJECTED;
    return OrderStatus::RESERVED;
}

struct Order {
    int          id = 0;
    int          sampleId = 0;
    std::wstring customerName;
    int          quantity = 0;
    OrderStatus  status = OrderStatus::RESERVED;
    std::wstring createdAt;
    std::wstring updatedAt;

    int  getId()        const { return id; }
    void setId(int i)         { id = i; }

    JsonValue   toJson()                  const;
    static Order fromJson(const JsonValue& j);
};
