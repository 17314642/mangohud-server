#include <sys/socket.h>
#include "log_errno.hpp"
#include "gpu_metrics.hpp"

bool receive_message(int fd, mangohud_message& msg);
bool receive_message_with_creds(int fd, size_t& pid);
void send_message(int fd, mangohud_message& msg);
std::string get_socket_path();
