#include "defines.hpp"
#include "CInotify.hpp"

#include "CParser.hpp"


std::unique_ptr<CParser> g_pParser = nullptr;



using FilePositionCache = std::unordered_map<std::string, std::ifstream::pos_type>;

namespace
{
constexpr std::string_view KEY_RADACCT = "radacct";
struct FileWithSize
{
    std::ifstream file;
    std::ifstream::pos_type size;
};

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
void parse_modified_log(
        std::unordered_map<std::string, std::ifstream::pos_type>& prevFilePositions,
        const std::filesystem::path& modifiedLog)
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
        g_pParser->parse_file(file, position);
        
        // Update cached fileseeker value with active fileseeker's value
        iter->second = fileSize;
    }
    else
    {
        LOG_ERROR_FMT("Failed to open file: \"{}\" when trying to do event action for EventMask::inModify",
                      modifiedLog.c_str());
    }
}
void parse_created_log(
        std::unordered_map<std::string, std::ifstream::pos_type>& prevFilePositions,
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
        
        g_pParser->parse_file(file);
    }
    else
    {
        LOG_ERROR_FMT("Failed parse newly created file: \"{}\" because it could not be opened", newLog.c_str());
    }
}
void find_and_erase_old_log(
        CInotify& self,
        std::unordered_map<std::string, std::ifstream::pos_type>& filePosCache,
        std::filesystem::path& watchDir,
        const CInotify::EventInfo& info)
{
    auto oldLogPath = watchDir / std::filesystem::path{ common::previous_date(0) };
    for(int32_t i = 1u; i < 30; ++i) // only check one month back..
    {
        oldLogPath.replace_filename(common::previous_date(i));
        if(info.name.contains(oldLogPath.c_str()))
        {
            if(self.has_watch(oldLogPath))
            {
                // delete watch for previous log file watch cache
                if(!self.erase_watch(oldLogPath))
                {
                    LOG_ERROR_FMT("Failed to erase watch of yesterdays log: \"{}\"", oldLogPath.c_str());
                }
                else
                {
                    LOG_INFO_FMT("Erasing watch of yesterdays log: \"{}\"", oldLogPath.c_str());
                }
                // delete watch for previous log file fileseeker cache
                filePosCache.erase(oldLogPath);
                break;
            }
        }
    }
}
template<typename invocable_t> requires std::is_invocable_v<invocable_t, std::filesystem::path&&>
void do_event_action(CInotify& self, const CInotify::EventInfo& info, invocable_t&& action)
{
    std::optional<std::filesystem::path> result = self.watched_filepath(info.wd);
    if(result)
    {
        action(std::move(*result));
    }
    else
    {
        LOG_WARN_FMT("Failed to find a watch for Watch Descriptor: \"{}\". With name file: \"{}\".",
                     info.wd,
                     info.name);
    }
}
[[nodiscard]] auto event_handler(FilePositionCache& prevFilePositions)
{
    return [&prevFilePositions](auto&& self, const CInotify::EventInfo& info) mutable
    {
        if(info.mask & CWatch::EventMask::inCreate)
        {
            do_event_action(self, info, [&self, &prevFilePositions, &info](std::filesystem::path&& watchDir)
            {
                // Loop linearly over previous days
                find_and_erase_old_log(self, prevFilePositions, watchDir, info);
                
                // Get todays date
                if(info.name.contains(common::todays_date()))
                {
                    std::filesystem::path newlyCreatedLog = common::trim_trailing_seperator(watchDir / info.name);
                    parse_created_log(prevFilePositions, newlyCreatedLog);
                }
                else
                {
                    LOG_WARN_FMT("Expected newly created file: \"{}\" to contain today date: \"{}\".",
                                 info.name,
                                 common::todays_date());
                }
            });
        }
        else if(info.mask & CWatch::EventMask::inModify)
        {
            do_event_action(self, info, [&info, &prevFilePositions](std::filesystem::path&& watchDir)
            {
                std::filesystem::path logFilepath =
                        common::trim_trailing_seperator(watchDir / std::filesystem::path{ info.name });
                
                parse_modified_log(prevFilePositions, logFilepath);
            });
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
void cache_file_size_as_start_pos(FilePositionCache& cache, const std::filesystem::path& filepath)
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
void watch_log_files(
        CInotify& fileSystemMonitor,
        const std::filesystem::path& apDirectory,
        FilePositionCache& prevFilePositions)
{
    ASSERT(std::filesystem::is_directory(apDirectory), "The filepath to the access points log directory must be a directory!");
    
    std::string todaysDate = common::todays_date();
    for(const auto& entry : std::filesystem::directory_iterator(apDirectory))
    {
        std::error_code err{};
        if(std::filesystem::is_regular_file(entry.path(), err))
        {
            if(std::string_view{ entry.path().c_str() }.contains(todaysDate))
            {
                if(fileSystemMonitor.try_add_watch(entry.path(), CWatch::EventMask::inModify))
                {
                    LOG_INFO_FMT("Created a modification watch for file: \"{}\".", entry.path().c_str());
                    
                    cache_file_size_as_start_pos(prevFilePositions, entry.path());
                    break;
                }
                else
                {
                    LOG_ERROR_FMT("Failed to create watch for entry: \"{}\" when looking for the latest log file",
                                  entry.path().c_str());
                }
            }
        }
        else if(err)
        {
            LOG_ERROR_FMT("Failed to determine if entry: \"{}\" is is a file. Error message: \"{}\"",
                          entry.path().c_str(),
                          err.message());
        }
    }
}
void watch_ap_directories(
        CInotify& fileSystemMonitor,
        const std::filesystem::path& logDirectory,
        FilePositionCache& prevFilePositions)
{
    for(const auto& entry : std::filesystem::directory_iterator(logDirectory))
    {
        std::error_code err{};
        if(entry.is_directory(err))
        {
            if(fileSystemMonitor.try_add_watch(entry.path(), CWatch::EventMask::inCreate))
            {
                LOG_INFO_FMT("Created a watch for file creations in directory: \"{}\"..", entry.path().c_str());
                watch_log_files(fileSystemMonitor, entry.path(), prevFilePositions);
            }
            else
            {
                LOG_ERROR_FMT("Failed to create watch for entry: \"{}\" when iterating log directory: \"{}\"",
                              entry.path().c_str(),
                              logDirectory.c_str());
            }
        }
        else if(err)
        {
            LOG_ERROR_FMT("Failed to determine if entry: \"{}\" in log directory: \"{}\" is a directroy. Error message: \"{}\"",
                          entry.path().c_str(),
                          logDirectory.c_str(),
                          err.message());
        }
    }
}
[[nodiscard]] CInotify make_filesystem_monitor(FilePositionCache& prevFilePositions)
{
    std::expected<CInotify, common::ErrorCode> result = CInotify::make_inotify(event_handler(prevFilePositions));
    if(!result)
    {
        LOG_FATAL_FMT("FATAL: Failed to create Inotify. Failed with error code: \"{}\" - \"{}\"",
                      std::to_underlying(result.error()),
                      common::error_code_to_str(result.error()));
    }
    
    CInotify fsmonitor = std::move(*result);
    return fsmonitor;
}
[[nodiscard]] bool validate_log_directory(const std::filesystem::path& path)
{
    bool valid = true;
    
    std::error_code err{};
    if(!std::filesystem::exists(path, err))
    {
        LOG_ERROR_FMT("Log directory: \"{}\" does not exist.", path.c_str());
        valid = false;
    }
    else if(err)
    {
        LOG_ERROR_FMT("Error when trying to determine if log directory exist. Message: \"{}\"", err.message());
        valid = false;
    }
    else if(!std::filesystem::is_directory(path, err))
    {
        LOG_ERROR_FMT("Log directory: \"{}\" does is not a directory..", path.c_str());
        valid = false;
    }
    else if(err)
    {
        LOG_ERROR_FMT("Error when trying to determine if log directory is a directory. Message: \"{}\"", err.message());
        valid = false;
    }
    
    return valid;
}
[[nodiscard]] std::filesystem::path make_log_directory_path(const std::unordered_map<std::string_view, std::string>& argCache)
{
    std::filesystem::path logDirectory{};
    if(argCache.contains(KEY_RADACCT))
        logDirectory = argCache.find(KEY_RADACCT)->second;
    else
        logDirectory = "/var/log/freeradius/radacct";
    
    
    if(!validate_log_directory(logDirectory))
    {
        LOG_FATAL_FMT("Failed to validate log directory: \"{}\"", logDirectory.c_str());
    }
    
    return logDirectory;
}
void process_cmd_line_args(int argc, char** argv, std::unordered_map<std::string_view, std::string>& argCache)
{
    for (int32_t i = 0; i < argc; ++i)
    {
        std::string_view arg = argv[i];
        if(arg == "-radacct")
        {
            i = i + 1;
            auto[iter, emplaced] = argCache.try_emplace(KEY_RADACCT, std::string{ argv[i] });
            if(emplaced)
            {
                LOG_INFO_FMT("Using custom filepath to radacct directory: \"{}\"", iter->second);
            }
            else
            {
                LOG_WARN("Failed to add custom radacct directory to the argument cache..");
            }
        }
    }
}
}   // namespace


#include <iostream>

int main(int argc, char** argv)
{
    ASSERT_FMT(0 < argc, "ARGC is {} ?!", argc);

    try
    {
        std::unordered_map<std::string_view, std::string> argCache{};
        process_cmd_line_args(argc, argv, argCache);
        
        std::filesystem::path logDirectory = make_log_directory_path(argCache);
        
        
        CParserBuilder builder{};
        g_pParser = std::make_unique<CParser>(builder
                .event_timestamp()
                .user_name()
                .acct_status_type()
                .acct_terminate_cause()
                .nas_ip_address()
                .mobility_domain_id()
                .build());
        
        
        
        FilePositionCache prevFilePos{};
        CInotify fileSystemMonitor = make_filesystem_monitor(prevFilePos);
        watch_ap_directories(fileSystemMonitor, logDirectory, prevFilePos);
        
        [[maybe_unused]] bool result = fileSystemMonitor.start_listening();
        
        bool exit = false;
        while (!exit)
        {
        
        }

        return EXIT_SUCCESS;
    }
    catch(const exception::fatal_error& err)
    {
        LOG_ERROR_FMT("Fatal exception: {}", err.what());
        return EXIT_FAILURE;
    }
}