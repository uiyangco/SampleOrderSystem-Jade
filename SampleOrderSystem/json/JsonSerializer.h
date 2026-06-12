#pragma once
#include "JsonValue.h"
#include <string>

class JsonSerializer {
public:
    static std::string serialize(const JsonValue& v, bool pretty = true, int indent = 0);

private:
    static std::string serializeString(const std::string& s);
    static std::string indentStr(int level);
};
