SET(MAIN_SRCS 
        main.cpp
        common.hpp
        CInotify.cpp
        CInotify.hpp
        CWatch.cpp
        CWatch.hpp
        CParser.cpp
        CParser.hpp
        event_handler.cpp
        event_handler.hpp
)

target_sources(${MAIN_PROJECT} PRIVATE ${MAIN_SRCS})