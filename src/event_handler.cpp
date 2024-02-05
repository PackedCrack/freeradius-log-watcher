//
// Created by hackerman on 2/4/24.
//
#include "event_handler.hpp"
#include "CInotify.hpp"
#include "CParser.hpp"


namespace
{
enum class ErrorCode
{
    noPrevLogFound
};
struct FileWithSize
{
    std::ifstream file;
    std::ifstream::pos_type size;
};
[[nodiscard]] CParser make_parser()
{
    CParserBuilder builder{};
    
    return CParser{ builder
            .event_timestamp()
            .user_name()
            .acct_status_type()
            .acct_terminate_cause()
            .nas_ip_address()
            .mobility_domain_id()
            .build()
    };
}
[[nodiscard]] auto log_wd_filepath_lookup_failure(const CInotify::EventInfo& info)
{
    return [&info]() -> std::optional<std::filesystem::path>
    {
        LOG_WARN_FMT("Failed to find a watch for Watch Descriptor: \"{}\", with name: \"{}\".",
                     info.wd,
                     info.name);
        
        return std::nullopt;
    };
}
[[nodiscard]] std::optional<uintmax_t> get_file_size(const std::filesystem::path& filepath)
{
    std::error_code err{};
    uintmax_t fileSize = std::filesystem::file_size(filepath, err);
    if(err)
    {
        LOG_ERROR_FMT("Failed to retrieve filesize for file: {}. Error message: {}",
                      filepath.c_str(),
                      err.message());
        
        return std::nullopt;
    }
    
    return std::make_optional(fileSize);
}
[[nodiscard]] std::optional<FileWithSize> open_file_with_size(const std::filesystem::path& filepath)
{
    std::ifstream file{ filepath, std::ios::binary | std::ios::in };
    if(file.is_open())
    {
        std::optional<uintmax_t> result = get_file_size(filepath);
        if(result)
        {
            return std::make_optional(FileWithSize{
                    .file = std::move(file),
                    .size = static_cast<std::ifstream::pos_type>(*result)
            });
        }
    }
    
    return std::nullopt;
}
[[nodiscard]] FilePositionCache make_file_pos_cache(const std::vector<std::filesystem::path>& watchedFiles)
{
    FilePositionCache cache{};
    for(const std::filesystem::path& filepath : watchedFiles)
    {
        std::optional<uintmax_t> result = get_file_size(filepath);
        if(result)
        {
            auto[iter, emplaced] = cache.try_emplace(filepath, static_cast<std::ifstream::pos_type>(*result));
            if(!emplaced)
            {
                LOG_ERROR_FMT("Failed to cache starting position for {}", filepath.c_str());
            }
        }
        else
        {
            LOG_ERROR_FMT("Can't cache a previous starting position for {}", filepath.c_str());
        }
    }
    
    return cache;
}
void parse_created_log(
        FilePositionCache& prevFilePositions,
        const CParser& parser,
        const std::filesystem::path& newLog)
{
    // Parse new log file
    std::optional<FileWithSize> result = open_file_with_size(newLog);
    if(result)
    {
        std::ifstream& file = result.value().file;
        std::ifstream::pos_type fileSize = result.value().size;
        
        // Add new log file to fileseeker cache
        auto[iter, emplaced] = prevFilePositions.try_emplace(newLog, fileSize);
        if(!emplaced)
        {
            LOG_ERROR_FMT(
                    "Failed to cache file position after reading newly created file: \"{}\"", newLog.c_str());
        }
        
        parser.parse_file(file);
    }
    else
    {
        LOG_ERROR_FMT("Failed parse newly created file: \"{}\" because it could not be opened", newLog.c_str());
    }
}
[[nodiscard]] auto log_old_log_lookup_failure()
{
    return []([[maybe_unused]] ErrorCode err) -> std::expected<std::filesystem::path, ErrorCode>
    {
        LOG_WARN("Failed to find watch for old log! Continuing without deleting a previous watch...");
        return std::unexpected{ err };
    };
}
[[nodiscard]] auto do_modified_file_action(FilePositionCache& prevFilePositions, const CParser& parser)
{
    return [&prevFilePositions, &parser](const std::filesystem::path& modifiedLog) -> std::optional<std::filesystem::path>
    {
        // Open log file
        std::optional<FileWithSize> result = open_file_with_size(modifiedLog);
        if(result)
        {
            std::ifstream& file = result.value().file;
            std::ifstream::pos_type fileSize = result.value().size;
            
            // We can assume that a previous file position is always cached before modified events.
            auto iter = prevFilePositions.find(modifiedLog);
            ASSERT(iter != std::end(prevFilePositions), "Modify event handler is missing cached file position!");
            
            // Parse file
            std::ifstream::pos_type position = iter->second;
            parser.parse_file(file, position);
            
            // Update cached fileseeker value with active fileseeker's value
            iter->second = fileSize;
        }
        else
        {
            LOG_ERROR_FMT("Failed to open file: \"{}\" when trying to do event action for EventMask::inModify",
                          modifiedLog.c_str());
        }
        
        return modifiedLog;
    };
}
[[nodiscard]] auto erase_old_log(CInotify& self, FilePositionCache& prevFilePositions)
{
    return [&self, &prevFilePositions](const std::filesystem::path& oldLog) -> std::expected<std::filesystem::path, ErrorCode>
    {
        if(self.erase_watch(oldLog))
        {
            LOG_INFO_FMT("Erasing watch of yesterdays log: \"{}\"", oldLog.c_str());
        }
        else
        {
            LOG_ERROR_FMT("Failed to erase watch of yesterdays log: \"{}\"", oldLog.c_str());
        }
        // delete watch for previous log file fileseeker cache
        prevFilePositions.erase(oldLog);
        
        return oldLog;
    };
}
[[nodiscard]] std::expected<std::filesystem::path, ErrorCode> find_old_log(
        CInotify& self,
        const CInotify::EventInfo& info,
        const std::filesystem::path& watchDir)
{
    std::filesystem::path oldLog = watchDir / std::filesystem::path{ common::todays_date() };
    for(int32_t i = 1u; i < 30; ++i) // only check one month back..
    {
        oldLog.replace_filename(common::previous_date(i));
        if(info.name.contains(oldLog.c_str()))
        {
            if(self.has_watch(oldLog))
                return oldLog;
        }
    }

    return std::unexpected{ ErrorCode::noPrevLogFound };
}
[[nodiscard]] auto do_file_creation_action(
        FilePositionCache& prevFilePositions,
        const CParser& parser,
        CInotify& self,
        const CInotify::EventInfo& info)
{
    return [&prevFilePositions, &parser, &self, &info](const std::filesystem::path& watchDir) -> std::optional<std::filesystem::path>
    {
        find_old_log(self, info, watchDir)
        .and_then(erase_old_log(self, prevFilePositions))
        .or_else(log_old_log_lookup_failure());
        
        
        // Get todays date
        if(info.name.contains(common::todays_date()))
        {
            std::filesystem::path newlyCreatedLog = common::trim_trailing_seperator(watchDir / info.name);
            parse_created_log(prevFilePositions, parser, newlyCreatedLog);
        }
        else
        {
            LOG_WARN_FMT("Expected newly created file: \"{}\" to contain today date: \"{}\".",
                         info.name,
                         common::todays_date());
        }
        
        return watchDir;
    };
}
}   // namespace


std::function<void(CInotify& self, const CInotify::EventInfo& info)> make_event_handler(
        const std::vector<std::filesystem::path>& watchedFiles)
{
    return[prevFilePositions = make_file_pos_cache(watchedFiles),parser = make_parser()](
            auto&& self, const CInotify::EventInfo& info) mutable
    {
        if(info.mask & CWatch::EventMask::inCreate)
        {
            self.find_watched_filepath(info.wd)
                    .and_then(do_file_creation_action(prevFilePositions, parser, self, info))
                    .or_else(log_wd_filepath_lookup_failure(info));
        }
        else if(info.mask & CWatch::EventMask::inModify)
        {
            self.find_watched_filepath(info.wd)
                    .and_then(do_modified_file_action(prevFilePositions, parser))
                    .or_else(log_wd_filepath_lookup_failure(info));
        }
        else if(info.mask & CWatch::EventMask::inIgnored)
        {
            LOG_INFO_FMT("Received ignore event from watch descriptor: \"{}\" with name: \"{}\"",
                         info.wd,
                         info.name);
        }
        else
        {
            LOG_WARN_FMT("Unknown event mask received: \"{}\"", std::to_underlying(info.mask));
        }
    };
}