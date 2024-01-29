//
// Created by hackerman on 1/28/24.
//

#pragma once
#include "CWatch.hpp"
// linux
#include <sys/inotify.h>

#include <unistd.h>

class CInotify final
{
public:
    struct EventInfo
    {
        int32_t wd;
        CWatch::EventMask mask;
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
private:
    CInotify();
public:
    [[nodiscard]] static std::expected<CInotify, common::ErrorCode> make_inotify();
    ~CInotify();
    CInotify(const CInotify& other);
    CInotify(CInotify&& other) noexcept;
    CInotify& operator=(const CInotify& other);
    CInotify& operator=(CInotify&& other) noexcept;
public:
    template<typename... mask_t> requires (std::is_same_v<std::remove_reference_t<mask_t>, CWatch::EventMask> && ...)
    [[nodiscard]] bool try_add_watch(std::string path, mask_t&&... args)
    {
        CWatch::EventMask eventMask = (std::forward<mask_t>(args) | ...);
        
        bool added = false;
        std::expected<CWatch, common::ErrorCode> result = CWatch::make_watch(m_FileDescriptor, std::move(path), eventMask);
        if(result)
        {
            auto[iter, emplaced] = m_Watches.try_emplace(std::string{ result.value().watching() }, std::move(*result));
            if(!emplaced)
            {
                LOG_ERROR_FMT("Failed to emplaced newly created watch for file: {}", result.value().watching());
            }
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
    void move_thread(CInotify& other);
    void copy_thread(const CInotify& other);
    [[nodiscard]] CInotify copy(const CInotify& other) const;
    [[nodiscard]] auto listen();
private:
    int32_t m_FileDescriptor;
    State m_CurrentState;
    std::filesystem::path m_ExitState;
    std::thread m_ListeningThread;
    std::function<void(const EventInfo& info)> m_EventHandler;
    std::unordered_map<std::string, CWatch> m_Watches;
};

