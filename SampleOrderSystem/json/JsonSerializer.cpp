#include "JsonSerializer.h"
#include <sstream>
#include <cmath>

std::string JsonSerializer::indentStr(int level) {
    return std::string(static_cast<size_t>(level) * 2, ' ');
}

std::string JsonSerializer::serializeString(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:   out += c;      break;
        }
    }
    out += '"';
    return out;
}

std::string JsonSerializer::serialize(const JsonValue& v, bool pretty, int indent) {
    if (v.isNull())   return "null";
    if (v.isBool())   return v.getBool() ? "true" : "false";
    if (v.isString()) return serializeString(v.getString());

    if (v.isNumber()) {
        double d = v.getNumber();
        if (d == std::floor(d) && std::abs(d) < 1e15)
            return std::to_string(static_cast<long long>(d));
        std::ostringstream oss;
        oss << d;
        return oss.str();
    }

    if (v.isArray()) {
        const auto& arr = v.getArray();
        if (arr.empty()) return "[]";
        std::string out = "[";
        for (size_t i = 0; i < arr.size(); ++i) {
            if (pretty) out += "\n" + indentStr(indent + 1);
            out += serialize(arr[i], pretty, indent + 1);
            if (i + 1 < arr.size()) out += ",";
        }
        if (pretty) out += "\n" + indentStr(indent);
        out += "]";
        return out;
    }

    if (v.isObject()) {
        const auto& obj = v.getObject();
        if (obj.empty()) return "{}";
        std::string out = "{";
        bool first = true;
        for (const auto& [key, val] : obj) {
            if (!first) out += ",";
            if (pretty) out += "\n" + indentStr(indent + 1);
            out += serializeString(key) + ": " + serialize(val, pretty, indent + 1);
            first = false;
        }
        if (pretty) out += "\n" + indentStr(indent);
        out += "}";
        return out;
    }

    return "null";
}
