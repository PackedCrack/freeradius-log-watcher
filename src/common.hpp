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

template<typename int_t> requires std::integral<int_t>
[[nodiscard]] std::string previous_date(int_t daysToSubtract)
{
    std::chrono::sys_days date = std::chrono::year_month_day{ floor<std::chrono::days>(std::chrono::system_clock::now()) };
    if(daysToSubtract > 0)
        date -= std::chrono::days{ daysToSubtract };
    
    
    std::string dateAsStr = std::format("{}", date);
    std::erase_if(dateAsStr, [](char c){ return c == '-'; });
    
    return dateAsStr;
}

[[nodiscard]] inline std::string todays_date()
{
    static constexpr int32_t DAYS_TO_SUBTRACT = 0;
    return previous_date(DAYS_TO_SUBTRACT);
}

template<typename path_t> requires std::same_as<std::remove_reference_t<path_t>, std::filesystem::path>
[[nodiscard]] std::filesystem::path trim_trailing_seperator(path_t&& filepath)
{
    if(std::string_view{ filepath.c_str() }.ends_with(std::filesystem::path::preferred_separator))
    {
        std::string tmp{ std::forward<path_t>(filepath) };
        tmp.pop_back();
        filepath = std::filesystem::path{ std::move(tmp) };
    }
    
    return filepath;
}
}   // namespace common