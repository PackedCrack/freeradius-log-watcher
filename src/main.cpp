#include "defines.hpp"
#include "CInotify.hpp"


namespace
{
constexpr std::string_view KEY_RADACCT = "radacct";


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

[[nodiscard]] std::string previous_date(uint32_t daysToSubtract)
{
    std::chrono::sys_days date = std::chrono::year_month_day{ floor<std::chrono::days>(std::chrono::system_clock::now()) };
    if(daysToSubtract > 0u)
        date -= std::chrono::days{ daysToSubtract };
    
    
    std::string dateAsStr = std::format("{}", date);
    std::erase_if(dateAsStr, [](char c){ return c == '-'; });
    
    return dateAsStr;
}

[[nodiscard]] std::string todays_date()
{
    return previous_date(0u);
}

[[nodiscard]] auto event_handler()
{
    using PositionCache = std::unordered_map<std::string, std::ifstream::pos_type>;
    return [filePosCache = PositionCache{}](auto&& self, const CInotify::EventInfo& info) mutable
    {
        if(info.mask & CWatch::EventMask::inCreate)
        {
            std::optional<std::filesystem::path> result = self.watched_filepath(info.wd);
            if(!result)
            {
                LOG_WARN_FMT("Failed to find a watch for Watch Descriptor: \"{}\". With name file: \"{}\".",
                             info.wd,
                             info.name);
                return;
            }
            
            std::filesystem::path watchDirectory = std::move(*result);
            
            // Loop linearly over previous days
            auto oldLogPath = watchDirectory / std::filesystem::path{ previous_date(0) };
            for(uint32_t i = 1u; i < 30u; ++i) // only check one month back..
            {
                oldLogPath.replace_filename(previous_date(i));
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
            
            
            
            
            
            // Get todays date
            if(!info.name.contains(todays_date()))
            {
                LOG_WARN_FMT("Expected newly created file: \"{}\" to contain today date: \"{}\".",
                             info.name,
                             todays_date());
            }
            else
            {
                std::filesystem::path newLogPath = watchDirectory / info.name;
                
                // create watch for todays log file
                if(!self.try_add_watch(newLogPath, CWatch::EventMask::inModify))
                {
                    LOG_ERROR_FMT("Failed to create watch for newly created file: \"{}\"", info.name);
                }
                // Parse new log file
                std::ifstream file{ newLogPath, std::ios::binary | std::ios::in };
                if(file.is_open())
                {
                    std::string line{};
                    while(std::getline(file, line))
                    {
                        std::printf("%s", line.c_str());
                    }
                    
                    // Add new log file to fileseeker cacheauto
                    LOG_INFO_FMT("Caching file position: \"{}\"", static_cast<ssize_t>(file.tellg()));
                    auto[iter, emplaced] = filePosCache.try_emplace(newLogPath, file.tellg());
                    if(!emplaced)
                    {
                        LOG_WARN_FMT(
                                "Failed to cache file position after reading newly created file: \"{}\"", newLogPath.c_str());
                    }
                }
                else
                {
                    LOG_ERROR_FMT("Failed parse newly created file: \"{}\" because it could not be opened", newLogPath.c_str());
                }
            }
        }
        else if(info.mask & CWatch::EventMask::inModify)
        {
            LOG_INFO("TODO:: HANDLE FILE MODIFICATION EVENT");
            // Open log file
            // Get previous fileseeker from cache
            // Set update fileseeker to cached fileseeker value
            // Parse file
            // Update cached fileseeker value with active fileseeker's value
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

[[nodiscard]] CInotify make_filesystem_monitor()
{
    std::expected<CInotify, common::ErrorCode> result = CInotify::make_inotify(event_handler());
    if(!result)
    {
        LOG_FATAL_FMT("FATAL: Failed to create Inotify. Failed with error code: \"{}\" - \"{}\"",
                      std::to_underlying(result.error()),
                      common::error_code_to_str(result.error()));
    }
    
    CInotify fsmonitor = std::move(*result);
    return fsmonitor;
}

void watch_log_file(CInotify& fileSystemMonitor, const std::filesystem::path& apDirectory)
{
    ASSERT(std::filesystem::is_directory(apDirectory), "The filepath to the access points log directory must be a directory!");
    
    std::string todaysDate = todays_date();
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

void watch_ap_directories(CInotify& fileSystemMonitor, const std::filesystem::path& logDirectory)
{
    for(const auto& entry : std::filesystem::directory_iterator(logDirectory))
    {
        std::error_code err{};
        if(entry.is_directory(err))
        {
            if(fileSystemMonitor.try_add_watch(entry.path(), CWatch::EventMask::inCreate))
            {
                LOG_INFO_FMT("Created a watch for file creations in directory: \"{}\"..", entry.path().c_str());
                watch_log_file(fileSystemMonitor, entry.path());
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
        CInotify fileSystemMonitor = make_filesystem_monitor();
        watch_ap_directories(fileSystemMonitor, logDirectory);
        
        
        fileSystemMonitor.start_listening();
        
        
                
        //return EXIT_SUCCESS;
        
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