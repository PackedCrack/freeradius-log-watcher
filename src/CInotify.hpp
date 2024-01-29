//
// Created by hackerman on 1/28/24.
//

#pragma once
#include "CWatch.hpp"
// linux
#include <sys/inotify.h>

#include <unistd.h>
#include <thread>
#include <iostream>

class CInotify
{
public:
    enum class EventMask : uint32_t
    {
        inAccess = IN_ACCESS,
        inModify = IN_MODIFY,
        inAttrib = IN_ATTRIB,
        inCloseWrite = IN_CLOSE_WRITE,
        inCloseNoWrite = IN_CLOSE_NOWRITE,
        inOpen = IN_OPEN,
        inMovedFrom = IN_MOVED_FROM,
        inMovedTo = IN_MOVED_TO,
        inCreate = IN_CREATE,
        inDelete = IN_DELETE,
        inDeleteSelf = IN_DELETE_SELF,
        inMoveSelf = IN_MOVE_SELF,
        inUnmount = IN_UNMOUNT,
        inQOverflow = IN_Q_OVERFLOW,
        inIgnored = IN_IGNORED,
        inOnlyDir = IN_ONLYDIR,
        inDontFollow = IN_DONT_FOLLOW,
        inIsDir = IN_ISDIR,
        inOneshot = IN_ONESHOT,
        inMaskAdd = IN_MASK_ADD
    };
    struct EventInfo
    {
        int32_t wd;
        EventMask mask;
        uint32_t cookie;
        uint32_t len;
        std::string name;
    };
private:
    enum class State
    {
        idle,
        listening
    };
protected:
    CInotify();
public:
    [[nodiscard]] static std::expected<CInotify, common::ErrorCode> make_inotify();
    [[nodiscard]] CInotify copy() const;
    ~CInotify();
    CInotify(const CInotify& other) = delete;
    CInotify(CInotify&& other) noexcept;
    CInotify& operator=(const CInotify& other) = delete;
    CInotify& operator=(CInotify&& other) noexcept;
public:
    template<typename... mask_t> requires (std::is_same_v<std::remove_reference_t<mask_t>, EventMask> && ...)
    [[nodiscard]] bool try_add_watch(std::string path, mask_t&&... args)
    {
        uint32_t eventMask = (std::to_underlying(std::forward<mask_t>(args)) | ...);
        
        bool added = false;
        std::expected<CWatch, common::ErrorCode> result = CWatch::make_watch(m_FileDescriptor, std::move(path), eventMask);
        if(result)
        {
            auto[iter, emplaced] = m_Watches.try_emplace(std::string{ result.value().watching() }, std::move(*result));
            added = emplaced;
        }
        else
        {
            LOG_ERROR_FMT("Failed to add watch: {}, for File Descriptor: {}. Failed with: {}",
                          path,
                          m_FileDescriptor,
                          common::error_code_to_str(result.error()));
        }
        
        return added;
    }
    [[nodiscard]] bool start_listening();
    [[nodiscard]] bool stop_listening();
private:
    auto listen();
private:
    int32_t m_FileDescriptor;
    State m_CurrentState;
    std::filesystem::path m_ExitState;
    std::thread m_ListeningThread;
    std::function<void(const EventInfo& info)> m_EventHandler;
    std::unordered_map<std::string, CWatch> m_Watches;
};


inline size_t operator&(CInotify::EventMask lhs, CInotify::EventMask rhs)
{
    return std::to_underlying(lhs) & std::to_underlying(rhs);
}