//
// Created by hackerman on 1/28/24.
//
#include "CInotify.hpp"
#include "defines.hpp"



namespace
{
constexpr int32_t MOVED_FILE_DESCRIPTOR = -1;
constexpr size_t EVENT_SIZE = sizeof(inotify_event);
constexpr size_t AVG_NAME_LENGTH = 32u;
constexpr size_t LISTEN_BUFFER_SIZE = 512u * (sizeof(inotify_event) + AVG_NAME_LENGTH);
constexpr std::string_view TEMP_DIR = "/tmp/freerad-log-watcher";
constexpr std::string_view TEMP_FILE_NAME = "/tmp/freerad-log-watcher/thread-status";

CInotify::EventInfo make_event_info(const std::array<char, LISTEN_BUFFER_SIZE>& buffer, size_t offset)
{
    static_assert(std::is_trivially_copy_constructible_v<decltype(CInotify::EventInfo::wd)>);
    static_assert(std::is_trivially_copy_constructible_v<decltype(CInotify::EventInfo::mask)>);
    static_assert(std::is_trivially_copy_constructible_v<decltype(CInotify::EventInfo::cookie)>);
    static_assert(std::is_trivially_copy_constructible_v<decltype(CInotify::EventInfo::len)>);
    // Make sure the number of bytes copied are the same as sizeof amount of trivially members we have
    static_assert(
            (sizeof(decltype(CInotify::EventInfo::wd)) +
             sizeof(decltype(CInotify::EventInfo::mask)) +
             sizeof(decltype(CInotify::EventInfo::cookie)) +
             sizeof(decltype(CInotify::EventInfo::len)))
            == EVENT_SIZE);
    
    // The EventInfo struct is not trivially constructable since it contains a std::string..
    // But the trivial members are all the same size (32bit) so initialize them in a new array and direct assign them
    std::array<uint32_t, 4> trivialMembers{};
    static_assert(trivialMembers.size() <= EVENT_SIZE);
    std::memcpy(trivialMembers.data(), &buffer[offset], EVENT_SIZE);
    
    ASSERT(trivialMembers[0] <= INT32_MAX, "Unsigned/signed cast overflow");
    CInotify::EventInfo event{
            .wd = static_cast<int32_t>(trivialMembers[0]),
            .mask = CWatch::EventMask{ trivialMembers[1] },
            .cookie = trivialMembers[2],
            .len = trivialMembers[3],
            .name = std::string{ &buffer[offset + EVENT_SIZE], event.len }
    };
    
    // Remove any potential null terminators that the constructor may have put in the string content - it screws up logging
    while(event.name.ends_with(static_cast<char>(0)))
        event.name.pop_back();
    
    return event;
}

[[nodiscard]] std::filesystem::path create_tmp_filepath()
{
    for(size_t i = 0u; i < 100u; ++i)
    {
        std::string filename{ TEMP_FILE_NAME.data() + std::to_string(i) };
        if(!std::filesystem::exists(filename))
            return std::filesystem::path{ filename };
    }
    ASSERT(false, "100(!) temp files already created!");
    std::unreachable();
}
}   // namespace


std::expected<CInotify, common::ErrorCode> CInotify::make_inotify()
{
    try
    {
        return CInotify{};
    }
    catch(const std::runtime_error& err)
    {
        LOG_ERROR_FMT("{}", err.what());
        return std::unexpected{ common::ErrorCode::constructorError };
    }
}
std::expected<CInotify, common::ErrorCode> CInotify::make_inotify(
        std::function<void(CInotify& self, const EventInfo& info)> eventHandler)
{
    try
    {
        return CInotify{ std::move(eventHandler) };
    }
    catch(const std::runtime_error& err)
    {
        LOG_ERROR_FMT("{}", err.what());
        return std::unexpected{ common::ErrorCode::constructorError };
    }
}
void CInotify::copy_thread(const CInotify& other)
{
    if(other.m_CurrentState == State::listening)
    {
        if(!start_listening())
        {
            LOG_ERROR_FMT(
                    "Failed to start listening thread for inotify file descriptor: {}. Copy of inotify file descriptor: {}",
                    m_FileDescriptor,
                    other.m_FileDescriptor);
        }
    }
}
bool CInotify::move_thread(CInotify& other)
{
    if(other.m_CurrentState == State::listening)
    {
        if(!other.stop_listening())
        {
            LOG_ERROR_FMT(
                    "Failed to stop the listening thread when moving CInotify with File Descriptor: {}", other.m_FileDescriptor);
        }
        m_Watches.erase(other.m_ExitState);
        
        m_ExitState = std::move(other.m_ExitState);
        if(!start_listening())
        {
            LOG_ERROR_FMT(
                    "Failed to start the listening thread on the moved to CInotify with File Descriptor: {}", m_FileDescriptor);
        }
        
        return true;
    }
    
    return false;
}
CInotify CInotify::copy(const CInotify& other) const
{
    LOG_WARN("Called copy on CInotify.. Is this really what you want?");
    
    std::expected<CInotify, common::ErrorCode> result = make_inotify(other.m_EventHandler);
    if(!result)
        throw std::runtime_error{ std::format("Failed making a copy of CInotify with file descriptor: {}", m_FileDescriptor) };
    
    CInotify cpy = std::move(result.value());
    for(const auto& pair : other.m_Watches)
    {
        const CWatch& watch = pair.second;
        if(watch.watching() != other.m_ExitState)
        {
            if(!cpy.try_add_watch(std::string{ watch.watching() }, watch.event_mask()))
            {
                LOG_ERROR_FMT("Failed to copy watch: {}", watch.watching());
            }
        }
    }
    
    
    return cpy;
}
CInotify::CInotify()
        : m_FileDescriptor{ inotify_init() }
          , m_CurrentState{ State::idle }
          , m_ExitState{ create_tmp_filepath() }
          , m_ListeningThread{}
          , m_EventHandler{}
          , m_Watches{}
{
    if(m_FileDescriptor < 0)
    {
        throw std::runtime_error{ std::format("Failed to initialize Inotify: {}", common::str_error()) };
    }
    
    // If the /tmp/freerad-log-watcher/ directory doesnt exist we have to create it..
    std::error_code err{};
    if(!std::filesystem::exists(TEMP_DIR, err))
        std::filesystem::create_directory(TEMP_DIR);
    
    if(err)
        throw std::runtime_error{ err.message() };
}
CInotify::CInotify(std::function<void(CInotify& self, const EventInfo& info)>&& eventHandler)
    : m_FileDescriptor{ inotify_init() }
    , m_CurrentState{ State::idle }
    , m_ExitState{ create_tmp_filepath() }
    , m_ListeningThread{}
    , m_EventHandler{ std::move(eventHandler) }
    , m_Watches{}
{
    if(m_FileDescriptor < 0)
    {
        throw std::runtime_error{ std::format("Failed to initialize Inotify: {}", common::str_error()) };
    }
    
    // If the /tmp/freerad-log-watcher/ directory doesnt exist we have to create it..
    std::error_code err{};
    if(!std::filesystem::exists(TEMP_DIR, err))
        std::filesystem::create_directory(TEMP_DIR);
    
    if(err)
        throw std::runtime_error{ err.message() };
}
CInotify::~CInotify()
{
    if(m_FileDescriptor < 0)
        return;
    
    if(m_CurrentState == State::listening)
    {
        if(!stop_listening())
        {
            LOG_ERROR_FMT("Failed to stop the listening thread in destructor of: {}", m_ExitState.c_str());
        }
    }
    
    m_Watches.clear();
    int32_t result = close(m_FileDescriptor);
    if(result < 0)
    {
        LOG_ERROR_FMT("Failed to close File Descriptor: {}. Failed with: {}", m_FileDescriptor, common::str_error());
    }
}
CInotify::CInotify(const CInotify& other)
        : m_FileDescriptor{}
        , m_CurrentState{ }
        , m_ExitState{ }
        , m_ListeningThread{}
        , m_EventHandler{}
        , m_Watches{}
{
    *this = copy(other);
    copy_thread(other);
}
CInotify::CInotify(CInotify&& other) noexcept
    : m_FileDescriptor{ other.m_FileDescriptor }
    , m_CurrentState{ State::idle }
    , m_ExitState{}
    , m_ListeningThread{}
    , m_EventHandler{ std::move(other.m_EventHandler) }
    , m_Watches{ std::move(other.m_Watches) }
{
    other.m_FileDescriptor = MOVED_FILE_DESCRIPTOR;
    
    if(!move_thread(other))
        m_ExitState = std::move(other.m_ExitState);
}
CInotify& CInotify::operator=(const CInotify& other)
{
    if(this != &other)
    {
        *this = copy(other);
        copy_thread(other);
    }
    
    return *this;
}
CInotify& CInotify::operator=(CInotify&& other) noexcept
{
    if(this != &other)
    {
        m_FileDescriptor = other.m_FileDescriptor;
        other.m_FileDescriptor = MOVED_FILE_DESCRIPTOR;
        
        m_CurrentState = State::idle;
        m_EventHandler = std::move(other.m_EventHandler);
        m_Watches = std::move(other.m_Watches);
        if(!move_thread(other))
            m_ExitState = std::move(other.m_ExitState);
    }
    
    return *this;
}
auto CInotify::listen()
{
    return [this]()
    {
        ASSERT(m_EventHandler, "Event handler is missing..");
        
        std::array<char, LISTEN_BUFFER_SIZE> buffer{ 0 };
        bool listening = true;
        while(listening)
        {
            ssize_t bytesRead = read(m_FileDescriptor, buffer.data(), buffer.size());
            if (bytesRead < 0)
            {
                LOG_ERROR_FMT("Read error: {}", common::str_error());
            }
            else
            {
                for(ssize_t i = 0; i < bytesRead;)
                {
                    EventInfo event = make_event_info(buffer, static_cast<size_t>(i));
                    i = i + static_cast<ssize_t>(EVENT_SIZE) + event.len;
                    
                    
                    if(event.mask & CWatch::EventMask::inDeleteSelf || event.mask & CWatch::EventMask::inMoveSelf)
                    {
                        if(static_cast<std::string>(m_ExitState).contains(event.name))
                        {
                            listening = false;
                            continue;
                        }
                        
                        m_Watches.erase(event.name);
                    }
                    
                    if(m_EventHandler)
                        m_EventHandler(*this, event);
                }
            }
        }
    };
}
std::optional<const CWatch*> CInotify::find_watch(int32_t watchDescriptor) const
{
    auto iter = std::find_if(std::begin(m_Watches), std::end(m_Watches),
                 [watchDescriptor](const auto& pair)
                 {
                     const CWatch& watch = pair.second;
                     return watch.descriptor() == watchDescriptor;
                 });
    
    return iter != std::end(m_Watches) ? std::make_optional(&(iter->second)) : std::nullopt;
}
std::optional<std::filesystem::path> CInotify::find_watched_filepath(int32_t watchDescriptor) const
{
    const CWatch* pWatch = find_watch(watchDescriptor).value_or(nullptr);
    return pWatch != nullptr ? std::make_optional(std::filesystem::path{ pWatch->watching() }) : std::nullopt;
}
bool CInotify::erase_watch(const std::filesystem::path& path)
{
    bool erased = false;
    if(m_Watches.erase(path) > 0)
        erased = true;
    
    return erased;
}
bool CInotify::start_listening()
{
    ASSERT(m_CurrentState == State::idle, "This notify is already listening to events!");
    
    // Write working file
    std::fstream file{ common::trim_trailing_seperator(m_ExitState), std::ios::binary | std::ios::out | std::ios::trunc };
    if(!file.is_open())
    {
        LOG_ERROR_FMT("Failed to create tmp file required for listening: {}", m_ExitState.c_str());
        return false;
    }
    file << "exit:0";
    
    // add watch
    bool watchAdded = try_add_watch(m_ExitState, CWatch::EventMask::inDeleteSelf);
    if(!watchAdded)
    {
        LOG_ERROR_FMT("Failed to create a watch for tmp file: {}", m_ExitState.c_str());
        return false;
    }
    
    // Spawn thread
    try
    {
        m_ListeningThread = std::thread{ listen() };
        m_CurrentState = State::listening;
    }
    catch(const std::system_error& err)
    {
        m_Watches.erase(m_ExitState);
        LOG_ERROR_FMT("Failed to start listening thread! Message: {}", err.what());
    }
    
    return true;
}
bool CInotify::stop_listening()
{
    ASSERT(m_CurrentState == State::listening, "This notify is not listening to events!");
    
    // Join thread
    try
    {
        // erase working file
        std::error_code err{};
        bool erased = std::filesystem::remove( m_ExitState, err );
        if(!erased)
        {
            if(err)
            {
                LOG_ERROR_FMT("Failed to erase exit state file! Failed with: {}", err.message());
            }
            
            return false;
        }
        
        m_ListeningThread.join();
        m_Watches.erase(m_ExitState);
        m_CurrentState = State::idle;
    }
    catch(const std::system_error& err)
    {
        // Maybe this is fatal? The tmp file has been removed from disk at this point..
        LOG_ERROR_FMT("Failed to join the listening thread! Message: {}", err.what());
        return false;
    }
    
    
    return true;
}
bool CInotify::has_watch(const std::filesystem::path& path)
{
    return m_Watches.contains(path);
}