// --- includes ----------------------------------------------------------------
#include <net/coap.h>
#include <net/socket.h>
#include <zephyr.h>
#include <logging/log.h>
#include "coap_client.h"
#include <random/rand32.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// --- logging settings --------------------------------------------------------
LOG_MODULE_REGISTER(coap_client_m);

// --- defines -----------------------------------------------------------------
#define COAP_SERVER_PORT 5683
#define APP_COAP_VERSION 1
#define COAP_POLL_TIMEOUT_MS 1000
// TODO: future work: create a ble characteristic on nrf52840 in order for user
// to set up the central node and add the server hostname and also the credentials
// for the server
#define COAP_SERVER_HOSTNAME "gpappasv.dynv6.net"
// -- static variables definitions ---------------------------------------------
// UDP socket
static int sock;
// File descriptor
static struct pollfd fds;
// UDP server to connect
static struct sockaddr_storage server;
// COAP token
static uint16_t next_token;
// COAP buffer
static uint8_t coap_buf[APP_COAP_MAX_MSG_LEN];
// Token for userpayload resource observation
// Hardcoded token to avoid issues with server if 9160 resets
static uint16_t obs_token = 0x9889;
static struct k_timer obs_renew_timer;
static struct k_work user_payload_obs_renew_work;

// --- static functions declarations -------------------------------------------
static void udp_server_init(void);
static int wait(void);
static void renew_coap_observe(struct k_work *work);
static void coap_obs_renew_timer_handler(struct k_timer *timer_id);

// --- static functions definitions --------------------------------------------
/**
 * @brief Function to initialize udp server to connect to
 *
 */
static void udp_server_init(void)
{
    int err;
    struct addrinfo *result;
    struct addrinfo hints = {
        .ai_family = AF_INET6,
        .ai_socktype = SOCK_DGRAM};
    char ipv6_addr[NET_IPV6_ADDR_LEN];
    err = getaddrinfo(COAP_SERVER_HOSTNAME, NULL, &hints, &result);
    if (err != 0)
    {
        LOG_INF("ERROR: getaddrinfo failed %d", err);
    }
    if (result == NULL)
    {
        LOG_INF("ERROR: Address not found");
    }

    /* IPv6 Address. */
    struct sockaddr_in6 *server6 = ((struct sockaddr_in6 *)&server);

    memcpy(server6->sin6_addr.s6_addr, ((struct sockaddr_in6 *)result->ai_addr)->sin6_addr.s6_addr, 16);
    server6->sin6_family = AF_INET6;
    server6->sin6_port = htons(COAP_SERVER_PORT);
    server6->sin6_scope_id = 0;

    inet_ntop(AF_INET6, &server6->sin6_addr.s6_addr, ipv6_addr,
              sizeof(ipv6_addr));
    /* Free the address. */
    freeaddrinfo(result);
}

/**
 * @brief Poll wait
 *
 * @param timeout
 * @return int
 */
static int wait(void)
{
    // https://man7.org/linux/man-pages/man2/poll.2.html Info about poll
    int ret = poll(&fds, 1, COAP_POLL_TIMEOUT_MS);

    // --- error checking ---
    if (ret < 0)
    {
        LOG_INF("poll error: %d", errno);
        return -errno;
    }

    if (ret == 0)
    {
        /* Timeout. */
        return -EAGAIN;
    }

    if ((fds.revents & POLLERR) == POLLERR)
    {
        LOG_INF("wait: POLLERR");
        return -EIO;
    }

    if ((fds.revents & POLLNVAL) == POLLNVAL)
    {
        LOG_INF("wait: POLLNVAL");
        return -EBADF;
    }

    if ((fds.revents & POLLIN) != POLLIN)
    {
        return -EAGAIN;
    }

    return 0;
}

/**
 * @brief Function that is called when obs_renew_timer triggers (once every 5mins)
 * 
 * @param timer_id 
 */
static void coap_obs_renew_timer_handler(struct k_timer *timer_id)
{
    k_work_submit(&user_payload_obs_renew_work);
}

/**
 * @brief Handler to renew the observation on userpayload coap resource
 *        This is a function for the workqueue user_payload_obs_renew_work
 *        This workqueue is updated via a timer
 *
 * @param work
 */
static void renew_coap_observe(struct k_work *work)
{
    // Register observe on the userpayload resource
    char obs_resource[] = "userpayload";
    coap_observe(obs_resource, strlen(obs_resource));
    LOG_INF("Just updated coap observe for userpayload resource");
}

// --- functions definitions ---------------------------------------------------
/**
 * @brief
 *
 * @return int
 */
int coap_get_socket(void)
{
    return sock;
}
/**
 * @brief Init coap client
 *
 * @return int
 */
void coap_client_init(void)
{
    int err;

    // Initialize server that we will connect to
    udp_server_init();

    sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        LOG_INF("Failed to create CoAP socket: %d.", errno);
        return;
    }

    // Connect to server
    err = connect(sock, (struct sockaddr *)&server,
                  sizeof(struct sockaddr_in6));
    if (err < 0)
    {
        LOG_INF("Connect failed : %d", errno);
        return;
    }

    // Initialize FDS, for poll.
    fds.fd = sock;
    fds.events = POLLIN;

    // Randomize token that will be used on coap put transactions
    next_token = sys_rand32_get();
    // TODO: do we need this k_sleep()??
    k_sleep(K_MSEC(1000));
}

/**
 * @brief create a coap put request on a resource
 *
 * @param resource
 * @param resourse_length
 * @param payload
 */
void coap_put(uint8_t *resource, uint16_t resourse_length, uint8_t *payload, uint8_t length)
{
    int err;
    struct coap_packet request;

    // --- clear coap buffer
    memset(coap_buf, 0, sizeof(coap_buf));

    // Token must be unique for each request
    next_token++;

    // --- init coap packet
    err = coap_packet_init(&request, coap_buf, sizeof(coap_buf),
                           APP_COAP_VERSION, COAP_TYPE_NON_CON,
                           sizeof(next_token), (uint8_t *)&next_token,
                           COAP_METHOD_PUT, coap_next_id());
    if (err < 0)
    {
        LOG_INF("Failed to create CoAP request, %d", err);
        return;
    }

    // Append option to the coap packet
    err = coap_packet_append_option(&request, COAP_OPTION_URI_PATH, resource, resourse_length);
    if (err < 0)
    {
        LOG_INF("Failed to encode CoAP option, %d", err);
        return;
    }
    err = coap_packet_append_payload_marker(&request);
    if (err < 0)
    {
        LOG_INF("Unable to append payload marker %d", err);
        return;
    }
    // Add payload to the packet
    err = coap_packet_append_payload(&request, (uint8_t *)payload,
                                     length);
    if (err < 0)
    {
        LOG_INF("Not able to append payload %d", err);
        return;
    }

    // Send the coap request
    err = send(sock, request.data, request.offset, 0);

    wait();
}

/**
 * @brief Function to observe a coap resource
 *        Just sends the observe request
 *          TODO: might need function to reset the observe request
 * @param resource
 * @param resourse_length
 */
void coap_observe(uint8_t *resource, uint16_t resourse_length)
{
    int err;
    struct coap_packet request;
    // --- clear coap buffer
    memset(coap_buf, 0, sizeof(coap_buf));

    // --- init coap packet
    err = coap_packet_init(&request, coap_buf, sizeof(coap_buf),
                           APP_COAP_VERSION, COAP_TYPE_NON_CON,
                           sizeof(uint16_t), (uint8_t *)&obs_token,
                           COAP_METHOD_GET, coap_next_id());
    if (err < 0)
    {
        LOG_INF("Failed to create CoAP request, %d", err);
        return;
    }

    // Append option to the coap packet
    err = coap_append_option_int(&request, COAP_OPTION_OBSERVE, 0);
    err = coap_packet_append_option(&request, COAP_OPTION_URI_PATH, resource, resourse_length);
    if (err < 0)
    {
        LOG_INF("Failed to encode CoAP option, %d", err);
        return;
    }

    err = send(sock, request.data, request.offset, 0);

    wait();
}

/**
 * @brief Get the token to observe the userpayload resource
 * 
 * @return uint16_t* the observation token for userpayload resource
 */
uint16_t *get_obs_token(void)
{
    return &obs_token;
}

/**
 * @brief Initialize workitem and timer for observe renew for coap userpayload 
 *        resource
 * 
 */
void initialize_observe_renew(void)
{
    // Initialize the work item user_payload_obs_renew_work which will renew the
    // observation on userpayload resource
    k_work_init(&user_payload_obs_renew_work, renew_coap_observe);
    k_timer_init(&obs_renew_timer, coap_obs_renew_timer_handler, NULL);
    // This timer will go off in 150 seconds and will trigger every 150 seconds
    k_timer_start(&obs_renew_timer, K_SECONDS(600), K_SECONDS(600));
}