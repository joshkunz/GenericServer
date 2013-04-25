#include <string>
#include <map>
#include <list>
#include <set>
#include <pthread.h>

#ifndef BROADCAST_H
#define BROADCAST_H

class Client;

/* Broadcast a message to all clients */
class Broadcast {
    private:
        /* Channels and their subscribed clients */
        std::map <std::string, std::list<Client *> > channels;
        std::set <std::string> channel_list;

        pthread_mutex_t lock;
    public:
        /* default constructor */
        Broadcast();
        /* Subscribe to the specified channels */
        void subscribe(std::string channel, Client *);
        /* Unsubscribe a channel */
        void unsubscribe(std::string, Client *);
        /* remove the client from every list */
        void unsubscribe_all(Client *);
        /* return a list of channels available on the broadcast */
        std::list<std::string> get_channels();
        /* Broadcast a messages to all clients on the specified channels.
         * Returns 0 if the message was succesfully transmitted to all clients,
         * otherwise it returns the number of clients that the message
         * couldn't be sent to. 
         * The optional 'exclude' parameter is a client to exclude the message
         * from if they are in the channel. (i.e. don't send this message
         * to me.) */
        int broadcast(std::string channel, std::string msg, 
                      Client * exclude = NULL);
        
        /* Reports whether the client is subscribed to the broadcast */
        bool is_subscribed(std::string channel, Client * client);
};

#endif
