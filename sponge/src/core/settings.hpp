#pragma once

#include <cstdint>
#include <string>

namespace sponge::core {

class Settings {
public:
    static void load(const std::string& filepath);
    static void save();

    static std::string getString(const std::string& key,
                                 const std::string& defaultValue = {});
    static bool     getBool(const std::string& key, bool defaultValue = false);
    static uint32_t getUInt32(const std::string& key,
                              uint32_t           defaultValue = 0);

    static void set(const std::string& key, const std::string& value);
    static void set(const std::string& key, bool value);
    static void set(const std::string& key, uint32_t value);
};

}  // namespace sponge::core
