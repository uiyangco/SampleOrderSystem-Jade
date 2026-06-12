#include "JsonSerializer.h"

// Stub: always returns empty string — tests will fail
std::string JsonSerializer::serialize(const JsonValue&, bool, int) { return ""; }
std::string JsonSerializer::serializeString(const std::string&)    { return ""; }
std::string JsonSerializer::indentStr(int)                         { return ""; }
