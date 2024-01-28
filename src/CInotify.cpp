//
// Created by hackerman on 1/28/24.
//
#include "CInotify.hpp"
#include "defines.hpp"
// linux
#include <unistd.h>


static constexpr int32_t MOVED_FILED_DESCRIPTOR = -1;

std::expected<CInotify, common::ErrorCode> CInotify::make_inotify()
{
    try
    {
        return CInotify{};
    }
    catch(const std::runtime_error& err)
    {
        return std::unexpected{ common::ErrorCode::constructorError };
    }
}

CInotify CInotify::copy() const
{

}

CInotify::CInotify()
    : m_FileDescriptor{ inotify_init() }
    , m_Watches{}
{
    if(m_FileDescriptor < 0)
    {
        throw std::runtime_error{ std::format("Failed to initialize Inotify: {}", common::str_error()) };
    }
}

CInotify::~CInotify()
{
    if(m_FileDescriptor < 0)
        return;
    
    m_Watches.clear();
    
    int32_t result = close(m_FileDescriptor);
    if(result < 0)
    {
        LOG_ERROR_FMT("Failed to close File Descriptor: {}. Failed with: {}", m_FileDescriptor, common::str_error());
    }
}

CInotify::CInotify(CInotify&& other) noexcept
    : m_FileDescriptor{ other.m_FileDescriptor }
    , m_Watches{ std::move(other.m_Watches) }
{
    other.m_FileDescriptor = MOVED_FILED_DESCRIPTOR;
}

CInotify& CInotify::operator=(CInotify&& other) noexcept
{
    if(this != &other)
    {
        m_FileDescriptor = other.m_FileDescriptor;
        other.m_FileDescriptor = MOVED_FILED_DESCRIPTOR;
        
        m_Watches = std::move(other.m_Watches);
    }
    
    return *this;
}
//bool CInotify::try_add_watch(std::string path, uint32_t eventMask)
//{
//    bool added = false;
//    std::expected<CWatch, common::ErrorCode> result = CWatch::make_watch(m_FileDescriptor, std::move(path), eventMask);
//    if(result)
//    {
//        m_Watches.insert(std::move(*result));
//        added = true;
//    }
//    else
//    {
//        LOG_ERROR_FMT("Failed to add watch: {}, for File Descriptor: {}. Failed with: {}",
//                      path,
//                      m_FileDescriptor,
//                      common::error_code_to_str(result.error()));
//    }
//
//    return added;
//}