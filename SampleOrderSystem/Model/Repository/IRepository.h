#pragma once
#include <optional>
#include <vector>

template<typename T>
class IRepository {
public:
    virtual ~IRepository() = default;
    virtual bool             create(T& entity)       = 0;
    virtual std::optional<T> read(int id)            = 0;
    virtual std::vector<T>   readAll()               = 0;
    virtual bool             update(const T& entity) = 0;
    virtual bool             remove(int id)          = 0;
};
