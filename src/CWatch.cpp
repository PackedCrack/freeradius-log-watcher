//
// Created by hackerman on 1/28/24.
//
#include "CWatch.hpp"
#include "defines.hpp"
// linux
#include <sys/inotify.h>


static constexpr int32_t MOVED_FILE_DESCRIPTOR = -1;
static constexpr int32_t MOVED_WATCH_DESCRIPTOR = -1;


size_t CWatch::Hasher::operator()(const CWatch& owner) const
{
    return std::hash<std::string>{}( owner.m_WatchedFile );
}

std::expected<CWatch, common::ErrorCode> CWatch::make_watch(int32_t fd, std::string path, uint32_t eventMask)
{
    try
    {
        return CWatch{ fd, std::move(path), eventMask };
    }
    catch(const std::runtime_error& err)
    {
        return std::unexpected{ common::ErrorCode::constructorError };
    }
}

CWatch::CWatch(int32_t fd, std::string path, uint32_t eventMask)
    : m_FileDescriptor{ fd }
    , m_WatchDescriptor{ inotify_add_watch(m_FileDescriptor, path.c_str(), eventMask) }
    , m_EventMask{ eventMask }
    , m_WatchedFile{ std::move(path) }
{
    if(m_WatchDescriptor < 0)
    {
        throw std::runtime_error{ std::format("Failed to create watch: {}. For file descriptor: {}. Error: {}",
                                              m_WatchedFile,
                                              m_FileDescriptor,
                                              common::str_error()) };
    }
}

CWatch::~CWatch()
{
    if(m_FileDescriptor < 0)
        return;
    
    // Inotify apperantly handles deletion of SOME events.......
    if(m_EventMask & IN_MOVE_SELF || m_EventMask & IN_DELETE_SELF)
        return;
    
    int32_t result = inotify_rm_watch(m_FileDescriptor, m_WatchDescriptor);
    if(result < 0)
    {
        LOG_ERROR_FMT("Error when trying to remove Watch Descriptor: {}, from File Descriptor: {}. Failed with: {}",
                      m_WatchDescriptor,
                      m_FileDescriptor,
                      common::str_error());
    }
}

CWatch::CWatch(CWatch&& other) noexcept
        : m_FileDescriptor{ other.m_FileDescriptor }
        , m_WatchDescriptor{ other.m_WatchDescriptor }
        , m_EventMask{ other.m_EventMask }
        , m_WatchedFile{ std::move(other.m_WatchedFile) }
{
    other.m_FileDescriptor = MOVED_FILE_DESCRIPTOR;
    other.m_WatchDescriptor = MOVED_WATCH_DESCRIPTOR;
}

CWatch& CWatch::operator=(CWatch&& other) noexcept
{
    if(this != &other)
    {
        m_FileDescriptor = other.m_FileDescriptor;
        other.m_FileDescriptor = MOVED_FILE_DESCRIPTOR;
        m_WatchDescriptor = other.m_WatchDescriptor;
        other.m_WatchDescriptor = MOVED_WATCH_DESCRIPTOR;
        m_EventMask = other.m_EventMask;
        
        m_WatchedFile = std::move(other.m_WatchedFile);
    }
    
    return *this;
}

size_t CWatch::operator()(const CWatch& self) const
{
    return Hasher{}(*this);
}

bool operator==(const CWatch& lhs, const CWatch& rhs)
{
    if(lhs.m_FileDescriptor != rhs.m_FileDescriptor)
        return false;
    if(lhs.m_WatchDescriptor != rhs.m_WatchDescriptor)
        return false;
    if(lhs.m_WatchedFile != rhs.m_WatchedFile)
        return false;
    
    
    return true;
}

std::string_view CWatch::watching() const
{
    return m_WatchedFile;
}