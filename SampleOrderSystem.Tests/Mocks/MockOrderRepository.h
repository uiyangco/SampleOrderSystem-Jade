#pragma once
#include <gmock/gmock.h>
#include "Model/Repository/IRepository.h"
#include "Model/Order.h"

class MockOrderRepository : public IRepository<Order> {
public:
    MOCK_METHOD(bool,              create,  (Order&),              (override));
    MOCK_METHOD(std::optional<Order>, read, (int),                (override));
    MOCK_METHOD(std::vector<Order>,   readAll, (),                 (override));
    MOCK_METHOD(bool,              update,  (const Order&),        (override));
    MOCK_METHOD(bool,              remove,  (int),                 (override));
};
