#include "box3d_replay_conformance.hpp"

#include "box3d_world_lifecycle.hpp"

#include <box3d/box3d.h>

#include <system_error>

namespace ninho::physics::detail {
namespace {

struct RecordingOwner {
    b3Recording* value{};

    ~RecordingOwner() { b3DestroyRecording(value); }
};

struct WorldOwner {
    b3WorldId value{};

    ~WorldOwner()
    {
        if (B3_IS_NON_NULL(value) && b3World_IsValid(value)) {
            destroy_box3d_world(value);
        }
    }
};

}

ReplayConformanceResult validate_box3d_replay(
    const std::filesystem::path& temporary_path)
{
    ReplayConformanceResult result;
    std::error_code filesystem_error;
    const auto temporary_file_absent = [&] {
        filesystem_error.clear();
        const bool exists = std::filesystem::exists(temporary_path, filesystem_error);
        return !filesystem_error && !exists;
    };
    const auto remove_temporary_file = [&] {
        filesystem_error.clear();
        std::filesystem::remove(temporary_path, filesystem_error);
        return !filesystem_error && temporary_file_absent();
    };

    std::filesystem::remove(temporary_path, filesystem_error);
    if (filesystem_error || !temporary_file_absent()) {
        result.error = "initial temporary replay file cleanup failed";
        return result;
    }

    RecordingOwner recording{b3CreateRecording(0)};
    if (recording.value == nullptr) {
        result.error = "b3CreateRecording returned null";
        result.temporary_file_removed = temporary_file_absent();
        return result;
    }

    b3WorldDef world_def = b3DefaultWorldDef();
    WorldOwner world{create_box3d_world(world_def)};
    if (B3_IS_NULL(world.value) || !b3World_IsValid(world.value)) {
        result.error = "b3CreateWorld returned an invalid id";
        result.temporary_file_removed = temporary_file_absent();
        return result;
    }

    b3World_StartRecording(world.value, recording.value);
    b3World_SetGravity(world.value, {0, -10, 0});

    b3BodyDef ground_def = b3DefaultBodyDef();
    ground_def.type = b3_staticBody;
    const b3BodyId ground = b3CreateBody(world.value, &ground_def);
    const b3BoxHull ground_box = b3MakeBoxHull(5, 0.5f, 5);
    b3ShapeDef ground_shape = b3DefaultShapeDef();
    b3CreateHullShape(ground, &ground_shape, &ground_box.base);

    b3BodyDef body_def = b3DefaultBodyDef();
    body_def.type = b3_dynamicBody;
    body_def.position = {0, 3, 0};
    const b3BodyId body = b3CreateBody(world.value, &body_def);
    const b3Sphere sphere{{0, 0, 0}, 0.5f};
    b3ShapeDef sphere_shape = b3DefaultShapeDef();
    sphere_shape.density = 1.0f;
    b3CreateSphereShape(body, &sphere_shape, &sphere);
    for (int tick = 0; tick < 10; ++tick) {
        b3World_Step(world.value, 1.0f / 60.0f, 4);
    }
    b3World_StopRecording(world.value);

    result.bytes = static_cast<std::size_t>(b3Recording_GetSize(recording.value));
    const std::string path = temporary_path.string();
    result.saved = b3SaveRecordingToFile(recording.value, path.c_str());
    if (!result.saved) {
        result.error = "b3SaveRecordingToFile failed";
    } else {
        RecordingOwner loaded{b3LoadRecordingFromFile(path.c_str())};
        result.loaded = loaded.value != nullptr;
        if (!result.loaded) {
            result.error = "b3LoadRecordingFromFile failed";
        } else {
            result.validated = b3ValidateReplay(
                b3Recording_GetData(loaded.value),
                b3Recording_GetSize(loaded.value),
                1);
            if (!result.validated) {
                result.error = "b3ValidateReplay reported a divergence";
            }
        }
    }

    result.temporary_file_removed = remove_temporary_file();
    if (!result.temporary_file_removed && result.error.empty()) {
        result.error = "temporary replay file cleanup failed";
    }
    return result;
}

}
