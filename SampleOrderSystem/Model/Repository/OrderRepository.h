#pragma once
#include "JsonRepository.h"
#include "../Order.h"
#include <functional>

class OrderRepository : public JsonRepository<Order> {
public:
    using EventCallback = std::function<void(const Order&)>;

    explicit OrderRepository(const std::string& filePath)
        : JsonRepository<Order>(filePath) {}

    void setEventCallback(EventCallback cb) { callback_ = std::move(cb); }

    bool create(Order& entity) override {
        bool ok = JsonRepository<Order>::create(entity);
        if (ok && callback_) callback_(entity);
        return ok;
    }

    bool update(const Order& entity) override {
        bool ok = JsonRepository<Order>::update(entity);
        if (ok && callback_) callback_(entity);
        return ok;
    }

private:
    EventCallback callback_;
};
