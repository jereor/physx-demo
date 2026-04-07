// main.cpp
// Minimal PhysX 5.x demo: box dropped onto a static plane.
// Demonstrates: foundation init, determinism flag, fixed timestep loop.

#include "ErrorCallback.h"

#include <PxPhysicsAPI.h>
#include <iostream>

int main()
{
    // PxDefaultAllocator is provided by PhysX — it wraps malloc/free with the 16-byte alignment PhysX internally requires. Fine for our purposes.
    static physx::PxDefaultAllocator gAllocator;
    static ErrorCallback gErrorCallback;

    // -----------------------------------------------------------------------
    // 1. PxFoundation
    // The base layer. Owns the allocator and error callback for the entire PhysX session. Must be created first, destroyed last.
    // PX_PHYSICS_VERSION is a compile-time constant from PxPhysicsVersion.h that encodes major.minor.bugfix — PhysX checks this at runtime to catch header/library mismatches.
    // -----------------------------------------------------------------------
    physx::PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    if (!foundation) {
        std::cerr << "PxCreateFoundation failed\n";
        return 1;
    }

    // -----------------------------------------------------------------------
    // 2. PxPhysics
    // The main factory. Creates scenes, materials, shapes, actors.
    // PxTolerancesScale defines what "1 unit" means in your sim (default: 1 unit = 1 metre, which is what we want).
    // -----------------------------------------------------------------------
    physx::PxPhysics* physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale());
    if (!physics) {
        std::cerr << "PxCreatePhysics failed\n";
        return 1;
    }

    // PxInitExtensions wires up the Extensions library (PxRigidBodyExt etc.)
    // Must be called before using anything from PhysXExtensions_static_64.
    PxInitExtensions(*physics, nullptr);

    // -----------------------------------------------------------------------
    // 3. PxScene
    // The simulation world. This is where determinism and timestep live.
    // -----------------------------------------------------------------------
    physx::PxSceneDesc sceneDesc(physics->getTolerancesScale());

    // Gravity: 9.81 m/s² downward. Y is up in PhysX's default convention.
    sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);

    // CPU dispatcher: manages threads for simulation tasks.
    // 1 thread is enough for our demo and keeps output deterministic.
    constexpr int DISPATCHER_THREAD_LIMIT = 1;
    physx::PxDefaultCpuDispatcher* dispatcher = physx::PxDefaultCpuDispatcherCreate(DISPATCHER_THREAD_LIMIT);
    sceneDesc.cpuDispatcher = dispatcher;

    // Default filter shader — controls which actors collide with which.
    // For our demo (one box, one plane) this is irrelevant but required.
    sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

    // --- Determinism flag ---
    // eENABLE_ENHANCED_DETERMINISM guarantees that given identical inputs, simulation results are bitwise reproducible across runs ON THE SAME MACHINE with the same thread count.
    // It does NOT guarantee cross-platform or cross-CPU reproducibility (floating point, OS scheduler differences).
    // Cost: disables some internal multi-threading optimizations (~5-15% slower). Worth it for simulations where replay/logging matters.
    sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ENHANCED_DETERMINISM;

    physx::PxScene* scene = physics->createScene(sceneDesc);
    if (!scene) {
        std::cerr << "createScene failed\n";
        return 1;
    }

    // -----------------------------------------------------------------------
    // 4. Material
    // Defines surface interaction: static friction, dynamic friction, restitution (bounciness). One material can be shared across shapes.
    // -----------------------------------------------------------------------
    physx::PxMaterial* material = physics->createMaterial(
        0.5f,   // static friction
        0.5f,   // dynamic friction
        0.1f    // restitution (slight bounce)
    );

    // -----------------------------------------------------------------------
    // 5. Static plane (the floor)
    // PxCreatePlane is an Extensions helper. The plane is defined by a normal and a distance from origin. 
    // PxPlane(PxVec3(0,1,0), 0) = the XZ plane at Y=0, normal pointing up.
    // Static actors have infinite mass and are never moved by the simulation.
    // -----------------------------------------------------------------------
    physx::PxRigidStatic* groundPlane = PxCreatePlane(
        *physics, physx::PxPlane(physx::PxVec3(0.0f, 1.0f, 0.0f), 0.0f), *material);
    scene->addActor(*groundPlane);

    // -----------------------------------------------------------------------
    // 6. Dynamic box
    // PxCreateDynamic is another Extensions helper. It:
    //   - Creates a PxRigidDynamic actor at the given transform
    //   - Attaches a shape with the given geometry and material
    //   - Sets the density and computes mass/inertia automatically
    //
    // Transform: PxTransformFromPlaneEquation would be for planes; for a box we just supply a position. 
    // We start at Y=10 (10 metres above the floor).
    //
    // PxBoxGeometry(0.5, 0.5, 0.5) = a 1x1x1 metre box (half-extents).
    // Density 1.0 kg/m³ is very light, but fine for this demo.
    // -----------------------------------------------------------------------
    physx::PxRigidDynamic* box = PxCreateDynamic(
        *physics,
        physx::PxTransform(physx::PxVec3(0.0f, 10.0f, 0.0f)), // start position
        physx::PxBoxGeometry(0.5f, 0.5f, 0.5f), // half-extents
        *material,
        1.0f // density kg/m³
    );
    scene->addActor(*box);

    // -----------------------------------------------------------------------
    // 7. Fixed timestep simulation loop
    //
    // Key design decision: PhysX does NOT enforce a fixed timestep internally.
    // You pass whatever dt you want to simulate(). For determinism and stability you must pass a CONSTANT dt every step — never pass wall-clock delta time directly.
    //
    // Why fixed timestep matters:
    //   - Collision detection and constraint solving are iterative; variable dt changes the iteration count implicitly and breaks reproducibility.
    //   - Contact events, joint limits, friction all depend on dt magnitude.
    //
    // simulate() kicks off async simulation on the dispatcher threads.
    // fetchResults(true) blocks until simulation completes, then swaps buffers.
    // -----------------------------------------------------------------------
    constexpr float  fixedDt    = 1.0f / 60.0f;  // 60 Hz fixed step
    constexpr int    stepCount  = 300;            // 5 seconds of simulation
    constexpr bool SIM_BLOCKS_CPU = true;  // "block until done" — always use true unless you have other CPU work to overlap with simulation.

    std::cout << "step, time_s, pos_x, pos_y, pos_z\n";

    for (int step = 0; step < stepCount; ++step) {
        scene->simulate(fixedDt);
        scene->fetchResults(SIM_BLOCKS_CPU);

        physx::PxTransform t = box->getGlobalPose();
        std::cout << step << ", "
                  << step * fixedDt << ", "
                  << t.p.x << ", "
                  << t.p.y << ", "
                  << t.p.z << "\n";
    }

    // -----------------------------------------------------------------------
    // 8. Cleanup
    // PhysX objects are reference-counted via release().
    // Order matters: scene before physics, physics before foundation. 
    // Actors/materials are owned by the scene/physics factory and released with their parent, but explicit release is cleaner and required for shared resources.
    // -----------------------------------------------------------------------
    PxCloseExtensions();
    scene->release();
    dispatcher->release();
    physics->release();
    foundation->release();

    std::cout << "Done.\n";
    return 0;
}
