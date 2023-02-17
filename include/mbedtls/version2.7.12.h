/**
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: GPL-2.0
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */
/*
 * This set of compile-time defines and run-time variables can be used to
 * determine the version number of the mbed TLS library used.
 */
#ifndef __BUILD_MBEDTLS_VERSION_H__
#define __BUILD_MBEDTLS_VERSION_H__

/**
 * The version number x.y.z is split into three parts.
 * Major, Minor, Patchlevel
 */
#define MBEDTLS_VERSION_MAJOR  2
#define MBEDTLS_VERSION_MINOR  7
#define MBEDTLS_VERSION_PATCH  12

/**
 * The single version number has the following structure:
 *    MMNNPP00
 *    Major version | Minor version | Patch version
 */
#define MBEDTLS_VERSION_NUMBER         0x02070C00
#define MBEDTLS_VERSION_STRING         "2.7.12"
#define MBEDTLS_VERSION_STRING_FULL    "mbed TLS 2.7.12"

#endif /* __BUILD_MBEDTLS_VERSION_H__ */
