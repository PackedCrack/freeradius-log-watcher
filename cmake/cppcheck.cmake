add_custom_target(CPPCHECK
    COMMAND /bin/bash cppcheck.sh
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running CppCheck.."
    VERBATIM
)