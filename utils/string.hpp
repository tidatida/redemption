/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Raphael Zhou
*/

#ifndef _REDEMPTION_UTILS_STRING_HPP_
#define _REDEMPTION_UTILS_STRING_HPP_

#include "error.hpp"
#include "log.hpp"

#define STRING_STATIC_BUFFER_SIZE 1024

namespace redemption {

class string {
protected:
    char static_buffer[STRING_STATIC_BUFFER_SIZE];

    char   * buffer_pointer;
    size_t   buffer_length;     /* Size of buffer pointed by buffer_pointer. */

public:
    string() {
        this->static_buffer[0] = 0;

        this->buffer_pointer = this->static_buffer;
        this->buffer_length  = sizeof(this->static_buffer);
    }

    string(const char * source) {
        this->static_buffer[0] = 0;

        this->buffer_pointer = this->static_buffer;
        this->buffer_length  = sizeof(this->static_buffer);

        this->copy_c_str(source);
    }

    string(const string & source) {
        this->static_buffer[0] = 0;

        this->buffer_pointer = this->static_buffer;
        this->buffer_length  = sizeof(this->static_buffer);

        this->copy_c_str(source.buffer_pointer);
    }

protected:
    string & operator=(const char * source) {
        return (*this);
    }

    string & operator=(const string & source) {
        return (*this);
    }

public:

    virtual ~string() {
        if (this->buffer_pointer != this->static_buffer) {
            delete [] this->buffer_pointer;
        }
    }

    const char * c_str() const {
        return this->buffer_pointer;
    }

    void concatenate_c_str(const char * source) {
        size_t source_length  = ::strlen(source);
        size_t content_length = ::strlen(this->buffer_pointer);

        realloc_memory(content_length + source_length + 1, true);

        ::strcpy(this->buffer_pointer + content_length, source);
    }

    void concatenate_str(const string & source) {
        concatenate_c_str(source.buffer_pointer);
    }

    void copy_c_str(const char * source) {
        size_t source_length = ::strlen(source);

        realloc_memory(source_length + 1, false);

        ::strcpy(this->buffer_pointer, source);
    }

    void copy_str(const string & source) {
        copy_c_str(source.buffer_pointer);
    }

    void empty() {
        this->buffer_pointer[0] = 0;
    }

    bool is_empty() const {
        return (this->buffer_pointer[0] == 0);
    }

    size_t length() const {
        return ::strlen(this->buffer_pointer);
    }

protected:
    // Ensure that the buffer is large enough to hold size bytes.
    void realloc_memory(size_t size, bool preserve_content) {
        if (this->buffer_length < size) {
            // Rounds new buffer length up to alignment boundary of 1024
            //     bytes.
            size_t new_buffer_length =
                ((size / 1024) + ((size % 1024) ? 1 : 0)) * 1024;

            char * new_buffer_pointer = new char[new_buffer_length];
            if (!new_buffer_pointer) {
                LOG(LOG_ERR, "Memory allocation failed");
                throw Error(ERR_MEMORY_ALLOCATION_FAILED);
            }

            if (preserve_content) {
               ::strcpy(new_buffer_pointer, this->buffer_pointer);
            }

            if (this->buffer_pointer != this->static_buffer) {
                delete [] this->buffer_pointer;
            }
            else {
                this->static_buffer[0] = 0;
            }

            this->buffer_pointer = new_buffer_pointer;
            this->buffer_length  = new_buffer_length;
        }
    }
};  // class string

}   // namespace redemption

#endif  // #ifndef _REDEMPTION_UTILS_STRING_HPP_