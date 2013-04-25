#include <string>
#include <pthread.h>
#include "Server.h"
#include "Broadcast.h"

#ifndef LINE_CLIENT_H
#define LINE_CLIENT_H 

/* read state of the line client */
typedef enum buffer_state {
    LINE,   /* If we're currently receiving lines */
    RAW     /* If we're currently receiveing lines */
} buffer_state;

class LineClient : public virtual Client {
    public:
        /* receive a full line of data */
        virtual void receive_line(std::string) = 0;
        /* receive the chunk of 'raw' data previously asked for */
        virtual void receive_raw(std::string) = 0;

        /* ask the line reader for a chunk of raw data */
        void set_raw(unsigned int count);

        /* Getters and accessors */
        virtual void set_socket(int socket);
        virtual void set_write_lock(pthread_mutex_t *);
        virtual void set_broadcast(Broadcast *);

        virtual int get_socket();
        virtual pthread_mutex_t* get_write_lock();
        virtual Broadcast* get_broadcast();

        /* Conform to the Client class */
        LineClient();
        int run();

        /* our destructor */
        virtual ~LineClient();

    private:
        /* Client variables */
        int socket;
        Broadcast * broadcast;
        pthread_mutex_t * lock;

        /* read state */
        buffer_state mode;

        /* holds the buffer of lines */
        char *line_buffer;
        /* points to the end of the buffer */
        char *last;
        /* length of the buffer */
        int buffer_len;

        /* check if there's lines in the buffer */
        int check_lines(int byte_count);
        /* Check for more characters */
        int check_chars(int byte_count);

        /* the number of bytes we've traversed so far */
        unsigned int byte_count;
        /* the number of characters we've received so far */
        unsigned int character_count;
        /* the number of characters we need to get */
        unsigned int characters_required;
};

#endif
