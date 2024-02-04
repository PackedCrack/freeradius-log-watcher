//
// Created by hackerman on 2/4/24.
//
#include "CParser.hpp"
#include "defines.hpp"


void CParser::parse_file(std::ifstream& file, std::ifstream::pos_type startPosition) const
{
    ASSERT(file.is_open(), "File parameter can't be parsed since its not opened!");
    
    file.seekg(startPosition);
    
    std::string line{};
    while(std::getline(file, line))
    {
        std::string::size_type headerStart = line.find_first_not_of("\t ");
        std::string::size_type headerEnd = line.find(" = ") - 1;
        if(headerStart != std::string::npos && headerEnd != std::string::npos)
        {
            if(m_Headers.contains(line.substr(headerStart, headerEnd)))
                std::printf("\n%s", line.c_str());
        }
    }
}