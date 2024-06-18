//
// Created by hackerman on 2/4/24.
//
#include "CParser.hpp"
#include "defines.hpp"

namespace
{
void green_text()
{
    std::printf("\033[92m");
}
void red_text()
{
    std::printf("\033[91m");
}
void handle_acct_type_str(std::string_view acctType)
{
    if(acctType.contains("Start"))
    {
        green_text();
    }
    else if(acctType.contains("Stop"))
    {
        red_text();
    }
}
[[nodiscard]] std::string_view as_formatted_view(const std::string& line)
{
    std::string_view printableLine = line;
    std::string_view::size_type offset{};
    while(std::string_view{ std::begin(printableLine) + offset, std::end(printableLine) }.starts_with('\t'))
    {
        ++offset;
    }
    std::string_view::size_type count = printableLine.size() - offset;
    printableLine = printableLine.substr(offset, count);
    
    return printableLine;
}
//void clear_color()
//{
//    std::printf("\n \033[0m");
//}
}
void CParser::parse_file(std::ifstream& file, std::ifstream::pos_type startPosition) const
{
    ASSERT(file.is_open(), "File parameter can't be parsed since its not opened!");
    
    file.seekg(startPosition);
    
    std::printf("\n");
    
    std::string line{};
    while(std::getline(file, line))
    {
        std::string::size_type headerStart = line.find_first_not_of("\t ");
        std::string::size_type headerEnd = line.find(" = ");
        if(headerStart != std::string::npos && headerEnd != std::string::npos)
        {
            std::string::size_type count = headerEnd - headerStart;
            std::string header = line.substr(headerStart, count);
            // Print only the lines from the log that exist in the member cache
            if(m_Headers.contains(header))
            {
                if(header == "Acct-Status-Type")
                {
                    handle_acct_type_str(line);
                }
                
                std::printf("\n%s", as_formatted_view(line).data());
            }
        }
    }
}