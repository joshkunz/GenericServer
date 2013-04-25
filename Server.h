#include <string>
#include <map>
#include <pthread.h>
#include "Broadcast.h"

#ifndef SERVER_H
#define SERVER_H

/* Spawned By the server in a new thread for every
 * new connection */
class Client {
    public:
        /* The server-defined variables for this object */
        virtual void set_socket(int socket) = 0;
        virtual void set_write_lock(pthread_mutex_t *) = 0;
        virtual void set_broadcast(Broadcast *) = 0;

        /* variable accessors */
        virtual int get_socket() = 0;
        virtual pthread_mutex_t* get_write_lock() = 0;
        virtual Broadcast* get_broadcast() = 0;

        /* send a message to this client */
        virtual int send_message(std::string msg);

        /* Called whent the client is in its own thread of control.
         * This is the main entry point for the application. */
        virtual int run() = 0;
        virtual ~Client() {}
};

typedef Client * (*client_instance)();

class Server {
    private:
        /* interface we're running on */
        std::string interface;
        /* port we're running on */
        std::string port; 
        /* The server's broadcast object */
        Broadcast * broadcast;
    public:
        /* Create a server that runs on interface 'interface' and
         * serves on port 'port' */
        Server(std::string interface, std::string port);

        /* Run the server. This call will terminate when the server is
         * shutdown. */
        void run(client_instance);

        ~Server();
};

#endif
