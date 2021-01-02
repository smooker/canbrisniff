#include <linux/can.h>
#include <linux/can/raw.h>

#include <endian.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <pthread.h>


#include <helpers.h>

#define PROGNAME  "socketcan-raw-demo"
#define VERSION  "2.0.0"

pthread_t thread1;
pthread_t thread2;


int signalValue = 0;

struct thread_data {
    int thread_id;
    int sockfd;
    uint8_t* msg;
    const char* interface;
    int must_exit=0;
    thread_data *other_td;
};

thread_data td1;
thread_data td2;

//listener
void *listener(void *data)
{
    usleep(500);

    struct iovec iov;
    char ctrlmsg[CMSG_SPACE(sizeof(struct timeval) + 3*sizeof(struct timespec) + sizeof(__u32))];

    struct msghdr msg;

    struct canfd_frame frame;
    struct sockaddr_can addr;

    struct cmsghdr *cmsg;
    struct can_filter *rfilter;

    struct thread_data *my_data;
    my_data = (struct thread_data *) data;

    struct ifreq ifr;
    int sockfd;

    printf("listener: %s\n", my_data->interface);

    int rc;

    // Open the CAN network interface
    my_data->sockfd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (-1 == sockfd) {
        std::perror("socket");
        exit(10);
    }

    // Enable reception of CAN FD frames
    {
        int enable = 1;

        rc = ::setsockopt(
            my_data->sockfd,
            SOL_CAN_RAW,
            CAN_RAW_FD_FRAMES,
            &enable,
            sizeof(enable)
        );
        if (-1 == rc) {
            std::perror("setsockopt CAN FD");
            exit(10);
        }
    }

    // Get the index of the network interface
    std::strncpy(ifr.ifr_name, my_data->interface, IFNAMSIZ);
    if (::ioctl(my_data->sockfd, SIOCGIFINDEX, &ifr) == -1) {
        std::perror("ioctl");
        exit(10);
    }

    // Bind the socket to the network interface
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    rc = ::bind(
        my_data->sockfd,
        reinterpret_cast<struct sockaddr*>(&addr),
        sizeof(addr)
    );
    if (-1 == rc) {
        std::perror("bind");
        exit(10);
    }

    /* these settings are static and can be held out of the hot path */
    iov.iov_base = &frame;
    msg.msg_name = &addr;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &ctrlmsg;

    char buf[16]; /* max length */

    while(my_data->must_exit == 0) {
        iov.iov_len = sizeof(frame);
        msg.msg_namelen = sizeof(addr);
        msg.msg_controllen = sizeof(ctrlmsg);
        msg.msg_flags = 0;

        //here loop
        auto numBytes = recvmsg(my_data->sockfd, &msg, 0);

        switch (numBytes) {
        case CAN_MTU:
            sprint_canframe(buf, &frame, 1, 16);
    //        std::this_thread::sleep_for (std::chrono::seconds(1));
            printf("%d:%s:%s\n", my_data->thread_id, my_data->interface, buf);
            {
//                int nbytes = sendmsg(my_data->other_td->sockfd, &msg, 0);
//                int nbytes = send(my_data->other_td->sockfd, &frame, frame.len, 0);
                auto nbytes = write(my_data->other_td->sockfd, &frame, sizeof(struct can_frame));
                printf("Wrote %d bytes\n", nbytes);
                std::perror("send");
            }
            break;
        case CANFD_MTU:
            // TODO: Should make an example for CAN FD
            break;
        case -1:
            // Check the signal value on interrupt
            if (EINTR == errno)
                continue;

            // Delay before continuing
            std::perror("read");
//            std::this_thread::sleep_for(100ms);
            usleep(20000);
        default:
            continue;
        }


//        usleep(200);
//        pthread_exit(NULL);
    }

    ::close(my_data->sockfd);
    //here loop

    pthread_exit(NULL);
}

namespace {

//std::sig_atomic_t signalValue;

void onSignal(int value) {
    signalValue = static_cast<decltype(signalValue)>(value);
    std::cout << "SIGNAL###############################################:" << value << std::endl;
    td1.must_exit=1;
    td2.must_exit=1;
}

void sendPacket(struct canfd_frame *cf, int sockfd)
{
  struct can_frame frame;

  //00 9F E0 3F 66
  frame.can_id  = 0x072;
  frame.can_dlc = 5;
  frame.data[0] = 0x00;
  frame.data[1] = 0x9F;
  frame.data[2] = 0xE0;
  frame.data[3] = 0x3F;
  frame.data[4] = 0xFF;

  auto nbytes = write(sockfd, &frame, sizeof(struct can_frame));
}

void usage() {
    std::cout << "Usage: " PROGNAME " [-h] [-V] [-f] interface" << std::endl
              << "Options:" << std::endl
              << "  -h  Display this information" << std::endl
              << "  -V  Display version information" << std::endl
              << "  -f  Run in the foreground" << std::endl
              << std::endl;
}

void version() {
    std::cout << PROGNAME " version " VERSION << std::endl
              << "Compiled on " __DATE__ ", " __TIME__ << std::endl
              << std::endl;
}


} // namespace

int main(int argc, char** argv) {
    using namespace std::chrono_literals;

    bool foreground = false;

    // Service variables
    struct sigaction sa;

    // Parse command line arguments

    int opt;

    // Parse option flags
    while ((opt = ::getopt(argc, argv, "Vfh12")) != -1) {
        switch (opt) {
        case 'V':
            version();
            return EXIT_SUCCESS;
        case 'f':
            foreground = true;
            break;
        case '1':
            td1.interface = argv[optind];
            break;
        case '2':
            td2.interface = argv[optind];
            break;
        case 'h':
            usage();
            return EXIT_SUCCESS;
        default:
            usage();
            return EXIT_FAILURE;
        }
    }

//    // Check for the one positional argument
//    if (optind != (argc - 1 )) {
//        std::cerr << "Missing network interface option!" << std::endl;
//        usage();
//        return EXIT_FAILURE;
//    }

    // Check if the service should be run as a daemon
    if (!foreground) {
        if (::daemon(0, 1) == -1) {
            std::perror("daemon");
            return EXIT_FAILURE;
        }
    }

    // Register signal handlers
    sa.sa_handler = onSignal;
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    ::sigaction(SIGINT, &sa, nullptr);
    ::sigaction(SIGTERM, &sa, nullptr);
    ::sigaction(SIGQUIT, &sa, nullptr);
    ::sigaction(SIGHUP, &sa, nullptr);

    // Log that the service is up and running
    std::cout << "Started" << std::endl;

    int rc;

    td1.thread_id = 1;
    td1.other_td = &td2;

    usleep(100);

    rc = pthread_create(&thread1, NULL, listener, &td1);

    if (rc) {
       std::cout << "Error:unable to create thread," << rc << std::endl;
       exit(-1);
    }

    std::cout << "Started1:" << td1.thread_id << ":" << td1.interface << std::endl;

    td2.thread_id = 2;
    td2.other_td = &td1;

    usleep(100);

    rc = pthread_create(&thread2, NULL, listener, &td2);

    if (rc) {
       std::cout << "Error:unable to create thread," << rc << std::endl;
       exit(-1);
    }

    std::cout << "Started2:" << td2.thread_id << ":" << td2.interface << std::endl;

//		/* these settings may be modified by recvmsg() */
//		iov.iov_len = sizeof(frame);
//		msg.msg_namelen = sizeof(addr);
//		msg.msg_controllen = sizeof(ctrlmsg);
//		msg.msg_flags = 0;


//		auto numBytes = recvmsg(sockfd, &msg, 0);


//		if (frame.can_id == 0x208) {
//			sendPacket(&frame, sockfd);
//		}
    pthread_exit(NULL);

    std::cout << std::endl << "Bye!" << std::endl;
    return EXIT_SUCCESS;
}
