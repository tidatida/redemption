/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2012
   Author(s): Christophe Grosjean

   Transport layer abstraction
*/

#ifndef REDEMPTION_TRANSPORT_OUT_FILENAME_TRANSPORT_HPP
#define REDEMPTION_TRANSPORT_OUT_FILENAME_TRANSPORT_HPP

// #include "buffer/buffering_buf.hpp"
#include "transport/mixin_transport.hpp"
#include "transport/buffer/file_buf.hpp"
#include "transport/buffer/crypto_filename_buf.hpp"


struct OutFilenameTransport
: SeekableTransport<OutputTransport<transbuf::ofile_buf>>
{
    OutFilenameTransport(const char * filename)
    {
        if (this->buffer().open(filename, 0600) < 0) {
            LOG(LOG_ERR, "failed opening=%s\n", filename);
            throw Error(ERR_TRANSPORT_OPEN_FAILED);
        }
    }
};

struct CryptoOutFilenameTransport
: OutputTransport<transbuf::ocrypto_filename_buf>
{
    CryptoOutFilenameTransport(CryptoContext * crypto_ctx, const char * filename, auth_api * authentifier = nullptr)
    : CryptoOutFilenameTransport::TransportType(crypto_ctx)
    {
        if (this->buffer().open(filename, 0600) < 0) {
            LOG(LOG_ERR, "failed opening=%s\n", filename);
            throw Error(ERR_TRANSPORT_OPEN_FAILED);
        }

        if (authentifier) {
            this->set_authentifier(authentifier);
        }
    }
};

#endif
