//
// Created by hackerman on 1/28/24.
//

#pragma once
#include "CWatch.hpp"


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
    explicit CInotify(std::function<void(CInotify& self, const EventInfo& info)>&& eventHandler);
public:
    [[nodiscard]] static std::expected<CInotify, common::ErrorCode> make_inotify(
            std::function<void(CInotify& self, const EventInfo& info)> eventHandler);
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
                LOG_ERROR_FMT("Failed to emplace newly created watch for file: {}", result.value().watching());
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
    [[nodiscard]] std::optional<const CWatch*> find_watch(int32_t watchDescriptor) const;
    [[nodiscard]] std::optional<std::filesystem::path> watched_filepath(int32_t watchDescriptor) const;
    [[nodiscard]] bool erase_watch(const std::filesystem::path& path);
    [[nodiscard]] bool start_listening();
    [[nodiscard]] bool stop_listening();
    [[nodiscard]] bool has_watch(const std::filesystem::path& path);
private:
    void copy_thread(const CInotify& other);
    [[nodiscard]] bool move_thread(CInotify& other);
    [[nodiscard]] CInotify copy(const CInotify& other) const;
    [[nodiscard]] auto listen();
private:
    int32_t m_FileDescriptor;
    State m_CurrentState;
    std::filesystem::path m_ExitState;
    std::thread m_ListeningThread;
    std::function<void(CInotify& self, const EventInfo& info)> m_EventHandler;
    std::unordered_map<std::string, CWatch> m_Watches;
};

