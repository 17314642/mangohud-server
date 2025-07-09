#include <fstream>
#include <set>
#include <filesystem>
#include <spdlog/spdlog.h>
#include "iostats.hpp"
#include "../common/helpers.hpp"

void IOStats::add_pid(pid_t pid) {
    if (pids.find(pid) != pids.end())
        return;

    SPDLOG_DEBUG("adding pid {} to iostats", pid);
    auto entry = pids.try_emplace(pid, _io_stats());

    if (!entry.second)
        return;

    std::string f = "/proc/" + std::to_string(pid) + "/io";
    std::ifstream& stream = entry.first->second.stream;
    stream.open(f);

    if (!stream.is_open()) {
        SPDLOG_DEBUG("failed to open \"{}\"", f);
        return;
    }
}

void IOStats::poll_pid(pid_t pid) {
    _io_stats& stats = pids[pid];

    uint64_t total_read = 0;
    uint64_t total_write = 0;

    stats.stream.clear();
    stats.stream.seekg(0);

    for (std::string line; std::getline(stats.stream, line);) {
        if (line.substr(0, 11) == "read_bytes:") {
            total_read = try_stoull(line.substr(12));
        }
        else if (line.substr(0, 12) == "write_bytes:") {
            total_write = try_stoull(line.substr(13));
        }
    }

    using namespace std::chrono;
    auto now = std::chrono::steady_clock::now();
    auto delta = now - stats.last_update;

    if (stats.previous_read_bytes == 0 && stats.previous_write_bytes == 0) {
        stats.last_update = now;
        stats.previous_read_bytes = total_read;
        stats.previous_write_bytes = total_write;

        return;
    }

    float read_mb_per_sec = (total_read - stats.previous_read_bytes) / (1024.f * 1024.f);
    float write_mb_per_sec = (total_write - stats.previous_write_bytes) / (1024.f * 1024.f);

    read_mb_per_sec /= duration_cast<milliseconds>(delta).count() * 1000.f;
    write_mb_per_sec /= duration_cast<milliseconds>(delta).count() * 1000.f;

    stats.read_mb_per_sec = read_mb_per_sec;
    stats.write_mb_per_sec = write_mb_per_sec;

    stats.previous_read_bytes = total_read;
    stats.previous_write_bytes = total_write;

    stats.last_update = now;
}

void IOStats::poll() {
    std::set<pid_t> pids_to_delete;

    for (auto& p : pids) {
        pid_t pid = p.first;

        if (!std::filesystem::exists("/proc/" + std::to_string(pid)))
            pids_to_delete.insert(pid);
        else
            poll_pid(pid);
    }

    for (const auto& p : pids_to_delete) {
        SPDLOG_TRACE("deleting pid {}", p);
        pids.erase(p);
    }
}

io_stats_t IOStats::get_stats(pid_t pid) {
    _io_stats& stats = pids[pid];
    return { stats.read_mb_per_sec, stats.write_mb_per_sec };
}
