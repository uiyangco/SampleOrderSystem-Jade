#include "JsonParser.h"
#include <stdexcept>
#include <cctype>
#include <cstdlib>

JsonValue JsonParser::parse(std::string_view input) {
    JsonParser p(input);
    p.skipWhitespace();
    return p.parseValue();
}

void JsonParser::skipWhitespace() {
    while (pos_ < input_.size() && std::isspace((unsigned char)input_[pos_]))
        ++pos_;
}

char JsonParser::peek() const {
    if (pos_ >= input_.size()) throw std::runtime_error("Unexpected end of JSON");
    return input_[pos_];
}

char JsonParser::consume() {
    if (pos_ >= input_.size()) throw std::runtime_error("Unexpected end of JSON");
    return input_[pos_++];
}

JsonValue JsonParser::parseValue() {
    skipWhitespace();
    char c = peek();
    if (c == '{') return parseObject();
    if (c == '[') return parseArray();
    if (c == '"') return JsonValue(parseString());
    if (c == 't' || c == 'f' || c == 'n') return parseLiteral();
    return parseNumber();
}

JsonValue JsonParser::parseObject() {
    consume(); // '{'
    JsonValue obj = JsonValue::makeObject();
    skipWhitespace();
    if (peek() == '}') { consume(); return obj; }
    while (true) {
        skipWhitespace();
        std::string key = parseString();
        skipWhitespace();
        if (consume() != ':') throw std::runtime_error("Expected ':'");
        obj[key] = parseValue();
        skipWhitespace();
        char c = consume();
        if (c == '}') break;
        if (c != ',') throw std::runtime_error("Expected ',' or '}'");
    }
    return obj;
}

JsonValue JsonParser::parseArray() {
    consume(); // '['
    JsonValue arr = JsonValue::makeArray();
    skipWhitespace();
    if (peek() == ']') { consume(); return arr; }
    while (true) {
        arr.push_back(parseValue());
        skipWhitespace();
        char c = consume();
        if (c == ']') break;
        if (c != ',') throw std::runtime_error("Expected ',' or ']'");
    }
    return arr;
}

std::string JsonParser::parseString() {
    consume(); // '"'
    std::string result;
    while (true) {
        char c = consume();
        if (c == '"') break;
        if (c == '\\') {
            char esc = consume();
            switch (esc) {
            case '"':  result += '"';  break;
            case '\\': result += '\\'; break;
            case '/':  result += '/';  break;
            case 'n':  result += '\n'; break;
            case 'r':  result += '\r'; break;
            case 't':  result += '\t'; break;
            default:   result += esc;  break;
            }
        } else {
            result += c;
        }
    }
    return result;
}

JsonValue JsonParser::parseNumber() {
    size_t start = pos_;
    if (pos_ < input_.size() && input_[pos_] == '-') ++pos_;
    while (pos_ < input_.size() && std::isdigit((unsigned char)input_[pos_])) ++pos_;
    if (pos_ < input_.size() && input_[pos_] == '.') {
        ++pos_;
        while (pos_ < input_.size() && std::isdigit((unsigned char)input_[pos_])) ++pos_;
    }
    if (pos_ < input_.size() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
        ++pos_;
        if (pos_ < input_.size() && (input_[pos_] == '+' || input_[pos_] == '-')) ++pos_;
        while (pos_ < input_.size() && std::isdigit((unsigned char)input_[pos_])) ++pos_;
    }
    std::string s(input_.substr(start, pos_ - start));
    return JsonValue(std::stod(s));
}

JsonValue JsonParser::parseLiteral() {
    if (input_.substr(pos_, 4) == "true")  { pos_ += 4; return JsonValue(true); }
    if (input_.substr(pos_, 5) == "false") { pos_ += 5; return JsonValue(false); }
    if (input_.substr(pos_, 4) == "null")  { pos_ += 4; return JsonValue{}; }
    throw std::runtime_error("Unknown literal");
}
