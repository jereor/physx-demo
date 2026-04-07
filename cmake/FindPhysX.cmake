# cmake/FindPhysX.cmake
# Locates a PhysX build output directory and defines imported targets.
# Usage: set(PhysX_ROOT "...") before calling find_package(PhysX)

set(PHYSX_BIN_DIR "${PhysX_ROOT}/physx/bin/win.x86_64.vc143.mt/release")
set(PHYSX_INC_DIR "${PhysX_ROOT}/physx/include")

# Verify the paths exist so you get a clear error instead of a linker mystery
if(NOT EXISTS "${PHYSX_BIN_DIR}")
    message(FATAL_ERROR "PhysX bin dir not found: ${PHYSX_BIN_DIR}\nSet PhysX_ROOT correctly.")
endif()
if(NOT EXISTS "${PHYSX_INC_DIR}")
    message(FATAL_ERROR "PhysX include dir not found: ${PHYSX_INC_DIR}\nSet PhysX_ROOT correctly.")
endif()

# --- Imported targets ---
# We define one target per DLL-backed library. CMake imported targets are the
# clean way to carry both the .lib (linker input) and .dll (runtime) together.

function(add_physx_dll_library target_name lib_name)
    add_library(${target_name} SHARED IMPORTED GLOBAL)
    set_target_properties(${target_name} PROPERTIES
        IMPORTED_IMPLIB   "${PHYSX_BIN_DIR}/${lib_name}.lib"
        IMPORTED_LOCATION "${PHYSX_BIN_DIR}/${lib_name}.dll"
    )
endfunction()

function(add_physx_static_library target_name lib_name)
    add_library(${target_name} STATIC IMPORTED GLOBAL)
    set_target_properties(${target_name} PROPERTIES
        IMPORTED_LOCATION "${PHYSX_BIN_DIR}/${lib_name}.lib"
    )
endfunction()

# The four DLL-backed libs we actually need
add_physx_dll_library(PhysX::Foundation  PhysXFoundation_64)
add_physx_dll_library(PhysX::Common      PhysXCommon_64)
add_physx_dll_library(PhysX::PhysX       PhysX_64)

# Extensions is static — no DLL, links directly into your exe
add_physx_static_library(PhysX::Extensions PhysXExtensions_static_64)

# PVD (PhysX Visual Debugger) SDK — static, needed by Extensions at link time
add_physx_static_library(PhysX::PvdSDK  PhysXPvdSDK_static_64)

# Single interface target your executable links against
add_library(PhysX::All INTERFACE IMPORTED GLOBAL)
target_link_libraries(PhysX::All INTERFACE
    PhysX::Foundation
    PhysX::Common
    PhysX::PhysX
    PhysX::Extensions
    PhysX::PvdSDK
)
target_include_directories(PhysX::All INTERFACE "${PHYSX_INC_DIR}")

# Collect DLLs for the copy step in the main CMakeLists
set(PHYSX_RUNTIME_DLLS
    "${PHYSX_BIN_DIR}/PhysXFoundation_64.dll"
    "${PHYSX_BIN_DIR}/PhysXCommon_64.dll"
    "${PHYSX_BIN_DIR}/PhysX_64.dll"
)
set(PhysX_FOUND TRUE)
