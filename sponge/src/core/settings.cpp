#include "core/settings.hpp"

#include <fkYAML/node.hpp>

#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <utility>

namespace {
using fkyaml::node;

node data = node::mapping();

const node* navigate(const std::string& key) {
    const node*            current = &data;
    std::string::size_type start   = 0;

    while (true) {
        const auto pos = key.find('.', start);
        const auto segment =
            key.substr(start, pos == std::string::npos ? pos : pos - start);

        if (!current->is_mapping() || !current->contains(segment)) {
            return nullptr;
        }

        current = &(*current)[segment];
        if (pos == std::string::npos) {
            return current;
        }

        start = pos + 1;
    }
}

node& navigateOrCreate(const std::string& key) {
    node*                  current = &data;
    std::string::size_type start   = 0;

    while (true) {
        const auto pos = key.find('.', start);
        const auto segment =
            key.substr(start, pos == std::string::npos ? pos : pos - start);

        // fkYAML throws on operator[] for a scalar or null node, so a missing
        // or scalar parent is replaced with an empty mapping first.
        if (!current->is_mapping()) {
            *current = node::mapping();
        }

        current = &(*current)[segment];
        if (pos == std::string::npos) {
            return *current;
        }

        start = pos + 1;
    }
}

std::string& settingsPath() {
    static std::string path;
    return path;
}

void writeToFile(const std::string& filepath) {
    const auto dir = std::filesystem::path(filepath).parent_path();
    std::filesystem::create_directories(dir);

    std::ofstream file(filepath);
    if (file.is_open()) {
        file << node::serialize(data);
    }
}
}  // namespace

namespace sponge::core {

void Settings::load(const std::string& filepath) {
    settingsPath() = filepath;
    data           = node::mapping();

    std::ifstream file(filepath);
    if (!file.is_open()) {
        return;
    }

    try {
        auto root = node::deserialize(file);
        if (root.is_mapping()) {
            data = std::move(root);
        }
    } catch (const std::exception&) {
        // A malformed file keeps the defaults and is rewritten on save.
    }
}

void Settings::save() {
    if (!settingsPath().empty()) {
        writeToFile(settingsPath());
    }
}

std::string Settings::getString(const std::string& key,
                                const std::string& defaultValue) {
    const auto* value = navigate(key);
    if (value == nullptr || !value->is_string()) {
        return defaultValue;
    }
    return value->get_value<std::string>();
}

bool Settings::getBool(const std::string& key, const bool defaultValue) {
    const auto* value = navigate(key);
    if (value == nullptr || !value->is_boolean()) {
        return defaultValue;
    }
    return value->get_value<bool>();
}

float Settings::getFloat(const std::string& key, const float defaultValue) {
    const auto* value = navigate(key);
    // YAML reads 1 and 1.0 as different types; accept both.
    if (value == nullptr ||
        !(value->is_float_number() || value->is_integer())) {
        return defaultValue;
    }
    return value->get_value<float>();
}

uint32_t Settings::getUInt32(const std::string& key,
                             const uint32_t     defaultValue) {
    const auto* value = navigate(key);
    if (value == nullptr || !value->is_integer()) {
        return defaultValue;
    }
    const auto number = value->get_value<int64_t>();
    if (number < 0 || number > std::numeric_limits<uint32_t>::max()) {
        return defaultValue;
    }
    return static_cast<uint32_t>(number);
}

void Settings::set(const std::string& key, const std::string& value) {
    navigateOrCreate(key) = value;
}

void Settings::set(const std::string& key, const bool value) {
    navigateOrCreate(key) = value;
}

void Settings::set(const std::string& key, const uint32_t value) {
    navigateOrCreate(key) = static_cast<int64_t>(value);
}

}  // namespace sponge::core
