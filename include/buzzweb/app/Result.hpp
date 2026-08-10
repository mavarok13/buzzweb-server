#pragma once

#include <variant>
#include <utility>

namespace buzzweb::app {

template <typename SuccessType, typename ErrorType>
class Result {
public:
    static Result Ok(SuccessType value) {
        return Result(std::move(value));
    }

    static Result Err(ErrorType error) {
        return Result(std::move(error));
    }

    bool IsOk() const {
        return std::holds_alternative<SuccessType>(value_);
    }

    const SuccessType& Value() const {
        return std::get<SuccessType>(value_);
    }

    const ErrorType& Error() const {
        return std::get<ErrorType>(value_);
    }

private:
    explicit Result(SuccessType value) : value_(std::move(value)) {}
    explicit Result(ErrorType error) : value_(std::move(error)) {}

    std::variant<SuccessType, ErrorType> value_;
};

} //namespace buzzweb::app