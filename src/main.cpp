#include "defines.hpp"


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


}   // namespace

#include "CInotify.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    ASSERT_FMT(0 < argc, "ARGC is {} ?!", argc);

    try
    {
        std::unordered_map<std::string_view, std::string> argCache{};
        process_cmd_line_args(argc, argv, argCache);
        
        std::filesystem::path logDirectory{};
        if(argCache.contains(KEY_RADACCT))
            logDirectory = argCache.find(KEY_RADACCT)->second;
        else
            logDirectory = "/var/log/freeradius/radacct";
        
        
        
        
        std::expected<CInotify, common::ErrorCode> result = CInotify::make_inotify(
                [](const CInotify::EventInfo& info){ LOG_WARN("TODO: Implemenet event handling"); });
        if(!result)
        {
            LOG_FATAL_FMT("FATAL: Failed to create Inotify. Failed with error code: {} - {}",
                          std::to_underlying(result.error()),
                          common::error_code_to_str(result.error()));
        }
        CInotify fileSystemMonitor = std::move(*result);
        
        
        
        
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