//
// Created by hackerman on 1/28/24.
//

#pragma once
#include "defines.hpp"


namespace common
{
enum class ErrorCode
{
    constructorError
};

[[nodiscard]] constexpr std::string_view error_code_to_str(ErrorCode code)
{
    UNHANDLED_CASE_PROTECTION_ON
    switch(code)
    {
        case ErrorCode::constructorError:
        {
            static constexpr std::string_view asString = "Constructor Error";
            return asString;
        }
    }
    UNHANDLED_CASE_PROTECTION_OFF
    
    std::unreachable();
}

[[nodiscard]] inline const char* str_error()
{
    std::array<char, 256> buffer{ 0 };
    return strerror_r(errno, buffer.data(), buffer.size());
}
}   // namespace common