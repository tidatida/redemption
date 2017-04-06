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
  Copyright (C) Wallix 2013
  Author(s): Christophe Grosjean, Raphael Zhou, Meng Tan
*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestCredSSP
#include "system/redemption_unit_tests.hpp"

#define LOGNULL

#include "core/RDP/nla/credssp.hpp"

#include "check_sig.hpp"

BOOST_AUTO_TEST_CASE(TestTSRequest)
{

    // ===== NTLMSSP_NEGOTIATE =====
    constexpr static uint8_t packet[] = {
        0x30, 0x37, 0xa0, 0x03, 0x02, 0x01, 0x02, 0xa1,
        0x30, 0x30, 0x2e, 0x30, 0x2c, 0xa0, 0x2a, 0x04,
        0x28, 0x4e, 0x54, 0x4c, 0x4d, 0x53, 0x53, 0x50,
        0x00, 0x01, 0x00, 0x00, 0x00, 0xb7, 0x82, 0x08,
        0xe2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x05, 0x01, 0x28, 0x0a, 0x00, 0x00, 0x00,
        0x0f
    };

    StaticOutStream<65536> s;

    s.out_copy_bytes(packet, sizeof(packet));

    uint8_t sig1[SslSha1::DIGEST_LENGTH];
    get_sig(s, sig1);

    InStream in_s(s.get_data(), s.get_offset());
    TSRequest ts_req(in_s);

    BOOST_CHECK_EQUAL(ts_req.version, 2);

    BOOST_CHECK_EQUAL(ts_req.negoTokens.size(), 0x28);
    BOOST_CHECK_EQUAL(ts_req.authInfo.size(), 0);
    BOOST_CHECK_EQUAL(ts_req.pubKeyAuth.size(), 0);

    StaticOutStream<65536> to_send;

    ts_req.emit(to_send);

    BOOST_CHECK_EQUAL(to_send.get_offset(), 0x37 + 2);

    CHECK_SIG(to_send, sig1);

    // hexdump_c(to_send.get_data(), to_send.size());

    // ===== NTLMSSP_CHALLENGE =====
    constexpr static uint8_t packet2[] = {
        0x30, 0x81, 0x94, 0xa0, 0x03, 0x02, 0x01, 0x02,
        0xa1, 0x81, 0x8c, 0x30, 0x81, 0x89, 0x30, 0x81,
        0x86, 0xa0, 0x81, 0x83, 0x04, 0x81, 0x80, 0x4e,
        0x54, 0x4c, 0x4d, 0x53, 0x53, 0x50, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x38,
        0x00, 0x00, 0x00, 0x35, 0x82, 0x8a, 0xe2, 0x26,
        0x6e, 0xcd, 0x75, 0xaa, 0x41, 0xe7, 0x6f, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
        0x00, 0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0x06,
        0x01, 0xb0, 0x1d, 0x00, 0x00, 0x00, 0x0f, 0x57,
        0x00, 0x49, 0x00, 0x4e, 0x00, 0x37, 0x00, 0x02,
        0x00, 0x08, 0x00, 0x57, 0x00, 0x49, 0x00, 0x4e,
        0x00, 0x37, 0x00, 0x01, 0x00, 0x08, 0x00, 0x57,
        0x00, 0x49, 0x00, 0x4e, 0x00, 0x37, 0x00, 0x04,
        0x00, 0x08, 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e,
        0x00, 0x37, 0x00, 0x03, 0x00, 0x08, 0x00, 0x77,
        0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x07,
        0x00, 0x08, 0x00, 0xa9, 0x8d, 0x9b, 0x1a, 0x6c,
        0xb0, 0xcb, 0x01, 0x00, 0x00, 0x00, 0x00
    };


    LOG(LOG_INFO, "=================================\n");
    s.rewind();
    s.out_copy_bytes(packet2, sizeof(packet2));

    uint8_t sig2[SslSha1::DIGEST_LENGTH];
    get_sig(s, sig2);

    in_s = InStream(s.get_data(), s.get_offset());
    TSRequest ts_req2(in_s);

    BOOST_CHECK_EQUAL(ts_req2.version, 2);

    BOOST_CHECK_EQUAL(ts_req2.negoTokens.size(), 0x80);
    BOOST_CHECK_EQUAL(ts_req2.authInfo.size(), 0);
    BOOST_CHECK_EQUAL(ts_req2.pubKeyAuth.size(), 0);

    StaticOutStream<65536> to_send2;

    BOOST_CHECK_EQUAL(to_send2.get_offset(), 0);
    ts_req2.emit(to_send2);

    BOOST_CHECK_EQUAL(to_send2.get_offset(), 0x94 + 3);

    CHECK_SIG(to_send2, sig2);

    // hexdump_c(to_send2.get_data(), to_send2.size());

    // ===== NTLMSSP_AUTH =====
    constexpr static uint8_t packet3[] = {
        0x30, 0x82, 0x02, 0x41, 0xa0, 0x03, 0x02, 0x01,
        0x02, 0xa1, 0x82, 0x01, 0x12, 0x30, 0x82, 0x01,
        0x0e, 0x30, 0x82, 0x01, 0x0a, 0xa0, 0x82, 0x01,
        0x06, 0x04, 0x82, 0x01, 0x02, 0x4e, 0x54, 0x4c,
        0x4d, 0x53, 0x53, 0x50, 0x00, 0x03, 0x00, 0x00,
        0x00, 0x18, 0x00, 0x18, 0x00, 0x6a, 0x00, 0x00,
        0x00, 0x70, 0x00, 0x70, 0x00, 0x82, 0x00, 0x00,
        0x00, 0x08, 0x00, 0x08, 0x00, 0x48, 0x00, 0x00,
        0x00, 0x10, 0x00, 0x10, 0x00, 0x50, 0x00, 0x00,
        0x00, 0x0a, 0x00, 0x0a, 0x00, 0x60, 0x00, 0x00,
        0x00, 0x10, 0x00, 0x10, 0x00, 0xf2, 0x00, 0x00,
        0x00, 0x35, 0x82, 0x88, 0xe2, 0x05, 0x01, 0x28,
        0x0a, 0x00, 0x00, 0x00, 0x0f, 0x77, 0x00, 0x69,
        0x00, 0x6e, 0x00, 0x37, 0x00, 0x75, 0x00, 0x73,
        0x00, 0x65, 0x00, 0x72, 0x00, 0x6e, 0x00, 0x61,
        0x00, 0x6d, 0x00, 0x65, 0x00, 0x57, 0x00, 0x49,
        0x00, 0x4e, 0x00, 0x58, 0x00, 0x50, 0x00, 0xa0,
        0x98, 0x01, 0x10, 0x19, 0xbb, 0x5d, 0x00, 0xf6,
        0xbe, 0x00, 0x33, 0x90, 0x20, 0x34, 0xb3, 0x47,
        0xa2, 0xe5, 0xcf, 0x27, 0xf7, 0x3c, 0x43, 0x01,
        0x4a, 0xd0, 0x8c, 0x24, 0xb4, 0x90, 0x74, 0x39,
        0x68, 0xe8, 0xbd, 0x0d, 0x2b, 0x70, 0x10, 0x01,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc3,
        0x83, 0xa2, 0x1c, 0x6c, 0xb0, 0xcb, 0x01, 0x47,
        0xa2, 0xe5, 0xcf, 0x27, 0xf7, 0x3c, 0x43, 0x00,
        0x00, 0x00, 0x00, 0x02, 0x00, 0x08, 0x00, 0x57,
        0x00, 0x49, 0x00, 0x4e, 0x00, 0x37, 0x00, 0x01,
        0x00, 0x08, 0x00, 0x57, 0x00, 0x49, 0x00, 0x4e,
        0x00, 0x37, 0x00, 0x04, 0x00, 0x08, 0x00, 0x77,
        0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x03,
        0x00, 0x08, 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e,
        0x00, 0x37, 0x00, 0x07, 0x00, 0x08, 0x00, 0xa9,
        0x8d, 0x9b, 0x1a, 0x6c, 0xb0, 0xcb, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb1,
        0xd2, 0x45, 0x42, 0x0f, 0x37, 0x9a, 0x0e, 0xe0,
        0xce, 0x77, 0x40, 0x10, 0x8a, 0xda, 0xba, 0xa3,
        0x82, 0x01, 0x22, 0x04, 0x82, 0x01, 0x1e, 0x01,
        0x00, 0x00, 0x00, 0x91, 0x5e, 0xb0, 0x6e, 0x72,
        0x82, 0x53, 0xae, 0x00, 0x00, 0x00, 0x00, 0x27,
        0x29, 0x73, 0xa9, 0xfa, 0x46, 0x17, 0x3c, 0x74,
        0x14, 0x45, 0x2a, 0xd1, 0xe2, 0x92, 0xa1, 0xc6,
        0x0a, 0x30, 0xd4, 0xcc, 0xe0, 0x92, 0xf6, 0xb3,
        0x20, 0xb3, 0xa0, 0xf1, 0x38, 0xb1, 0xf4, 0xe5,
        0x96, 0xdf, 0xa1, 0x65, 0x5b, 0xd6, 0x0c, 0x2a,
        0x86, 0x99, 0xcc, 0x72, 0x80, 0xbd, 0xe9, 0x19,
        0x1f, 0x42, 0x53, 0xf6, 0x84, 0xa3, 0xda, 0x0e,
        0xec, 0x10, 0x29, 0x15, 0x52, 0x5c, 0x77, 0x40,
        0xc8, 0x3d, 0x44, 0x01, 0x34, 0xb6, 0x0a, 0x75,
        0x33, 0xc0, 0x25, 0x71, 0xd3, 0x25, 0x38, 0x3b,
        0xfc, 0x3b, 0xa8, 0xcf, 0xba, 0x2b, 0xf6, 0x99,
        0x0e, 0x5f, 0x4e, 0xa9, 0x16, 0x2b, 0x52, 0x9f,
        0xbb, 0x76, 0xf8, 0x03, 0xfc, 0x11, 0x5e, 0x36,
        0x83, 0xd8, 0x4c, 0x9a, 0xdc, 0x9d, 0x35, 0xe2,
        0xc8, 0x63, 0xa9, 0x3d, 0x07, 0x97, 0x52, 0x64,
        0x54, 0x72, 0x9e, 0x9a, 0x8c, 0x56, 0x79, 0x4a,
        0x78, 0x91, 0x0a, 0x4c, 0x52, 0x84, 0x5a, 0x4a,
        0xb8, 0x28, 0x0b, 0x2f, 0xe6, 0x89, 0x7d, 0x07,
        0x3b, 0x7b, 0x6e, 0x22, 0xcc, 0x4c, 0xff, 0xf4,
        0x10, 0x96, 0xf2, 0x27, 0x29, 0xa0, 0x76, 0x0d,
        0x4c, 0x7e, 0x7a, 0x42, 0xe4, 0x1e, 0x6a, 0x95,
        0x7d, 0x4c, 0xaf, 0xdb, 0x86, 0x49, 0x5c, 0xbf,
        0xc2, 0x65, 0xb6, 0xf2, 0xed, 0xae, 0x8d, 0x57,
        0xed, 0xf0, 0xd4, 0xcb, 0x7a, 0xbb, 0x23, 0xde,
        0xe3, 0x43, 0xea, 0xb1, 0x02, 0xe3, 0xb4, 0x96,
        0xe9, 0xe7, 0x48, 0x69, 0xb0, 0xaa, 0xec, 0x89,
        0x38, 0x8b, 0xc2, 0xbd, 0xdd, 0xf7, 0xdf, 0xa1,
        0x37, 0xe7, 0x34, 0x72, 0x7f, 0x91, 0x10, 0x14,
        0x73, 0xfe, 0x32, 0xdc, 0xfe, 0x68, 0x2b, 0xc0,
        0x08, 0xdf, 0x05, 0xf7, 0xbd, 0x46, 0x33, 0xfb,
        0xc9, 0xfc, 0x89, 0xaa, 0x5d, 0x25, 0x49, 0xc8,
        0x6e, 0x86, 0xee, 0xc2, 0xce, 0xc4, 0x8e, 0x85,
        0x9f, 0xe8, 0x30, 0xb3, 0x86, 0x11, 0xd5, 0xb8,
        0x34, 0x4a, 0xe0, 0x03, 0xe5
    };

    LOG(LOG_INFO, "=================================\n");
    s.rewind();
    s.out_copy_bytes(packet3, sizeof(packet3));

    uint8_t sig3[SslSha1::DIGEST_LENGTH];
    get_sig(s, sig3);

    TSRequest ts_req3;

    in_s = InStream(s.get_data(), s.get_offset());
    ts_req3.recv(in_s);

    BOOST_CHECK_EQUAL(ts_req3.version, 2);

    BOOST_CHECK_EQUAL(ts_req3.negoTokens.size(), 0x102);
    BOOST_CHECK_EQUAL(ts_req3.authInfo.size(), 0);
    BOOST_CHECK_EQUAL(ts_req3.pubKeyAuth.size(), 0x11e);

    StaticOutStream<65536> to_send3;

    BOOST_CHECK_EQUAL(to_send3.get_offset(), 0);
    ts_req3.emit(to_send3);

    BOOST_CHECK_EQUAL(to_send3.get_offset(), 0x241 + 4);

    CHECK_SIG(to_send3, sig3);

    // hexdump_c(to_send3.get_data(), to_send3.size());

    // ===== PUBKEYAUTH =====
    constexpr static uint8_t packet4[] = {
        0x30, 0x82, 0x01, 0x2b, 0xa0, 0x03, 0x02, 0x01,
        0x02, 0xa3, 0x82, 0x01, 0x22, 0x04, 0x82, 0x01,
        0x1e, 0x01, 0x00, 0x00, 0x00, 0xc9, 0x88, 0xfc,
        0xf1, 0x11, 0x68, 0x2c, 0x72, 0x00, 0x00, 0x00,
        0x00, 0xc7, 0x51, 0xf4, 0x71, 0xd3, 0x9f, 0xb6,
        0x50, 0xbe, 0xa8, 0xf6, 0x20, 0x77, 0xa1, 0xfc,
        0xdd, 0x8e, 0x02, 0xf0, 0xa4, 0x6b, 0xba, 0x3f,
        0x9d, 0x65, 0x9d, 0xab, 0x4a, 0x95, 0xc9, 0xb4,
        0x38, 0x03, 0x87, 0x04, 0xb1, 0xfe, 0x42, 0xec,
        0xfa, 0xfc, 0xaa, 0x85, 0xf1, 0x31, 0x2d, 0x26,
        0xcf, 0x63, 0xfd, 0x62, 0x36, 0xcf, 0x56, 0xc3,
        0xfb, 0xf6, 0x36, 0x9b, 0xe5, 0xb2, 0xe7, 0xce,
        0xcb, 0xe1, 0x82, 0xb2, 0x89, 0xff, 0xdd, 0x87,
        0x5e, 0xd3, 0xd8, 0xff, 0x2e, 0x16, 0x35, 0xad,
        0xdb, 0xda, 0xc9, 0xc5, 0x81, 0xad, 0x48, 0xf1,
        0x8b, 0x76, 0x3d, 0x74, 0x34, 0xdf, 0x80, 0x6b,
        0xf3, 0x68, 0x6d, 0xf6, 0xec, 0x5f, 0xbe, 0xea,
        0xb7, 0x6c, 0xea, 0xe4, 0xeb, 0xe9, 0x17, 0xf9,
        0x4e, 0x0d, 0x79, 0xd5, 0x82, 0xdd, 0xb7, 0xdc,
        0xcd, 0xfc, 0xbb, 0xf1, 0x0b, 0x9b, 0xe9, 0x18,
        0xe7, 0xb3, 0xb3, 0x8b, 0x40, 0x82, 0xa0, 0x9d,
        0x58, 0x73, 0xda, 0x54, 0xa2, 0x2b, 0xd2, 0xb6,
        0x41, 0x60, 0x8a, 0x64, 0xf2, 0xa2, 0x59, 0x64,
        0xcf, 0x27, 0x1a, 0xe6, 0xb5, 0x1a, 0x0e, 0x0e,
        0xe1, 0x14, 0xef, 0x26, 0x68, 0xeb, 0xc8, 0x49,
        0xe2, 0x66, 0xbb, 0x11, 0x71, 0x49, 0xad, 0x7e,
        0xae, 0xde, 0xa8, 0x78, 0xfd, 0x64, 0x51, 0xd8,
        0x18, 0x01, 0x11, 0xc0, 0x8d, 0x3b, 0xec, 0x40,
        0x2b, 0x1f, 0xc5, 0xa4, 0x45, 0x1e, 0x07, 0xae,
        0x5a, 0xd8, 0x1c, 0xab, 0xdf, 0x89, 0x96, 0xdc,
        0xdc, 0x29, 0xd8, 0x30, 0xdb, 0xbf, 0x48, 0x2a,
        0x42, 0x27, 0xc2, 0x50, 0xac, 0xf9, 0x02, 0xd1,
        0x20, 0x12, 0xdd, 0x50, 0x22, 0x09, 0x44, 0xac,
        0xe0, 0x22, 0x1f, 0x66, 0x64, 0xec, 0xfa, 0x2b,
        0xb8, 0xcd, 0x43, 0x3a, 0xce, 0x40, 0x74, 0xe1,
        0x34, 0x81, 0xe3, 0x94, 0x47, 0x6f, 0x49, 0x01,
        0xf8, 0xb5, 0xfc, 0xd0, 0x75, 0x80, 0xc6, 0x35,
        0xac, 0xc0, 0xfd, 0x1b, 0xb5, 0xa2, 0xd3
    };

    LOG(LOG_INFO, "=================================\n");
    s.rewind();
    s.out_copy_bytes(packet4, sizeof(packet4));
    uint8_t sig4[SslSha1::DIGEST_LENGTH];
    get_sig(s, sig4);

    in_s = InStream(s.get_data(), s.get_offset());
    TSRequest ts_req4(in_s);

    BOOST_CHECK_EQUAL(ts_req4.version, 2);

    BOOST_CHECK_EQUAL(ts_req4.negoTokens.size(), 0);
    BOOST_CHECK_EQUAL(ts_req4.authInfo.size(), 0);
    BOOST_CHECK_EQUAL(ts_req4.pubKeyAuth.size(), 0x11e);

    StaticOutStream<65536> to_send4;

    BOOST_CHECK_EQUAL(to_send4.get_offset(), 0);
    ts_req4.emit(to_send4);

    BOOST_CHECK_EQUAL(to_send4.get_offset(), 0x12b + 4);

    CHECK_SIG(to_send4, sig4);

    // hexdump_c(to_send4.get_data(), to_send4.size());


    // ===== AUTHINFO =====
    constexpr static uint8_t packet5[] = {
        0x30, 0x5a, 0xa0, 0x03, 0x02, 0x01, 0x02, 0xa2,
        0x53, 0x04, 0x51, 0x01, 0x00, 0x00, 0x00, 0xb3,
        0x2c, 0x3b, 0xa1, 0x36, 0xf6, 0x55, 0x71, 0x01,
        0x00, 0x00, 0x00, 0xa8, 0x85, 0x7d, 0x11, 0xef,
        0x92, 0xa0, 0xd6, 0xff, 0xee, 0xa1, 0xae, 0x6d,
        0xc5, 0x2e, 0x4e, 0x65, 0x50, 0x28, 0x93, 0x75,
        0x30, 0xe1, 0xc3, 0x37, 0xeb, 0xac, 0x1f, 0xdd,
        0xf3, 0xe0, 0x92, 0xf6, 0x21, 0xbc, 0x8f, 0xa8,
        0xd4, 0xe0, 0x5a, 0xa6, 0xff, 0xda, 0x09, 0x50,
        0x24, 0x0d, 0x8f, 0x8f, 0xf4, 0x92, 0xfe, 0x49,
        0x2a, 0x13, 0x52, 0xa6, 0x52, 0x75, 0x50, 0x8d,
        0x3e, 0xe9, 0x6b, 0x57
    };
    LOG(LOG_INFO, "=================================\n");
    s.rewind();
    s.out_copy_bytes(packet5, sizeof(packet5));
    uint8_t sig5[SslSha1::DIGEST_LENGTH];
    get_sig(s, sig5);

    in_s = InStream(s.get_data(), s.get_offset());
    TSRequest ts_req5(in_s);

    BOOST_CHECK_EQUAL(ts_req5.version, 2);

    BOOST_CHECK_EQUAL(ts_req5.negoTokens.size(), 0);
    BOOST_CHECK_EQUAL(ts_req5.authInfo.size(), 0x51);
    BOOST_CHECK_EQUAL(ts_req5.pubKeyAuth.size(), 0);

    StaticOutStream<65536> to_send5;

    BOOST_CHECK_EQUAL(to_send5.get_offset(), 0);
    ts_req5.emit(to_send5);

    BOOST_CHECK_EQUAL(to_send5.get_offset(), 0x5a + 2);

    CHECK_SIG(to_send5, sig5);

    // hexdump_c(to_send5.get_data(), to_send5.size());
}


BOOST_AUTO_TEST_CASE(TestTSCredentialsPassword)
{

    uint8_t domain[] = "flatland";
    uint8_t user[] = "square";
    uint8_t pass[] = "hypercube";


    TSCredentials ts_cred(domain, sizeof(domain),
                          user,   sizeof(user),
                          pass,   sizeof(pass));

    StaticOutStream<65536> s;

    int emited = ts_cred.emit(s);
    BOOST_CHECK_EQUAL(s.get_offset(), *(s.get_data() + 1) + 2);
    BOOST_CHECK_EQUAL(s.get_offset(), emited);

    TSCredentials ts_cred_received;

    InStream in_s(s.get_data(), s.get_offset());
    ts_cred_received.recv(in_s);

    BOOST_CHECK_EQUAL(ts_cred_received.credType, 1);
    BOOST_CHECK_EQUAL(ts_cred_received.passCreds.domainName_length, sizeof(domain));
    BOOST_CHECK_EQUAL(ts_cred_received.passCreds.userName_length,   sizeof(user));
    BOOST_CHECK_EQUAL(ts_cred_received.passCreds.password_length,   sizeof(pass));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred_received.passCreds.domainName),
                      reinterpret_cast<const char*>(domain));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred_received.passCreds.userName),
                      reinterpret_cast<const char*>(user));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred_received.passCreds.password),
                      reinterpret_cast<const char*>(pass));



    uint8_t domain2[] = "somewhere";
    uint8_t user2[] = "someone";
    uint8_t pass2[] = "somepass";

    ts_cred.set_credentials(domain2, sizeof(domain2),
                            user2, sizeof(user2),
                            pass2, sizeof(pass2));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred.passCreds.domainName),
                      reinterpret_cast<const char*>(domain2));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred.passCreds.userName),
                      reinterpret_cast<const char*>(user2));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred.passCreds.password),
                      reinterpret_cast<const char*>(pass2));
}

BOOST_AUTO_TEST_CASE(TestTSCredentialsSmartCard)
{

    uint8_t pin[] = "3615";
    uint8_t userHint[] = "aka";
    uint8_t domainHint[] = "grandparc";

    uint8_t cardName[] = "passepartout";
    uint8_t readerName[] = "acrobat";
    uint8_t containerName[] = "docker";
    uint8_t cspName[] = "what";
    uint32_t keySpec = 32;

    TSCredentials ts_cred(pin, sizeof(pin),
                          userHint, sizeof(userHint),
                          domainHint, sizeof(domainHint),
                          keySpec, cardName, sizeof(cardName),
                          readerName, sizeof(readerName),
                          containerName, sizeof(containerName),
                          cspName, sizeof(cspName));

    StaticOutStream<65536> s;

    int emited = ts_cred.emit(s);
    BOOST_CHECK_EQUAL(s.get_offset(), *(s.get_data() + 1) + 2);
    BOOST_CHECK_EQUAL(s.get_offset(), emited);

    TSCredentials ts_cred_received;

    InStream in_s(s.get_data(), s.get_offset());
    ts_cred_received.recv(in_s);

    BOOST_CHECK_EQUAL(ts_cred_received.credType, 2);
    BOOST_CHECK_EQUAL(ts_cred_received.smartcardCreds.pin_length,
                      sizeof(pin));
    BOOST_CHECK_EQUAL(ts_cred_received.smartcardCreds.userHint_length,
                      sizeof(userHint));
    BOOST_CHECK_EQUAL(ts_cred_received.smartcardCreds.domainHint_length,
                      sizeof(domainHint));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred_received.smartcardCreds.pin),
                      reinterpret_cast<const char*>(pin));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred_received.smartcardCreds.userHint),
                      reinterpret_cast<const char*>(userHint));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred_received.smartcardCreds.domainHint),
                      reinterpret_cast<const char*>(domainHint));
    BOOST_CHECK_EQUAL(ts_cred_received.smartcardCreds.cspData.keySpec,
                      keySpec);
    BOOST_CHECK_EQUAL(ts_cred_received.smartcardCreds.cspData.cardName_length,
                      sizeof(cardName));
    BOOST_CHECK_EQUAL(ts_cred_received.smartcardCreds.cspData.readerName_length,
                      sizeof(readerName));
    BOOST_CHECK_EQUAL(ts_cred_received.smartcardCreds.cspData.containerName_length,
                      sizeof(containerName));
    BOOST_CHECK_EQUAL(ts_cred_received.smartcardCreds.cspData.cspName_length,
                      sizeof(cspName));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred_received.smartcardCreds.cspData.cardName),
                      reinterpret_cast<const char*>(cardName));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred_received.smartcardCreds.cspData.readerName),
                      reinterpret_cast<const char*>(readerName));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred_received.smartcardCreds.cspData.containerName),
                      reinterpret_cast<const char*>(containerName));
    BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(ts_cred_received.smartcardCreds.cspData.cspName),
                      reinterpret_cast<const char*>(cspName));

}
