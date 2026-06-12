#include "Sample.h"
#include "../Utils.h"

JsonValue Sample::toJson() const {
    JsonValue j = JsonValue::makeObject();
    j["id"]                = JsonValue(id);
    j["name"]              = JsonValue(Utils::wstringToUtf8(name));
    j["avgProductionTime"] = JsonValue(avgProductionTime);
    j["yield"]             = JsonValue(yield);
    j["stock"]             = JsonValue(stock);
    j["createdAt"]         = JsonValue(Utils::wstringToUtf8(createdAt));
    return j;
}

Sample Sample::fromJson(const JsonValue& j) {
    Sample s;
    s.id                = j["id"].getInt();
    s.name              = Utils::utf8ToWstring(j["name"].getString());
    s.avgProductionTime = j["avgProductionTime"].getInt();
    s.yield             = j["yield"].getNumber();
    s.stock             = j["stock"].getInt();
    s.createdAt         = Utils::utf8ToWstring(j["createdAt"].getString());
    return s;
}
