#pragma once

#include <string>
#include <fstream>
#include <cstdint>
#include <map>
#include <regex>
#include <filesystem>

#include <spdlog/spdlog.h>

#include "gpu.hpp"

namespace fs = std::filesystem;

struct hwmon_sensor {
    std::string generic_name;
    std::string filename;
    bool use_regex = false;
};

class HwmonBase {
private:
    struct sensor {
        std::string filename;
        bool use_regex = false;

        std::ifstream stream;
        std::string path;
        unsigned char id = 0;
        uint64_t val = 0;
    };

    std::map<std::string, sensor> sensors;
    void add_sensors(const std::vector<hwmon_sensor>& input_sensors);
    void find_sensors();
    void open_sensors();

public:
    std::string base_dir;

    std::string find_hwmon_dir(const std::string& drm_node);
    std::string find_hwmon_dir_by_name(const std::string& name);

    void setup(const std::vector<hwmon_sensor>& input_sensors, const std::string& drm_node = "");
    void poll_sensors();

    uint64_t get_sensor_value(const std::string& generic_name);
};

struct Hwmon {
    HwmonBase hwmon;
};
