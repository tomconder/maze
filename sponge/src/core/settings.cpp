#include "core/settings.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace {
nlohmann::json data;

const nlohmann::json* navigate(const std::string& key) {
    const nlohmann::json*  node  = &data;
    std::string::size_type start = 0;

    while (true) {
        const auto pos = key.find('.', start);
        const auto segment =
            key.substr(start, pos == std::string::npos ? pos : pos - start);

        const auto it = node->find(segment);
        if (it == node->end()) {
            return nullptr;
        }

        if (pos == std::string::npos) {
            return &*it;
        }

        if (!it->is_object()) {
            return nullptr;
        }

        node  = &*it;
        start = pos + 1;
    }
}

nlohmann::json& navigateOrCreate(const std::string& key) {
    nlohmann::json*        node  = &data;
    std::string::size_type start = 0;

    while (true) {
        const auto pos = key.find('.', start);
        const auto segment =
            key.substr(start, pos == std::string::npos ? pos : pos - start);

        if (pos == std::string::npos) {
            return (*node)[segment];
        }

        node  = &(*node)[segment];
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
        file << data.dump(4) << "\n";
    }
}
}  // namespace

namespace sponge::core {

void Settings::load(const std::string& filepath) {
    settingsPath() = filepath;
    data.clear();

    std::ifstream file(filepath);
    if (!file.is_open()) {
        return;
    }

    try {
        data = nlohmann::json::parse(file);
    } catch (const nlohmann::json::parse_error&) {
        data = nlohmann::json::object();
    }
}

void Settings::save() {
    if (!settingsPath().empty()) {
        writeToFile(settingsPath());
    }
}

std::string Settings::getString(const std::string& key,
                                const std::string& defaultValue) {
    const auto* node = navigate(key);
    if (node == nullptr || !node->is_string()) {
        return defaultValue;
    }
    return node->get<std::string>();
}

bool Settings::getBool(const std::string& key, const bool defaultValue) {
    const auto* node = navigate(key);
    if (node == nullptr || !node->is_boolean()) {
        return defaultValue;
    }
    return node->get<bool>();
}

uint32_t Settings::getUInt32(const std::string& key,
                             const uint32_t     defaultValue) {
    const auto* node = navigate(key);
    if (node == nullptr || !node->is_number_unsigned()) {
        return defaultValue;
    }
    return node->get<uint32_t>();
}

void Settings::set(const std::string& key, const std::string& value) {
    navigateOrCreate(key) = value;
}

void Settings::set(const std::string& key, const bool value) {
    navigateOrCreate(key) = value;
}

void Settings::set(const std::string& key, const uint32_t value) {
    navigateOrCreate(key) = value;
}

}  // namespace sponge::core
