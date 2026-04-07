cmake_minimum_required(VERSION 3.24)
project(physx_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Point this at your PhysX repo root ---
set(PhysX_ROOT "C:/PhysX"
    CACHE PATH "Root of the cloned PhysX repository")

# Pull in our finder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(PhysX REQUIRED)

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
