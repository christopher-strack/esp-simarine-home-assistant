idf_component_register()

include(FetchContent)

FetchContent_Declare(
    spymarine
    GIT_REPOSITORY git@github.com:christopher-strack/spymarine-cpp.git
    GIT_TAG v0.1.0
)

FetchContent_MakeAvailable(spymarine)

target_link_libraries(${COMPONENT_LIB} INTERFACE spymarine)
