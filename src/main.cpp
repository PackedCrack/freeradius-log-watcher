#include "defines.hpp"


namespace
{
void process_cmd_line_args(int argc, char** argv)
{
    if (argc < 1)
        LOG_FATAL_FMT("Not enough command line arguments given, expected at least 1.", argc);

    std::vector<std::string> arguments{};
    arguments.reserve(static_cast<std::size_t>(argc));
    for (int32_t i = 0; i < argc; ++i)
    {
        arguments.emplace_back(argv[i]);
    }


    for (std::string_view arg : arguments)
    {
        // TODO
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
        process_cmd_line_args(argc, argv);
        
        
        std::vector<std::filesystem::path> nas{};
        
        std::filesystem::path logDirectory = "/var/log/freeradius/radacct";
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
        
        
        std::expected<CInotify, common::ErrorCode> result = CInotify::make_inotify([](const CInotify::EventInfo& info){});
        if(result)
        {
            [[maybe_unused]] bool added = result.value().try_add_watch("/home/hackerman/Desktop/test2/", CWatch::EventMask::inCreate);
            
            
            if(!result.value().start_listening())
            {
                LOG_WARN("Failed to start listening..");
                return EXIT_FAILURE;
            }
            
            CInotify testcpy = result.value();
            CInotify testcpy2{ testcpy };
            CInotify testcpy4{ testcpy };
            
            CInotify testcpy3{ std::move(result.value()) };
            CInotify testcpy5 = std::move(testcpy4);
            
            
            LOG_INFO("Listeing for events..");
            while(true)
            {
                std::string inp{};
                std::cin >> inp;
                if(inp == "exit")
                {
                    if(testcpy3.stop_listening())
                    {
                        if(testcpy2.stop_listening())
                        {
                            if(testcpy5.stop_listening())
                            {
                                if(testcpy.stop_listening())
                                {
                                    return EXIT_SUCCESS;
                                }
                            }
                        }
                    }
                    
                    return EXIT_FAILURE;
                }
            }
        }
        
        
        
        return 0;
        
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