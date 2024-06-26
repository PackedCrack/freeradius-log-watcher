#include "defines.hpp"
#include "event_handler.hpp"


namespace
{
constexpr std::string_view KEY_RADACCT = "-radacct";
constexpr std::string_view KEY_RADIUS = "-radius";


[[nodiscard]] std::vector<std::filesystem::path> watch_accounting_log_files(
        CInotify& fileSystemMonitor,
        const std::vector<std::filesystem::directory_iterator>& watchedDirs)
{
    std::vector<std::filesystem::path> watchedFiles{};
    
    std::string todaysDate = common::todays_date();
    for(const std::filesystem::directory_iterator& iterator : watchedDirs)
    {
        for(const std::filesystem::directory_entry& file : iterator)
        {
            std::error_code err{};
            if(std::filesystem::is_regular_file(file.path(), err))
            {
                if(std::string_view{ file.path().c_str() }.contains(todaysDate))
                {
                    if(fileSystemMonitor.try_add_watch(file.path(), CWatch::EventMask::inModify))
                    {
                        LOG_INFO_FMT("Created a modification watch for file: \"{}\".", file.path().c_str());
                        watchedFiles.emplace_back(file);
                    }
                    else
                    {
                        LOG_ERROR_FMT("Failed to create watch for entry: \"{}\" when looking for the latest log file",
                                      file.path().c_str());
                    }
                }
            }
            else if(err)
            {
                LOG_ERROR_FMT("Failed to determine if entry: \"{}\" is is a file. Error message: \"{}\"",
                              file.path().c_str(),
                              err.message());
            }
        }
    }
    
    return watchedFiles;
}
[[nodiscard]] std::vector<std::filesystem::directory_iterator> watch_accounting_directories(
        CInotify& fileSystemMonitor,
        const std::filesystem::path& logDirectory)
{
    std::vector<std::filesystem::directory_iterator> watchedDirs{};
    
    for(const auto& entry : std::filesystem::directory_iterator(logDirectory))
    {
        std::error_code err{};
        if(entry.is_directory(err))
        {
            if(fileSystemMonitor.try_add_watch(entry.path(), CWatch::EventMask::inCreate))
            {
                LOG_INFO_FMT("Created a watch for file creations in directory: \"{}\"..", entry.path().c_str());
                watchedDirs.emplace_back(entry);
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
    
    return watchedDirs;
}
[[nodiscard]] CInotify make_filesystem_monitor()
{
    std::expected<CInotify, common::ErrorCode> result = CInotify::make_inotify();
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
        LOG_ERROR_FMT("Log directory: \"{}\" is not a directory..", path.c_str());
        valid = false;
    }
    else if(err)
    {
        LOG_ERROR_FMT("Error when trying to determine if log directory is a directory. Message: \"{}\"", err.message());
        valid = false;
    }
    
    return valid;
}
[[nodiscard]] std::filesystem::path validate(std::filesystem::path logDirectory)
{
    if(!validate_log_directory(logDirectory))
    {
        LOG_FATAL_FMT("Failed to validate log directory: \"{}\"", logDirectory.c_str());
    }
    
    return common::trim_trailing_seperator(logDirectory);
}
[[nodiscard]] std::filesystem::path make_accounting_directory_path(const std::unordered_map<std::string_view, std::string>& argCache)
{
    if(argCache.contains(KEY_RADACCT))
    {
        return validate(argCache.find(KEY_RADACCT)->second);
    }
    else
    {
        return validate("/var/log/freeradius/radacct");
    }
}
[[nodiscard]] std::filesystem::path make_radius_log_directory_path(const std::unordered_map<std::string_view, std::string>& argCache)
{
    if(argCache.contains(KEY_RADIUS))
    {
        return validate(argCache.find(KEY_RADIUS)->second);
    }
    else
    {
        return validate("/var/log/freeradius");
    }
}
[[nodiscard]] std::vector<std::filesystem::path> watch_radius_log_file(
        const std::unordered_map<std::string_view, std::string>& argCache,
        CInotify& fileSystemMonitor,
        std::vector<std::filesystem::path>& watchedFiles)
{
    std::filesystem::path directory = make_radius_log_directory_path(argCache);
    
    std::filesystem::path radiusLog = directory / "radius.log";
    std::error_code err{};
    if(std::filesystem::is_regular_file(radiusLog, err))
    {
        if(fileSystemMonitor.try_add_watch(radiusLog, CWatch::EventMask::inModify))
        {
            LOG_INFO_FMT("Created a modification watch for file: \"{}\".", radiusLog.c_str());
            watchedFiles.emplace_back(radiusLog);
        }
        else
        {
            LOG_ERROR_FMT("Failed to create watch for entry: \"{}\" when looking for the latest log file",
                          radiusLog.c_str());
        }
    }
    else if(err)
    {
        LOG_ERROR_FMT("Failed to find 'radius.log' in directory: \"{}\". Error message: \"{}\"",
                      radiusLog.c_str(),
                      err.message());
    }
    
    return watchedFiles;
}
void process_cmd_line_args(int argc, char** argv, std::unordered_map<std::string_view, std::string>& argCache)
{
    for (int32_t i = 0; i < argc; ++i)
    {
        std::string_view arg = argv[i];
        if(arg == KEY_RADACCT)
        {
            i = i + 1;
            
            std::filesystem::path customDir = common::trim_trailing_seperator(std::filesystem::path{ argv[i] });
            auto[iter, emplaced] = argCache.try_emplace(KEY_RADACCT, customDir.c_str());
            if(emplaced)
            {
                LOG_INFO_FMT("Using custom filepath to radacct directory: \"{}\"", iter->second);
            }
            else
            {
                LOG_WARN("Failed to add custom radacct directory to the argument cache..");
            }
        }
        else if (arg == KEY_RADIUS)
        {
            i = i + 1;
            
            std::filesystem::path customDir = common::trim_trailing_seperator(std::filesystem::path{ argv[i] });
            auto[iter, emplaced] = argCache.try_emplace(KEY_RADIUS, customDir.c_str());
            if(emplaced)
            {
                LOG_INFO_FMT("Using custom filepath to radius log directory: \"{}\"", iter->second);
            }
            else
            {
                LOG_WARN("Failed to add custom radius log directory to the argument cache..");
            }
        }
    }
}
}   // namespace


int main(int argc, char** argv)
{
    ASSERT_FMT(0 < argc, "ARGC is {} ?!", argc);

    try
    {
        std::unordered_map<std::string_view, std::string> argCache{};
        process_cmd_line_args(argc, argv, argCache);
        
        
        CInotify fileSystemMonitor = make_filesystem_monitor();
        std::vector<std::filesystem::directory_iterator> watchedDirs =
                watch_accounting_directories(fileSystemMonitor, make_accounting_directory_path(argCache));
        std::vector<std::filesystem::path> watchedFiles = watch_accounting_log_files(fileSystemMonitor, watchedDirs);
        watchedFiles = watch_radius_log_file(argCache, fileSystemMonitor, watchedFiles);
        
        fileSystemMonitor.update_event_handler(make_event_handler(watchedFiles));
        
        
        [[maybe_unused]] bool result = fileSystemMonitor.start_listening();
        
        // TODO: provide proper exit method so it can be gracefully exited and not just with CTRL - C
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        return EXIT_SUCCESS;
    }
    catch(const exception::fatal_error& err)
    {
        LOG_ERROR_FMT("Fatal exception: {}", err.what());
        return EXIT_FAILURE;
    }
}