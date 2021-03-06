/**
 * Copyright (c) 2010 Tom Burdick <thomas.burdick@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "http_message.h"

void octo_http_message_init(octo_http_message *message)
{
    octo_list_init(&message->message_queue);
    message->method = -1;

    octo_buffer_init(&message->path, 256);
    octo_buffer_init(&message->query, 256);
    message->http_major_version = 0;
    message->http_minor_version = 0;
    octo_hash_init(&message->headers, octo_default_hash_function, rand(), 6);
    octo_buffer_init(&message->body, 1024);
}

void octo_http_message_destroy(octo_http_message *message)
{
    octo_list_destroy(&message->message_queue);
    message->method = -1;

    octo_buffer_destroy(&message->path);
    octo_buffer_destroy(&message->query);
    message->http_major_version = 0;
    message->http_minor_version = 0;

    /*
    octo_hash_iterator iter;
    octo_http_header *header = octo_hash_iter(&message->headers, &iter);
    do {
        octo_http_header_free(header);
        octo_hash_iternext(iter);
    } while(header)
    */

    octo_hash_destroy(&message->headers);

    octo_buffer_destroy(&message->body);
}

octo_http_message * octo_http_message_alloc()
{
    return malloc(sizeof(octo_http_message));
}

void octo_http_message_free(octo_http_message *message)
{
    free(message);
}

octo_http_message * octo_http_message_new()
{
    octo_http_message *message = octo_http_message_alloc();
    octo_http_message_init(message);
    return message;
}

void octo_http_message_delete(octo_http_message *message)
{
    octo_http_message_destroy(message);
    octo_http_message_free(message);
}

void octo_http_message_add_header(octo_http_message *message, octo_http_header *header)
{
    octo_hash_put(&message->headers, &header->header_hash);
}

