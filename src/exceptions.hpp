#pragma once


namespace exception
{
    class fatal_error : public std::runtime_error
    {
    public:
        template<typename string_t>
        explicit fatal_error(string_t&& msg)
                : runtime_error{ msg }
                , m_Message{ std::forward<string_t>(msg) }
        {}
        [[nodiscard]] inline const char* what() const noexcept override
        {
            return m_Message.c_str();
        }
    private:
        std::string m_Message;
    };
}	// namespace exception