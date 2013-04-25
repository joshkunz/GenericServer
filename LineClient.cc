#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include "LineClient.h"

#define BUFFER_SIZE 1024
#define LINE_SEP '\n'

/* initalize all of the variables with defaults */
LineClient::
LineClient()
    : socket(-1), broadcast(NULL), lock(NULL),
      mode(LINE),
      line_buffer(NULL), last(NULL), buffer_len(0),
      byte_count(0), character_count(0), characters_required(0)
      {}

/* Free the buffers and call the superclass destructor */
LineClient::
~LineClient() {
    Broadcast* broadcast = this->get_broadcast();
    broadcast->unsubscribe_all(this);
    free(this->line_buffer);
}

/* gettors and accessors */
void LineClient::
set_socket(int socket) { 
    this->socket = socket;
}

void LineClient::
set_write_lock(pthread_mutex_t * lock) {
    this->lock = lock;
}

void LineClient::
set_broadcast(Broadcast *broadcast) {
    this->broadcast = broadcast;
}

int LineClient::
get_socket() {
    return this->socket;
}

pthread_mutex_t * LineClient::
get_write_lock() {
    return this->lock;
}

Broadcast* LineClient::
get_broadcast() {
    return this->broadcast;
}

void LineClient::
set_raw(unsigned int count) {
    this->mode = RAW;
    this->characters_required = count;
}

int LineClient::
check_lines(int byte_count) {
    std::string tmp_string;
    int next_byte = 0;
    char * tmp_buffer;
    /* iterate through the new bytes */
    for (int i = 0; i < byte_count; i++) {
        this->byte_count += 1;
        /* check if this character is *not* a line seperator */
        if (this->last[i] != LINE_SEP) {
            continue;
        }

        /* the offset for the next byte */
        next_byte = this->byte_count;

        /* XXX: We're on a line sperator */

        /* Generate the string */
        tmp_string = std::string(this->line_buffer, this->byte_count - 1);

        /* Call the line reciver */
        this->receive_line(tmp_string);

        /*
        fprintf(stderr, "LineBuffer: %s.\n", this->line_buffer);
        fprintf(stderr, "Last: %s.\n", this->last);
        fprintf(stderr, "Alloc_len: %d.\n", byte_count - next_byte);
        fprintf(stderr, "Next_length: %lu.\n", strlen(this->last + (offset + next_byte)));
        */

        /* store a pointer to our current buffer */
        tmp_buffer = this->line_buffer;

        /* Allocate a new buffer for the remaining data. The +1 is because
         * of off by one errors. We're currently iterating over the terminator
         * but we want to include it in the input */
        this->line_buffer = (char *) malloc(sizeof(char) * (this->buffer_len - next_byte));

        /* copy the remaining data into it */
        memcpy(this->line_buffer, this->last + (i + 1), this->buffer_len - next_byte );
        //fprintf(stderr, "Copying: %s.\n", this->last + (offset + next_byte));

        /* free the old buffer */
        free(tmp_buffer);

        /* update the current_pointer */
        this->last = this->line_buffer;

        /* update the buffer length, more off-by-one fixes*/
        this->buffer_len -= next_byte;
        /* re-set the byte_count */
        this->byte_count = 0;

        /*
        fprintf(stderr, "### AFTER ###\n");

        fprintf(stderr, "LineBuffer: %s.\n", this->line_buffer);
        fprintf(stderr, "Last: %s.\n", this->last);
        fprintf(stderr, "Bufferlen: %d.\n", this->buffer_len);
        */

        /* return the number of byte's we've processed */
        return i + 1;
    }

    /* we processed all the bytes */
    return byte_count;
}

int LineClient::
check_chars(int byte_count) {
    /* special case for zero length checks */
    if (this->characters_required == 0) {
        this->receive_raw("");
        this->mode = LINE;
        return 0;
    }
    int len = 0; int next_byte = 0;
    this->last = this->line_buffer + this->byte_count;
    for (int i = 0; i < byte_count; i++) {
        /* get the length in bytes of the next character. Make sure
         * to read at maximum the remaining number of bytes */
        len = mblen(this->last + i, byte_count - i);
        /* If we didn't get a line, there's an error */
        if (len == -1) {
            perror("Error parsing unicode");
        /* otherwise we got a line, increment the counts */
        } else {
            //fprintf(stderr, "Got character of length %d.\n", len);
            //fprintf(stderr, "Got character %c.\n", this->last[i]);
            /* increment i by the number of bytes in the character */
            i += (len - 1);
            this->byte_count += len;
            this->character_count += 1;
        }

        /* If we have all of the characters we need */
        if (this->characters_required == this->character_count) {
            /* The 'next' byte */
            next_byte = i + 1;

            /* call the 'raw receive' method with the buffer, as a string */
            this->receive_raw(std::string(this->line_buffer, this->byte_count));

            /* Remove the old bytes from the buffer */

            /* store a pointer to the old buffer */
            char * tmp_buffer = this->line_buffer;
            /* create a new buffer for the remaining bytes */
            this->line_buffer = (char *) malloc(sizeof(char) * (this->buffer_len - next_byte));
            /* copy the remaining characters to the new buffer */
            memcpy(this->line_buffer, tmp_buffer + next_byte, this->buffer_len - next_byte);
            /* free up the old buffer */
            free(tmp_buffer);

            /* fix the buffer length */
            this->buffer_len -= next_byte;

            /* fix the 'last' pointer */
            this->last = this->line_buffer;

            /* re-set the 'raw mode' variables */
            this->mode = LINE;
            this->byte_count = 0;
            this->character_count = 0;
            this->characters_required = 0;

            /* return the number of bytes we proccessed */
            return next_byte;
        }
    }

    /* we wen't through all of the bytes */
    return byte_count;
}

/* Run the main loop */
int LineClient::
run() {
    /* make our buffer */
    char buff[BUFFER_SIZE];
    int bytes_recvd = 0;
    while (true) {
        bytes_recvd = recv(this->socket, buff, BUFFER_SIZE, 0);
        /* Check if the client has left */
        if (bytes_recvd == 0) {
            return 0;
        /* check if there was an error */
        } else if (bytes_recvd == -1) {
            perror("LineClient read");
            return -1;
        }

        /* update the buffer length */
        this->buffer_len += bytes_recvd;

        /* resize the buffer to hold our data */
        this->line_buffer = (char *) realloc(this->line_buffer, 
                                             this->buffer_len);
        /* update our buffer pointer */
        this->last = this->line_buffer + (this->buffer_len - bytes_recvd);

        /* Copy the data recived into the buffer */
        memcpy(this->last, buff, bytes_recvd);

        /* check all of our bytes for data */
        while (bytes_recvd > 0) {
            if (this->mode == LINE) {
                /* check for newlines and call the callbacks */
                bytes_recvd -= this->check_lines(bytes_recvd);
            } else {
                /* otherwise start counting characters */
                bytes_recvd -= this->check_chars(bytes_recvd);
            }
        }
    }
}

