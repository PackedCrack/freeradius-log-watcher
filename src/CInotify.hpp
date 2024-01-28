//
// Created by hackerman on 1/28/24.
//

#pragma once
#include "CWatch.hpp"
// linux
#include <sys/inotify.h>

#include <unistd.h>

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
            m_Watches.insert(std::move(*result));
            added = true;
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
    
    inline void test()
    {
        const size_t event_size = sizeof(struct inotify_event);
        const size_t buf_len = 1024 * (event_size + 16);
        char buffer[buf_len];
        
        while (true) {
            int length = read(m_FileDescriptor, buffer, buf_len);
            if (length < 0) {
                LOG_ERROR_FMT("Read error: {}", common::str_error());
                break;
            }
            
            for (int i = 0; i < length;) {
                struct inotify_event *event = (struct inotify_event *)&buffer[i];
                if (event->mask & IN_CREATE) {
                    std::printf("\nFile created: %s", event->name);
                }
                i += event_size + event->len;
            }
        }
    }
private:
    int32_t m_FileDescriptor;
    std::unordered_set<CWatch, CWatch::Hasher> m_Watches;
};