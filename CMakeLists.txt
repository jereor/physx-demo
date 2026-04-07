cmake_minimum_required(VERSION 3.24)
project(physx_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pull in our finder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(PhysX REQUIRED)

#  Match PhysX's CRT linkage: /MT in Release, /MTd in Debug
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# --- The executable ---
add_executable(physx_demo main.cpp)
target_link_libraries(physx_demo PRIVATE PhysX::All)

# --- Runtime DLL copy ---
# Copies the three DLLs next to the exe after every build.
# Without this you'd need to add the PhysX bin dir to PATH manually.
add_custom_command(TARGET physx_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PHYSX_RUNTIME_DLLS}
        $<TARGET_FILE_DIR:physx_demo>
    COMMENT "Copying PhysX runtime DLLs"
)
