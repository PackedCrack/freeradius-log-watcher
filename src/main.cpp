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
                LOG_INFO_FMT("Using custom filepath to radacct directory: {}", iter->second);
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
        LOG_ERROR_FMT("Log directory: {}, does not exist.", path.c_str());
        valid = false;
    }
    else if(err)
    {
        LOG_ERROR_FMT("Error when trying to determine if log directory exist. Message: {}", err.message());
        valid = false;
    }
    else if(!std::filesystem::is_directory(path, err))
    {
        LOG_ERROR_FMT("Log directory: {}, does is not a directory..", path.c_str());
        valid = false;
    }
    else if(err)
    {
        LOG_ERROR_FMT("Error when trying to determine if log directory is a directory. Message: {}", err.message());
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
        LOG_FATAL_FMT("Failed to validate log directory: {}", logDirectory.c_str());
    }
    
    return logDirectory;
}

[[nodiscard]] CInotify make_filesystem_monitor()
{
    std::expected<CInotify, common::ErrorCode> result = CInotify::make_inotify(
            [](const CInotify::EventInfo& info){ LOG_WARN("TODO: Implemenet event handling"); });
    if(!result)
    {
        LOG_FATAL_FMT("FATAL: Failed to create Inotify. Failed with error code: {} - {}",
                      std::to_underlying(result.error()),
                      common::error_code_to_str(result.error()));
    }
    
    CInotify fsmonitor = std::move(*result);
    return fsmonitor;
}



void watch_ap_logs(CInotify& fileSystemMonitor, const std::filesystem::path& apDirectory)
{
    ASSERT(std::filesystem::is_directory(apDirectory), "The filepath to the access points log directory must be a directory!");
    
    for(const auto& entry : std::filesystem::directory_iterator(apDirectory))
    {
    
    }
}

void watch_ap_directories(CInotify& fileSystemMonitor, const std::filesystem::path& logDirectory)
{
    for(const auto& entry : std::filesystem::directory_iterator(logDirectory))
    {
        std::error_code err{};
        if(entry.is_directory(err))
        {
            if(!fileSystemMonitor.try_add_watch(entry.path(), CWatch::EventMask::inCreate))
            {
                LOG_ERROR_FMT("Failed to create watch for entry: {}, when iterating log directory: {}",
                              entry.path().c_str(),
                              logDirectory.c_str());
            }
            else
            {
                LOG_INFO_FMT("Created watch for entry: {}", entry.path().c_str());
                watch_ap_logs(fileSystemMonitor, entry.path());
            }
        }
        else if(err)
        {
            LOG_ERROR_FMT("Failed to determine if entry: {}, in log directory: {} is a directroy. Error message: {}",
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
        
        
        std::vector<std::filesystem::path> nas{};
        
        for(const auto& entry : std::filesystem::directory_iterator(logDirectory))
        {
            std::error_code err{};
            bool wasDir = entry.is_directory(err);
            if(err)
            {
                LOG_ERROR_FMT("Failed to iterate contents of the log directory. Failed with: {}", err.value());
            }
            else if(wasDir)
            {
                nas.push_back(entry);
            }
        }
        
        for(auto e : nas)
            std::printf("%s", e.c_str());
        
        
        std::expected<CInotify, common::ErrorCode> result2 = CInotify::make_inotify([](const CInotify::EventInfo& info){});
        if(result2)
        {
            [[maybe_unused]] bool added = result2.value().try_add_watch("/home/hackerman/Desktop/test2/", CWatch::EventMask::inCreate);
            
            
            if(!result2.value().start_listening())
            {
                LOG_WARN("Failed to start listening..");
                return EXIT_FAILURE;
            }
            
            
        }
        
        
        
        return EXIT_SUCCESS;
        
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