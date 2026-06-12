#include "JsonParser.h"
#include <stdexcept>
#include <cctype>

// Stub: always returns null — tests will fail
JsonValue JsonParser::parse(std::string_view) {
    return JsonValue{};
}

void        JsonParser::skipWhitespace() {}
char        JsonParser::peek()    const  { return '\0'; }
char        JsonParser::consume()        { return '\0'; }
JsonValue   JsonParser::parseValue()     { return {}; }
JsonValue   JsonParser::parseObject()    { return {}; }
JsonValue   JsonParser::parseArray()     { return {}; }
std::string JsonParser::parseString()    { return {}; }
JsonValue   JsonParser::parseNumber()    { return {}; }
JsonValue   JsonParser::parseLiteral()   { return {}; }
