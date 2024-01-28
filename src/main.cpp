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
        
        
        std::expected<CInotify, common::ErrorCode> result = CInotify::make_inotify();
        if(result)
        {
            result.value().try_add_watch("/home/hackerman/Desktop/test2/", CInotify::EventMask::inCreate);
            
            LOG_INFO("Listeing for events..");
            result.value().test();
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