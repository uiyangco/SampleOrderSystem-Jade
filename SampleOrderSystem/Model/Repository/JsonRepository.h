#pragma once
#include "IRepository.h"
#include "../../json/JsonValue.h"
#include "../../json/JsonParser.h"
#include "../../json/JsonSerializer.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <ranges>

template<typename T>
class JsonRepository : public IRepository<T> {
public:
    explicit JsonRepository(const std::string& filePath)
        : filePath_(filePath) { load(); }

    bool create(T& entity) override {
        entity.setId(nextId_++);
        data_.push_back(entity);
        return save();
    }

    std::optional<T> read(int id) override {
        auto it = std::ranges::find_if(data_,
            [id](const T& e) { return e.getId() == id; });
        if (it == data_.end()) return std::nullopt;
        return *it;
    }

    std::vector<T> readAll() override { return data_; }

    bool update(const T& entity) override {
        auto it = std::ranges::find_if(data_,
            [&](const T& e) { return e.getId() == entity.getId(); });
        if (it == data_.end()) return false;
        *it = entity;
        return save();
    }

    bool remove(int id) override {
        auto it = std::ranges::find_if(data_,
            [id](const T& e) { return e.getId() == id; });
        if (it == data_.end()) return false;
        data_.erase(it);
        return save();
    }

protected:
    std::vector<T> data_;
    int            nextId_ = 1;
    std::string    filePath_;

    void load() {
        if (!std::filesystem::exists(filePath_)) return;
        std::ifstream file(filePath_);
        std::string content(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        if (content.empty()) return;
        try {
            JsonValue root = JsonParser::parse(content);
            if (root.contains("nextId")) nextId_ = root["nextId"].getInt();
            if (root.contains("data") && root["data"].isArray())
                for (const auto& item : root["data"].getArray())
                    data_.push_back(T::fromJson(item));
        } catch (...) {}
    }

    bool save() {
        auto dir = std::filesystem::path(filePath_).parent_path();
        if (!dir.empty()) std::filesystem::create_directories(dir);

        std::ofstream file(filePath_);
        if (!file) return false;

        JsonValue arr = JsonValue::makeArray();
        for (const auto& item : data_)
            arr.push_back(item.toJson());

        JsonValue root = JsonValue::makeObject();
        root["nextId"] = JsonValue(nextId_);
        root["data"]   = std::move(arr);

        file << JsonSerializer::serialize(root);
        return true;
    }
};
