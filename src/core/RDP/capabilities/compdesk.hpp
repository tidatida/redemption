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

   RDP Capabilities : Desktop Composition Extension Capability Set ([MS-RDPBCGR] section 2.2.7.2.8)

*/

#ifndef _REDEMPTION_CORE_RDP_CAPABILITIES_COMPDESK_HPP_
#define _REDEMPTION_CORE_RDP_CAPABILITIES_COMPDESK_HPP_

#include "common.hpp"
#include "stream.hpp"
#include "error.hpp"

// 2.2.7.2.8 Desktop Composition Capability Set (TS_COMPDESK_CAPABILITYSET)
// ========================================================================
// The TS_COMPDESK_CAPABILITYSET structure is used to support desktop composition. This
// capability is sent by both client and server.

// CompDeskSupportLevel (2 bytes): A 16-bit, unsigned integer. The desktop composition
//    support level.
//  +-------------------------+-------------------------------------------------+
//  | COMPDESK_NOT_SUPPORTED  | Desktop composition services are not supported. |
//  | 0x0000                  |                                                 |
//  +-------------------------+-------------------------------------------------+
//  | COMPDESK_SUPPORTED      | Desktop composition services are supported.     |
//  | 0x0001                  |                                                 |
//  +-------------------------+-------------------------------------------------+

enum {
      COMPDESK_NOT_SUPPORTED
    , COMPDESK_SUPPORTED
};

enum {
    CAPLEN_COMPDESK = 8
};


struct CompDeskCaps : public Capability {
    uint16_t CompDeskSupportLevel;

    CompDeskCaps()
    : Capability(CAPSETTYPE_COMPDESK, CAPLEN_COMPDESK)
    , CompDeskSupportLevel(COMPDESK_NOT_SUPPORTED) // By default, minimal
    {
    }

    void emit(OutStream & stream)override {
        stream.out_uint16_le(this->capabilityType);
        stream.out_uint16_le(this->len);
        stream.out_uint16_le(this->CompDeskSupportLevel);
    }

    void recv(InStream & stream, uint16_t len)override {
        this->len = len;

        if (!stream.in_check_rem(2)){
            LOG(LOG_ERR, "Truncated CompDeskCaps, need=2 remains=%u",
                stream.in_remain());
            throw Error(ERR_MCS_PDU_TRUNCATED);
        }

        this->CompDeskSupportLevel = stream.in_uint16_le();
    }

    void log(const char * msg)override {
        LOG(LOG_INFO, "%s CompDesk caps (%u bytes)", msg, this->len);
        LOG(LOG_INFO, "CompDeskCaps caps::CompDeskSupportLevel %u", this->CompDeskSupportLevel);
    }
};

#endif
