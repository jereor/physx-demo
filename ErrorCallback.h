#include <PxPhysicsAPI.h>
#include <iostream>

class ErrorCallback : public physx::PxErrorCallback {
    public:
        void reportError(physx::PxErrorCode::Enum code, const char* message,
                        const char* file, int line) override
        {
            std::cerr << "[PhysX Error " << code << "] " << message
                    << " (" << file << ":" << line << ")\n";
        }
    };
