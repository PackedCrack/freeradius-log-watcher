SET(MAIN_SRCS 
        main.cpp
        common.hpp
        CInotify.cpp
        CInotify.hpp
        CWatch.cpp
        CWatch.hpp
)

target_sources(${MAIN_PROJECT} PRIVATE ${MAIN_SRCS})