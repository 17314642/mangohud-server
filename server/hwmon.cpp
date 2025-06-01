#include "hwmon.hpp"

void HwmonBase::add_sensors(const std::vector<hwmon_sensor>& input_sensors)
{
    for (const auto& s : input_sensors) {
        sensors[s.generic_name].use_regex = s.use_regex;
        sensors[s.generic_name].filename = s.filename;
    }
}

void HwmonBase::find_sensors() {
    if (base_dir.empty()) {
        SPDLOG_WARN("hwmon: base_dir is empty. hwmon will not work!");
        return;
    }

    SPDLOG_DEBUG("hwmon: checking \"{}\" directory", base_dir);

    for (const auto &entry : fs::directory_iterator(base_dir)) {
        auto filename = entry.path().filename().string();

        for (auto& s : sensors) {
            auto key = s.first;
            auto sensor = &s.second;
            std::smatch matches;

            if (sensor->use_regex) {
                std::regex rx(sensor->filename);

                if (!std::regex_match(filename, matches, rx) || matches.size() != 2)
                    continue;
    
                auto cur_id = std::stoull(matches[1].str());
    
                if (sensor->path.empty() || cur_id < sensor->id) {
                    sensor->path = entry.path().string();
                    sensor->id = cur_id;
                }
            } else if (filename == sensor->filename) {
                sensor->path = entry.path().string();
                break;
            }
        }
    }
};

void HwmonBase::open_sensors() {
    for (auto& s : sensors) {
        auto key = s.first;
        auto sensor = &s.second;

        if (sensor->path.empty()) {
            SPDLOG_DEBUG("hwmon: {} reading not found at {}", key, base_dir);
            continue;
        }

        SPDLOG_DEBUG("hwmon: {} reading found at {}", key, sensor->path);

        sensor->stream.open(sensor->path);

        if (!sensor->stream.good()) {
            SPDLOG_DEBUG(
                "hwmon: failed to open {} reading {}",
                key, sensor->path
            );
            continue;
        }
    }
}

std::string HwmonBase::find_hwmon_dir(const std::string& drm_node) {
    std::string d = "/sys/class/drm/" + drm_node + "/device/hwmon";

    if (!fs::exists(d)) {
        SPDLOG_DEBUG("hwmon: hwmon directory \"{}\" doesn't exist", d);
        return "";
    }

    auto dir_iterator = fs::directory_iterator(d);
    auto hwmon = dir_iterator->path().string();

    if (hwmon.empty()) {
        SPDLOG_DEBUG("hwmon: hwmon directory \"{}\" is empty.", d);
        return "";
    }

    return hwmon;
}

std::string HwmonBase::find_hwmon_dir_by_name(const std::string& name) {
    std::string d = "/sys/class/hwmon/";

    if (!fs::exists(d)) {
        SPDLOG_DEBUG("hwmon: hwmon directory doesn't exist (custom linux kernel?)");
        return "";
    }

    for (const auto &entry : fs::directory_iterator(d)) {
        auto hwmon_dir = entry.path().string();
        auto hwmon_name = hwmon_dir + "/name";

        std::ifstream name_stream(hwmon_name);
        std::string name_content;

        if (!name_stream.is_open())
            continue;

        std::getline(name_stream, name_content);

        if (name_content.find(name) == std::string::npos)
            continue;

        // return the first sensor with specified name
        return hwmon_dir;
    }

    return "";
}

void HwmonBase::setup(const std::vector<hwmon_sensor>& input_sensors, const std::string& drm_node) {
    add_sensors(input_sensors);

    if (base_dir.empty())
        base_dir = find_hwmon_dir(drm_node);

    find_sensors();
    open_sensors();
}

void HwmonBase::poll_sensors()
{
    for (auto& s : sensors) {
        auto name = s.first;
        auto sensor = &s.second;

        if (!sensor->stream.is_open())
            continue;

        sensor->stream.seekg(0);

        std::stringstream ss;
        ss << sensor->stream.rdbuf();

        if (ss.str().empty())
            continue;

        sensor->val = std::stoull(ss.str());
    }
}

uint64_t HwmonBase::get_sensor_value(const std::string& generic_name)
{
    if (sensors.find(generic_name) == sensors.end())
        return 0;
    
    return sensors[generic_name].val;
}
