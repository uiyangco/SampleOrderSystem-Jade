#pragma once
#include "JsonValue.h"
#include <string_view>

class JsonParser {
public:
    static JsonValue parse(std::string_view input);

private:
    explicit JsonParser(std::string_view input) : input_(input), pos_(0) {}

    JsonValue   parseValue();
    JsonValue   parseObject();
    JsonValue   parseArray();
    std::string parseString();
    JsonValue   parseNumber();
    JsonValue   parseLiteral();
    void        skipWhitespace();
    char        peek() const;
    char        consume();

    std::string_view input_;
    size_t           pos_;
};
