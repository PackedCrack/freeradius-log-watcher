//
// Created by hackerman on 1/28/24.
//

#pragma once
#include "common.hpp"
// linux
#include <sys/inotify.h>


class CWatch final
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
    
    struct Hasher
    {
        size_t operator()(const CWatch& owner) const;
    };
private:
    CWatch(int32_t fd, std::string path, EventMask eventMask);
public:
    [[nodiscard]] static std::expected<CWatch, common::ErrorCode> make_watch(int32_t fd, std::string path, EventMask eventMask);
    ~CWatch();
    CWatch(const CWatch& other) = delete;
    CWatch(CWatch&& other) noexcept;
    CWatch& operator=(const CWatch& other) = delete;
    CWatch& operator=(CWatch&& other) noexcept;
public:
    friend bool operator==(const CWatch& lhs, const CWatch& rhs);
    [[nodiscard]] size_t operator()() const;
    [[nodiscard]] std::string_view watching() const;
    [[nodiscard]] EventMask event_mask() const;
private:
    int32_t m_FileDescriptor;
    int32_t m_WatchDescriptor;
    EventMask m_EventMask;
    std::string m_WatchedFile;
};

inline size_t operator&(CWatch::EventMask lhs, CWatch::EventMask rhs)
{
    return std::to_underlying(lhs) & std::to_underlying(rhs);
}

inline CWatch::EventMask operator|(CWatch::EventMask lhs, CWatch::EventMask rhs)
{
    return CWatch::EventMask{ std::to_underlying(lhs) | std::to_underlying(rhs) };
}