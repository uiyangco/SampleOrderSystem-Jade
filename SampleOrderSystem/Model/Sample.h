#pragma once
#include <string>
#include "../json/JsonValue.h"

struct Sample {
    int          id = 0;
    std::wstring name;
    int          avgProductionTime = 0;  // 분/개
    double       yield = 1.0;            // 0.0 ~ 1.0
    int          stock = 0;
    std::wstring createdAt;

    int  getId()        const { return id; }
    void setId(int i)         { id = i; }

    JsonValue    toJson()                   const;
    static Sample fromJson(const JsonValue& j);
};
