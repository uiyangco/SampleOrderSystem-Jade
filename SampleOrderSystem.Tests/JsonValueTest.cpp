#include <gtest/gtest.h>
#include "json/JsonValue.h"
#include "json/JsonParser.h"
#include "json/JsonSerializer.h"

// ── JsonValue 기본 타입 ──────────────────────────────────────────────────────

TEST(JsonValueTest, DefaultConstruct_IsNull) {
    JsonValue v;
    EXPECT_TRUE(v.isNull());
}

TEST(JsonValueTest, ConstructFromBool_True) {
    JsonValue v(true);
    EXPECT_TRUE(v.isBool());
    EXPECT_TRUE(v.getBool());
}

TEST(JsonValueTest, ConstructFromBool_False) {
    JsonValue v(false);
    EXPECT_TRUE(v.isBool());
    EXPECT_FALSE(v.getBool());
}

TEST(JsonValueTest, ConstructFromInt) {
    JsonValue v(42);
    EXPECT_TRUE(v.isNumber());
    EXPECT_EQ(v.getInt(), 42);
}

TEST(JsonValueTest, ConstructFromDouble) {
    JsonValue v(3.14);
    EXPECT_TRUE(v.isNumber());
    EXPECT_DOUBLE_EQ(v.getNumber(), 3.14);
}

TEST(JsonValueTest, ConstructFromString) {
    JsonValue v(std::string("hello"));
    EXPECT_TRUE(v.isString());
    EXPECT_EQ(v.getString(), "hello");
}

// ── Object / Array ───────────────────────────────────────────────────────────

TEST(JsonValueTest, MakeObject_IsObject) {
    JsonValue obj = JsonValue::makeObject();
    EXPECT_TRUE(obj.isObject());
}

TEST(JsonValueTest, MakeArray_IsArray) {
    JsonValue arr = JsonValue::makeArray();
    EXPECT_TRUE(arr.isArray());
}

TEST(JsonValueTest, Object_StoreAndRetrieve) {
    JsonValue obj = JsonValue::makeObject();
    obj["key"] = JsonValue(std::string("value"));
    EXPECT_TRUE(obj.contains("key"));
    EXPECT_EQ(obj["key"].getString(), "value");
}

TEST(JsonValueTest, Object_Contains_MissingKey_ReturnsFalse) {
    JsonValue obj = JsonValue::makeObject();
    EXPECT_FALSE(obj.contains("missing"));
}

TEST(JsonValueTest, Array_PushBack_And_Access) {
    JsonValue arr = JsonValue::makeArray();
    arr.push_back(JsonValue(1));
    arr.push_back(JsonValue(2));
    EXPECT_EQ(arr.size(), 2u);
    EXPECT_EQ(arr[0].getInt(), 1);
    EXPECT_EQ(arr[1].getInt(), 2);
}

// ── JsonParser ───────────────────────────────────────────────────────────────

TEST(JsonParserTest, Parse_Null) {
    JsonValue v = JsonParser::parse("null");
    EXPECT_TRUE(v.isNull());
}

TEST(JsonParserTest, Parse_BoolTrue) {
    JsonValue v = JsonParser::parse("true");
    ASSERT_TRUE(v.isBool());
    EXPECT_TRUE(v.getBool());
}

TEST(JsonParserTest, Parse_BoolFalse) {
    JsonValue v = JsonParser::parse("false");
    ASSERT_TRUE(v.isBool());
    EXPECT_FALSE(v.getBool());
}

TEST(JsonParserTest, Parse_Integer) {
    JsonValue v = JsonParser::parse("42");
    ASSERT_TRUE(v.isNumber());
    EXPECT_EQ(v.getInt(), 42);
}

TEST(JsonParserTest, Parse_String) {
    JsonValue v = JsonParser::parse("\"hello\"");
    ASSERT_TRUE(v.isString());
    EXPECT_EQ(v.getString(), "hello");
}

TEST(JsonParserTest, Parse_SimpleObject) {
    JsonValue v = JsonParser::parse(R"({"id": 1, "name": "test"})");
    ASSERT_TRUE(v.isObject());
    EXPECT_EQ(v["id"].getInt(), 1);
    EXPECT_EQ(v["name"].getString(), "test");
}

TEST(JsonParserTest, Parse_NestedObject) {
    JsonValue v = JsonParser::parse(R"({"nextId": 3, "data": []})");
    ASSERT_TRUE(v.isObject());
    EXPECT_EQ(v["nextId"].getInt(), 3);
    EXPECT_TRUE(v["data"].isArray());
}

TEST(JsonParserTest, Parse_ArrayOfObjects) {
    JsonValue v = JsonParser::parse(R"([{"id":1},{"id":2}])");
    ASSERT_TRUE(v.isArray());
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0]["id"].getInt(), 1);
    EXPECT_EQ(v[1]["id"].getInt(), 2);
}

// ── JsonSerializer ───────────────────────────────────────────────────────────

TEST(JsonSerializerTest, Serialize_Null) {
    EXPECT_EQ(JsonSerializer::serialize(JsonValue{}), "null");
}

TEST(JsonSerializerTest, Serialize_Bool) {
    EXPECT_EQ(JsonSerializer::serialize(JsonValue(true)),  "true");
    EXPECT_EQ(JsonSerializer::serialize(JsonValue(false)), "false");
}

TEST(JsonSerializerTest, Serialize_Integer) {
    EXPECT_EQ(JsonSerializer::serialize(JsonValue(7)), "7");
}

TEST(JsonSerializerTest, Serialize_String) {
    EXPECT_EQ(JsonSerializer::serialize(JsonValue(std::string("hi"))), "\"hi\"");
}

TEST(JsonSerializerTest, Serialize_EmptyObject) {
    EXPECT_EQ(JsonSerializer::serialize(JsonValue::makeObject(), false), "{}");
}

TEST(JsonSerializerTest, Serialize_EmptyArray) {
    EXPECT_EQ(JsonSerializer::serialize(JsonValue::makeArray(), false), "[]");
}

TEST(JsonSerializerTest, RoundTrip_Object) {
    JsonValue obj = JsonValue::makeObject();
    obj["id"]   = JsonValue(1);
    obj["name"] = JsonValue(std::string("Alice"));
    std::string json = JsonSerializer::serialize(obj, false);
    JsonValue parsed = JsonParser::parse(json);
    EXPECT_EQ(parsed["id"].getInt(), 1);
    EXPECT_EQ(parsed["name"].getString(), "Alice");
}
