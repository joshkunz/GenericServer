#include <string>
#include <cstdio>
#include <cstring>
#include <exception>
#include "Server.h"
#include "Broadcast.h"

#include <cstdlib>

#define DEBUG

/* C libraries requried for threading */
#include <errno.h>
/* Threading */
#include <sys/types.h>
#include <pthread.h>
#include <cstdlib>
/* Sockets */
#include <sys/socket.h>
#include <netdb.h>
/* UTF-8 */
#include <locale.h>

#define BACKLOG 10

/* pthread entrypoint. This method calls the
 * clients's .run method once the thread has
 * started. */
void * __start(void * handler) {
    Client* r = (Client*) handler;
    int ret = r->run();
    /* XXX: Warning I'm not a hundred percent sure what
     * will happen when a mutex is destroyed while a client is
     * waiting on it. This may cause errors*/
    pthread_mutex_t * client_lock = r->get_write_lock();
    pthread_mutex_destroy(client_lock);
    /* free the lock's memory (see above) */
    free(client_lock);
    /* make sure this client's socket is closed */
    close(r->get_socket());
    /* Delete this client's object */
    delete r;
    fprintf(stderr, "Client left...\n");
    return (void *) ret;
}

/* send a string to this client's socket */
int Client::
send_message(std::string msg) {
    const char * bytes = msg.c_str();
#ifdef DEBUG
    printf("Sending message:\n%s.-.\n", bytes);
#endif
    int bytes_sent = 0,
        tmp_sent = 0,
        str_size = strlen(bytes),
        ret = 0;
    /* get the lock and socket of this client */
    pthread_mutex_t * lock = this->get_write_lock();
    int send_sock = this->get_socket();

    /* lock the client */
    pthread_mutex_lock(lock);
    /* send all of the bytes */
    while (bytes_sent < str_size) {
        /* Send as many bytes as possible */
        tmp_sent = send(send_sock, bytes + bytes_sent,
                        str_size - bytes_sent, 0);
        /* Check if send failed, and return the number of
         * bytes sent if the send failed */
        if (tmp_sent == -1) {
            ret = bytes_sent;
            break;
        }
        bytes_sent += tmp_sent;
    }
    pthread_mutex_unlock(lock);
    return ret;
}

/* Initialize the Server */
Server::Server(std::string i, std::string p)
    : interface(i), port(p), broadcast(new Broadcast) {}

/* delete our broadcast object when the server is killed */
Server::~Server() {
    delete broadcast;
}

void Server::
run(client_instance fetch_client) {
    /* Set the locale to english UTF-8 */
    setlocale(LC_ALL, "en_US.UTF-8");

    int sock = -1;
    struct addrinfo hints, *info;

    /* Initialize the addrinfo struct */
    memset(&hints, 0, sizeof(struct addrinfo));

    /* Some basic information about our protocol */
    hints.ai_family = AF_INET;       /* IPv4 */
    hints.ai_socktype = SOCK_STREAM; /* stream based sockets */
    hints.ai_protocol = IPPROTO_TCP; /* Use the TCP transport */

    /* The address of the interface we're serving on */
    getaddrinfo(this->interface.c_str(), this->port.c_str(), &hints, &info);

    /* Get a socket*/
    sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (sock == -1) {
        perror("Could not obtain a socket");
        throw std::exception();
    }

    /* Debug code to allow address reuse */
#ifdef DEBUG
    int _yes = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &_yes, sizeof(_yes)) == -1) {
        perror("DEBUG cannot set sockopt");
    }
#endif

    /* Bind the socket to our interface */
    if (bind(sock, info->ai_addr, info->ai_addrlen)) {
        perror("Could not bind to the specified interface");
        throw std::exception();
    }

    /* Listen on the interface */
    if (listen(sock, BACKLOG)) {
        perror("Could not listen on the specified interface");
        throw std::exception();
    }

    /* Variable to store thread IDs, we don't really care about */
    pthread_t thread_id;
    pthread_mutex_t * write_lock;

    printf("Serving on %s:%s\n", interface.c_str(), port.c_str());

    /* Wait for clients to connect */
    while (true) {
        /* Form a new client connection object for when a user connects */
        Client* handler = fetch_client();

        /* Get the new client's socket */
        int client_socket = accept(sock, NULL, NULL);
        printf("Client connection received...\n");

        /* set the socket on our hander */
        handler->set_socket(client_socket);

        /* Allocate a new lock */
        write_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));

        /* intialize the new lock */
        pthread_mutex_init(write_lock, NULL);

        /* Set the lock on the client */
        handler->set_write_lock(write_lock);

        /* Set the broadcast on the client */
        handler->set_broadcast(this->broadcast);

        /* fork the client into it's own thread */
        if (pthread_create(&thread_id, NULL,
                           __start, handler)) {
            perror("Failed to create a thread for the connection");
            throw std::exception();
        }
    }
}
