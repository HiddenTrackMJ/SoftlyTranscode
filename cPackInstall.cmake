
#vInstallCpack.cmake
###install
set(CPACK_PACKAGE_NAME  "transcode")
set(CPACK_PACKAGE_DIRECTORY "./")
set(CPACK_PACKAGE_DESCRIPTION "A TEST")

install(FILES [FileName] DESTINATION [InstallFoldName])
install(TARGETS [ExeName] [LibName] 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

###cpack
string(TIMESTAMP vTimeStamp "%Y%m%d%H%M%S")
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE vGitCommit
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE vGitBranch
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${vTimeStamp}-${vGitBranch}-${vGitCommit}")
include(CPack)