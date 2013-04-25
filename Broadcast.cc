#include <string>
#include <map>
#include <list>
#include <sys/socket.h>
#include <pthread.h>
#include <cstring>
#include "Server.h"

Broadcast::
Broadcast() {
    /* initialize this broadcast object's lock */
    pthread_mutex_init(&this->lock, NULL);
}

bool Broadcast::
is_subscribed(std::string channel, Client * client) {
    std::list<Client *>::iterator client_;
    bool ret = false;

    pthread_mutex_lock(&this->lock);
    /* traverse the list looking for a client match */
    for (client_ = this->channels[channel].begin();
         client_ != this->channels[channel].end(); ++client_) {
        if (client == *client_) {
            /* set the return value to true if found */
            ret = true;
        }
    }
    pthread_mutex_unlock(&this->lock);
    return ret;
}

void Broadcast::
subscribe(std::string channel, Client * client) {
    /* Add the client to this channel */
    pthread_mutex_lock(&this->lock);
    this->channels[channel].push_back(client);
    this->channel_list.insert(channel);
    pthread_mutex_unlock(&this->lock);
}

void Broadcast::
unsubscribe(std::string channel, Client * client) {
    /* Remove the client from this channel */
    pthread_mutex_lock(&this->lock);
    this->channels[channel].remove(client);
    /* if the channel is empty, remove it from the channel list */
    if (this->channels[channel].size() == 0) {
        this->channel_list.erase(channel);
    }
    pthread_mutex_unlock(&this->lock);
}

std::list<std::string> Broadcast::
get_channels() {
    std::list<std::string> keys;
    /* build a new list of channels */
    pthread_mutex_lock(&this->lock);
    for (std::set<std::string>::iterator chans = this->channel_list.begin();
         chans != this->channel_list.end(); ++chans) {
        keys.push_back(*chans);
    }
    pthread_mutex_unlock(&this->lock);
    return keys;
}


/* remove a client from all channels */
void Broadcast::
unsubscribe_all(Client * client) {
    std::list<std::string> keys = this->get_channels();

    /* unsubscribe this client from every channel */
    for (std::list<std::string>::iterator lchans = keys.begin();
         lchans != keys.end(); ++lchans) {
        this->unsubscribe(*lchans, client);
    }

}

int Broadcast::
broadcast(std::string channel, std::string msg, Client * exclude) {
    /* initialize all of our variables */
    int failed_count = 0;
    /* Get the list of clients in this channel */
    std::list<Client *> &clients = this->channels[channel];

    /* Iterate through all clients in this channel */
    pthread_mutex_lock(&this->lock);
    for (std::list<Client *>::iterator client = clients.begin(); 
         client != clients.end(); ++client) {
        /* skip this client if they are to be excluded */
        if (*client == exclude) { continue; }
        if((*client)->send_message(msg)) {
            failed_count++;
        }
    }
    pthread_mutex_unlock(&this->lock);

    return failed_count;
}
