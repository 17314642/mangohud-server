#include <mutex>
#include "../../common/gpu_metrics.hpp"

void init_client();

extern std::mutex metrics_lock;
extern mangohud_message metrics;
