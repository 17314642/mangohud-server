#include "socket.hpp"
#include <sys/socket.h>

cmsghdr* get_cmsghdr(msghdr* message_header) {
    cmsghdr* cmhp = CMSG_FIRSTHDR(message_header);

    if (cmhp == NULL) {
        SPDLOG_DEBUG("Error: bad cmsg header");
        return nullptr;
    }

    if (cmhp->cmsg_len != CMSG_LEN(sizeof(ucred))) {
        SPDLOG_DEBUG(
            "Error: bad cmsg header length {}. Required length is {}",
            cmhp->cmsg_len, CMSG_LEN(sizeof(ucred))
        );
        return nullptr;
    }

    if (cmhp->cmsg_level != SOL_SOCKET) {
        SPDLOG_DEBUG("Error: cmsg_level != SOL_SOCKET");
        return nullptr;
    }

    if (cmhp->cmsg_type != SCM_CREDENTIALS) {
        SPDLOG_DEBUG("Error: cmsg_type != SCM_CREDENTIALS");
        return nullptr;
    }

    return cmhp;
}

bool receive_message(int fd, mangohud_message& msg) {
    iovec iov = {
        .iov_base = &msg,
        .iov_len = sizeof(msg)
    };

    msghdr message_header = {
        .msg_iov = &iov,
        .msg_iovlen = 1
    };

    int ret = recvmsg(fd, &message_header, 0);

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Couldn't receive message via recvmsg() on fd {}.", fd);
        return false;
    }

    // SPDLOG_DEBUG("Received new message from fd {} len={}", fd, ret);

    return true;
}

bool receive_message_with_creds(int fd, size_t& pid) {
    const size_t buf_size = 4096;
    std::vector<char> buf(buf_size);

    iovec iov = {
        .iov_base = buf.data(),
        .iov_len = buf_size
    };

    union {
        cmsghdr cmh;
        char   control[CMSG_SPACE(sizeof(struct ucred))];
        /* Space large enough to hold a ucred structure */
    } control_un;

    control_un.cmh.cmsg_len = CMSG_LEN(sizeof(struct ucred));
    control_un.cmh.cmsg_level = SOL_SOCKET;
    control_un.cmh.cmsg_type = SCM_CREDENTIALS;

    msghdr message_header = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = &control_un.control,
        .msg_controllen = sizeof(control_un.control)
    };

    int ret = recvmsg(fd, &message_header, 0);

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Couldn't receive message via recvmsg() on fd {}.", fd);
        return false;
    }

    std::string s = buf.data();

    SPDLOG_DEBUG("Received new message from fd {} len={}: \"{}\"", fd, ret, s);

    cmsghdr *cmhp = get_cmsghdr(&message_header);

    if (!cmhp)
        return false;
 
    ucred *ucredp = (struct ucred *) CMSG_DATA(cmhp);

    SPDLOG_DEBUG(
        "Received credentials pid=[{}], uid=[{}], gid=[{}]",
        ucredp->pid, ucredp->uid, ucredp->gid
    );

    pid = ucredp->pid;
    return true;
}

void send_message(int fd, mangohud_message& msg) {
    iovec iov = {
        .iov_base = &msg,
        .iov_len = sizeof(msg)
    };

    msghdr message_header = {
        .msg_iov = &iov,
        .msg_iovlen = 1
    };

    int ret = sendmsg(fd, &message_header, 0);

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Couldn't send message via sendmsg() on fd {}.", fd);
        return;
    }

    SPDLOG_DEBUG("Sent message to fd {} len={}", fd, ret);
}
