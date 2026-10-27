#include <services/serialization/scene_serializer.h>

#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

#include <domain/scene.h>
#include <platform/file_io.h>

namespace slrye
{
namespace
{

void set_error(std::string* error, std::string message)
{
    if (error != nullptr)
    {
        *error = std::move(message);
    }
}

std::string read_required_string(const YAML::Node& node, const char* field)
{
    const YAML::Node value = node[field];
    if (!value || !value.IsScalar())
    {
        throw std::runtime_error(std::string("Scene file requires '") + field + "'");
    }

    std::string result = value.as<std::string>();
    if (result.empty())
    {
        throw std::runtime_error(std::string("Scene file field '") + field + "' cannot be empty");
    }
    return result;
}

float read_float(const YAML::Node& node, const char* field, float fallback)
{
    const YAML::Node value = node[field];
    return value ? value.as<float>() : fallback;
}

bool read_bool(const YAML::Node& node, const char* field, bool fallback)
{
    const YAML::Node value = node[field];
    return value ? value.as<bool>() : fallback;
}

glm::vec3 read_vec3(const YAML::Node& node, const char* field)
{
    if (!node || !node.IsSequence() || node.size() != 3)
    {
        throw std::runtime_error(std::string("Scene field '") + field + "' must contain three numbers");
    }
    return glm::vec3(node[0].as<float>(), node[1].as<float>(), node[2].as<float>());
}

glm::quat read_quat(const YAML::Node& node, const char* field)
{
    if (!node || !node.IsSequence() || node.size() != 4)
    {
        throw std::runtime_error(std::string("Scene field '") + field + "' must contain four numbers");
    }
    return glm::quat(
        node[0].as<float>(),
        node[1].as<float>(),
        node[2].as<float>(),
        node[3].as<float>());
}

YAML::Node write_vec3(const glm::vec3& value)
{
    YAML::Node node(YAML::NodeType::Sequence);
    node.push_back(value.x);
    node.push_back(value.y);
    node.push_back(value.z);
    return node;
}

YAML::Node write_quat(const glm::quat& value)
{
    YAML::Node node(YAML::NodeType::Sequence);
    node.push_back(value.w);
    node.push_back(value.x);
    node.push_back(value.y);
    node.push_back(value.z);
    return node;
}

std::string_view light_type_name(LightType type) noexcept
{
    switch (type)
    {
    case LightType::Directional:
        return "directional";
    case LightType::Point:
        return "point";
    case LightType::Spot:
        return "spot";
    }
    return {};
}

LightType read_light_type(const YAML::Node& node, LightType fallback)
{
    if (!node)
    {
        return fallback;
    }
    const std::string value = node.as<std::string>();
    if (value == "directional")
    {
        return LightType::Directional;
    }
    if (value == "point")
    {
        return LightType::Point;
    }
    if (value == "spot")
    {
        return LightType::Spot;
    }
    throw std::runtime_error("Unknown light type in scene file: " + value);
}

std::string_view collider_type_name(ColliderType type) noexcept
{
    switch (type)
    {
    case ColliderType::Box:
        return "box";
    case ColliderType::Sphere:
        return "sphere";
    case ColliderType::Capsule:
        return "capsule";
    }
    return {};
}

ColliderType read_collider_type(const YAML::Node& node, ColliderType fallback)
{
    if (!node)
    {
        return fallback;
    }
    const std::string value = node.as<std::string>();
    if (value == "box")
    {
        return ColliderType::Box;
    }
    if (value == "sphere")
    {
        return ColliderType::Sphere;
    }
    if (value == "capsule")
    {
        return ColliderType::Capsule;
    }
    throw std::runtime_error("Unknown collider type in scene file: " + value);
}

void write_entity_components(
    YAML::Node& entity,
    const Scene& scene,
    EntityId id,
    entt::entity handle)
{
    if (handle == entt::null)
    {
        throw std::runtime_error("Scene contains an invalid entity while saving");
    }

    if (const NameComponent* name = scene.registry().try_get<NameComponent>(handle))
    {
        entity["name"] = name->value;
    }

    if (const TransformComponent* transform = scene.registry().try_get<TransformComponent>(handle))
    {
        YAML::Node transform_node;
        transform_node["position"] = write_vec3(transform->position);
        transform_node["rotation"] = write_quat(transform->rotation);
        transform_node["scale"] = write_vec3(transform->scale);
        entity["transform"] = transform_node;
    }

    if (const CameraComponent* camera = scene.registry().try_get<CameraComponent>(handle))
    {
        YAML::Node camera_node;
        camera_node["fov_degrees"] = camera->vertical_fov_degrees;
        camera_node["near_clip"] = camera->near_clip;
        camera_node["far_clip"] = camera->far_clip;
        camera_node["primary"] = camera->primary;
        entity["camera"] = camera_node;
    }

    if (const LightComponent* light = scene.registry().try_get<LightComponent>(handle))
    {
        YAML::Node light_node;
        light_node["type"] = std::string(light_type_name(light->type));
        light_node["color"] = write_vec3(light->color);
        light_node["intensity"] = light->intensity;
        light_node["range"] = light->range;
        light_node["inner_cone_degrees"] = light->inner_cone_degrees;
        light_node["outer_cone_degrees"] = light->outer_cone_degrees;
        entity["light"] = light_node;
    }

    if (const ColliderComponent* collider = scene.registry().try_get<ColliderComponent>(handle))
    {
        YAML::Node collider_node;
        collider_node["type"] = std::string(collider_type_name(collider->type));
        collider_node["half_extents"] = write_vec3(collider->half_extents);
        collider_node["radius"] = collider->radius;
        collider_node["height"] = collider->height;
        collider_node["enabled"] = collider->enabled;
        entity["collider"] = collider_node;
    }

    if (const std::optional<EntityId> parent = scene.parent(id))
    {
        entity["parent"] = parent->value();
    }
}

void load_entity_components(const YAML::Node& node, Entity entity)
{
    const YAML::Node transform_node = node["transform"];
    if (transform_node)
    {
        if (!transform_node.IsMap())
        {
            throw std::runtime_error("Entity field 'transform' must be a mapping");
        }
        TransformComponent& transform = entity.get<TransformComponent>();
        if (transform_node["position"])
        {
            transform.position = read_vec3(transform_node["position"], "position");
        }
        if (transform_node["rotation"])
        {
            transform.rotation = read_quat(transform_node["rotation"], "rotation");
        }
        if (transform_node["scale"])
        {
            transform.scale = read_vec3(transform_node["scale"], "scale");
        }
    }

    const YAML::Node camera_node = node["camera"];
    if (camera_node)
    {
        if (!camera_node.IsMap())
        {
            throw std::runtime_error("Entity field 'camera' must be a mapping");
        }
        CameraComponent camera;
        camera.vertical_fov_degrees = read_float(
            camera_node, "fov_degrees", camera.vertical_fov_degrees);
        camera.near_clip = read_float(camera_node, "near_clip", camera.near_clip);
        camera.far_clip = read_float(camera_node, "far_clip", camera.far_clip);
        camera.primary = read_bool(camera_node, "primary", camera.primary);
        entity.add<CameraComponent>(camera);
    }

    const YAML::Node light_node = node["light"];
    if (light_node)
    {
        if (!light_node.IsMap())
        {
            throw std::runtime_error("Entity field 'light' must be a mapping");
        }
        LightComponent light;
        light.type = read_light_type(light_node["type"], light.type);
        if (light_node["color"])
        {
            light.color = read_vec3(light_node["color"], "color");
        }
        light.intensity = read_float(light_node, "intensity", light.intensity);
        light.range = read_float(light_node, "range", light.range);
        light.inner_cone_degrees = read_float(
            light_node, "inner_cone_degrees", light.inner_cone_degrees);
        light.outer_cone_degrees = read_float(
            light_node, "outer_cone_degrees", light.outer_cone_degrees);
        entity.add<LightComponent>(light);
    }

    const YAML::Node collider_node = node["collider"];
    if (collider_node)
    {
        if (!collider_node.IsMap())
        {
            throw std::runtime_error("Entity field 'collider' must be a mapping");
        }
        ColliderComponent collider;
        collider.type = read_collider_type(collider_node["type"], collider.type);
        if (collider_node["half_extents"])
        {
            collider.half_extents = read_vec3(collider_node["half_extents"], "half_extents");
        }
        collider.radius = read_float(collider_node, "radius", collider.radius);
        collider.height = read_float(collider_node, "height", collider.height);
        collider.enabled = read_bool(collider_node, "enabled", collider.enabled);
        entity.add<ColliderComponent>(collider);
    }
}

}

bool SceneSerializer::save(
    const std::filesystem::path& path,
    const Scene& scene,
    std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }

    try
    {
        YAML::Node root;
        root["version"] = kSceneFormatVersion;

        YAML::Node scene_node;
        scene_node["name"] = scene.name();
        YAML::Node entities(YAML::NodeType::Sequence);
        for (EntityId id : scene.entities())
        {
            const entt::entity handle = scene.handle_for(id);
            YAML::Node entity;
            entity["id"] = id.value();
            write_entity_components(entity, scene, id, handle);
            entities.push_back(entity);
        }
        scene_node["entities"] = entities;
        root["scene"] = scene_node;

        std::string contents = YAML::Dump(root);
        contents.push_back('\n');
        return platform::write_text_file_atomic(path, contents, error);
    }
    catch (const std::exception& exception)
    {
        set_error(error, "Unable to save scene: " + std::string(exception.what()));
        return false;
    }
}

std::unique_ptr<Scene> SceneSerializer::load(
    const std::filesystem::path& path,
    std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }

    std::string contents;
    if (!platform::read_text_file(path, contents, error))
    {
        return nullptr;
    }

    try
    {
        const YAML::Node root = YAML::Load(contents);
        if (!root || !root.IsMap())
        {
            throw std::runtime_error("Scene file must contain a YAML mapping");
        }

        const YAML::Node version_node = root["version"];
        if (!version_node || !version_node.IsScalar())
        {
            throw std::runtime_error("Scene file requires a format version");
        }
        const std::uint32_t version = version_node.as<std::uint32_t>();
        if (version != kSceneFormatVersion)
        {
            throw std::runtime_error(
                "Unsupported scene file version " + std::to_string(version) +
                " (expected " + std::to_string(kSceneFormatVersion) + ")");
        }

        const YAML::Node scene_node = root["scene"];
        if (!scene_node || !scene_node.IsMap())
        {
            throw std::runtime_error("Scene file requires a 'scene' mapping");
        }

        auto scene = std::make_unique<Scene>(read_required_string(scene_node, "name"));
        const YAML::Node entities = scene_node["entities"];
        if (!entities || !entities.IsSequence())
        {
            throw std::runtime_error("Scene file requires an 'entities' sequence");
        }

        std::vector<std::pair<EntityId, EntityId>> parents;
        parents.reserve(entities.size());
        for (const YAML::Node& entity_node : entities)
        {
            if (!entity_node.IsMap())
            {
                throw std::runtime_error("Scene entities must be mappings");
            }

            const YAML::Node id_node = entity_node["id"];
            if (!id_node || !id_node.IsScalar())
            {
                throw std::runtime_error("Scene entities require a stable id");
            }

            const EntityId id(id_node.as<std::uint64_t>());
            if (!id.is_valid() || scene->contains(id))
            {
                throw std::runtime_error("Scene entity ids must be unique and non-zero");
            }

            const YAML::Node name_node = entity_node["name"];
            const std::string name = name_node ? name_node.as<std::string>() : "Entity";
            Entity entity = scene->create_entity(id, name);
            if (!entity.is_valid())
            {
                throw std::runtime_error("Unable to create an entity from the scene file");
            }

            load_entity_components(entity_node, entity);
            if (const YAML::Node parent_node = entity_node["parent"])
            {
                parents.emplace_back(id, EntityId(parent_node.as<std::uint64_t>()));
            }
        }

        for (const auto& [child, parent] : parents)
        {
            if (!scene->contains(parent) || !scene->set_parent(child, parent))
            {
                throw std::runtime_error("Scene contains an invalid or cyclic entity parent");
            }
        }

        scene->clear_dirty();
        return scene;
    }
    catch (const std::exception& exception)
    {
        set_error(error, "Unable to load scene: " + std::string(exception.what()));
        return nullptr;
    }
}

}
