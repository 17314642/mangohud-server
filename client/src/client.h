#include <mutex>
#include <set>
#include "../../common/gpu_metrics.hpp"

void init_client();

extern std::mutex metrics_lock;
extern mangohud_message metrics;
extern std::set<uint8_t> gpu_list;
