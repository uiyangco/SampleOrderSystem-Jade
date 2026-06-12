#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>

class JsonValue {
public:
    using Null   = std::monostate;
    using Object = std::map<std::string, JsonValue>;
    using Array  = std::vector<JsonValue>;

private:
    using Variant = std::variant<Null, bool, double, std::string, Array, Object>;
    Variant v_;

public:
    JsonValue() : v_(Null{}) {}
    explicit JsonValue(bool b) : v_(b) {}
    JsonValue(int i) : v_(static_cast<double>(i)) {}
    JsonValue(double d) : v_(d) {}
    JsonValue(const std::string& s) : v_(s) {}
    JsonValue(std::string&& s) : v_(std::move(s)) {}
    JsonValue(const char* s) : v_(std::string(s)) {}

    static JsonValue makeObject() { JsonValue j; j.v_ = Object{}; return j; }
    static JsonValue makeArray()  { JsonValue j; j.v_ = Array{};  return j; }

    bool isNull()   const { return std::holds_alternative<Null>(v_); }
    bool isBool()   const { return std::holds_alternative<bool>(v_); }
    bool isNumber() const { return std::holds_alternative<double>(v_); }
    bool isString() const { return std::holds_alternative<std::string>(v_); }
    bool isArray()  const { return std::holds_alternative<Array>(v_); }
    bool isObject() const { return std::holds_alternative<Object>(v_); }

    bool        getBool()   const { return std::get<bool>(v_); }
    double      getNumber() const { return std::get<double>(v_); }
    int         getInt()    const { return static_cast<int>(std::get<double>(v_)); }
    const std::string& getString() const { return std::get<std::string>(v_); }
    const Array&  getArray()  const { return std::get<Array>(v_); }
    Array&        getArray()        { return std::get<Array>(v_); }
    const Object& getObject() const { return std::get<Object>(v_); }
    Object&       getObject()       { return std::get<Object>(v_); }

    bool contains(const std::string& key) const {
        return isObject() && std::get<Object>(v_).count(key) > 0;
    }

    JsonValue& operator[](const std::string& key) {
        if (isNull()) v_ = Object{};
        return std::get<Object>(v_)[key];
    }
    const JsonValue& operator[](const std::string& key) const {
        return std::get<Object>(v_).at(key);
    }
    JsonValue& operator[](size_t idx) {
        return std::get<Array>(v_)[idx];
    }
    const JsonValue& operator[](size_t idx) const {
        return std::get<Array>(v_)[idx];
    }

    void push_back(JsonValue val) {
        if (isNull()) v_ = Array{};
        std::get<Array>(v_).push_back(std::move(val));
    }

    size_t size() const {
        if (isArray())  return std::get<Array>(v_).size();
        if (isObject()) return std::get<Object>(v_).size();
        return 0;
    }
};
