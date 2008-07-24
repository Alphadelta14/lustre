/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * GPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 for more details (a copy is included
 * in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; If not, see [sun.com URL with a
 * copy of GPLv2].
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *
 * GPL HEADER END
 */
/*
 * Copyright  2008 Sun Microsystems, Inc. All rights reserved
 * Use is subject to license terms.
 */
/*
 * This file is part of Lustre, http://www.lustre.org/
 * Lustre is a trademark of Sun Microsystems, Inc.
 */

#ifndef __LNET_LINUX_LIB_LNET_H__
#define __LNET_LINUX_LIB_LNET_H__

#ifndef __LNET_LIB_LNET_H__
#error Do not #include this file directly. #include <lnet/lib-lnet.h> instead
#endif

#ifdef __KERNEL__
# include <asm/page.h>
# include <linux/string.h>
# include <asm/io.h>
# include <libcfs/kp30.h>

static inline __u64
lnet_page2phys (struct page *p)
{
        /* compiler optimizer will elide unused branches */

        switch (sizeof(typeof(page_to_phys(p)))) {
        case 4:
                /* page_to_phys returns a 32 bit physical address.  This must
                 * be a 32 bit machine with <= 4G memory and we must ensure we
                 * don't sign extend when converting to 64 bits. */
                return (unsigned long)page_to_phys(p);

        case 8:
                /* page_to_phys returns a 64 bit physical address :) */
                return page_to_phys(p);
                
        default:
                LBUG();
                return 0;
        }
}

#else  /* __KERNEL__ */
# include <libcfs/list.h>
# include <string.h>
# ifdef HAVE_LIBPTHREAD
#  include <pthread.h>
# endif
#endif

#define LNET_ROUTER

#endif /* __LNET_LINUX_LIB_LNET_H__ */
