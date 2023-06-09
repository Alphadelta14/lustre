/*
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
 * version 2 along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA
 *
 * GPL HEADER END
 */
/*
 * Copyright (c) 2009, 2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Copyright (c) 2012, 2017, Intel Corporation.
 *
 * Code originally extracted from quota directory
 */

#include <obd.h>
#include <lustre_osc.h>

#include "osc_internal.h"

static inline struct osc_quota_info *osc_oqi_alloc(u32 id)
{
	struct osc_quota_info *oqi;

	OBD_SLAB_ALLOC_PTR(oqi, osc_quota_kmem);
	if (oqi != NULL)
		oqi->oqi_id = id;

	return oqi;
}

int osc_quota_chkdq(struct client_obd *cli, const unsigned int qid[])
{
	int type;
	ENTRY;

	for (type = 0; type < LL_MAXQUOTAS; type++) {
		struct osc_quota_info *oqi;

		oqi = cfs_hash_lookup(cli->cl_quota_hash[type], &qid[type]);
		if (oqi) {
			/* do not try to access oqi here, it could have been
			 * freed by osc_quota_setdq() */

			/* the slot is busy, the user is about to run out of
			 * quota space on this OST */
			CDEBUG(D_QUOTA, "chkdq found noquota for %s %d\n",
			       type == USRQUOTA ? "user" : "grout", qid[type]);
			RETURN(-EDQUOT);
		}
	}

	RETURN(0);
}

static inline u32 md_quota_flag(int qtype)
{
	switch (qtype) {
	case USRQUOTA:
		return OBD_MD_FLUSRQUOTA;
	case GRPQUOTA:
		return OBD_MD_FLGRPQUOTA;
	case PRJQUOTA:
		return OBD_MD_FLPRJQUOTA;
	default:
		return 0;
	}
}

static inline u32 fl_quota_flag(int qtype)
{
	switch (qtype) {
	case USRQUOTA:
		return OBD_FL_NO_USRQUOTA;
	case GRPQUOTA:
		return OBD_FL_NO_GRPQUOTA;
	case PRJQUOTA:
		return OBD_FL_NO_PRJQUOTA;
	default:
		return 0;
	}
}

int osc_quota_setdq(struct client_obd *cli, __u64 xid, const unsigned int qid[],
		    u64 valid, u32 flags)
{
	int type;
	int rc = 0;

        ENTRY;

	if ((valid & (OBD_MD_FLALLQUOTA)) == 0)
		RETURN(0);

	mutex_lock(&cli->cl_quota_mutex);
	cli->cl_root_squash = !!(flags & OBD_FL_ROOT_SQUASH);
	cli->cl_root_prjquota = !!(flags & OBD_FL_ROOT_PRJQUOTA);
	/* still mark the quots is running out for the old request, because it
	 * could be processed after the new request at OST, the side effect is
	 * the following request will be processed synchronously, but it will
	 * not break the quota enforcement. */
	if (cli->cl_quota_last_xid > xid && !(flags & OBD_FL_NO_QUOTA_ALL))
		GOTO(out_unlock, rc);

	if (cli->cl_quota_last_xid < xid)
		cli->cl_quota_last_xid = xid;

	for (type = 0; type < LL_MAXQUOTAS; type++) {
		struct osc_quota_info *oqi;

		if ((valid & md_quota_flag(type)) == 0)
			continue;

		/* lookup the ID in the per-type hash table */
		oqi = cfs_hash_lookup(cli->cl_quota_hash[type], &qid[type]);
		if ((flags & fl_quota_flag(type)) != 0) {
			/* This ID is getting close to its quota limit, let's
			 * switch to sync I/O */
			if (oqi != NULL)
				continue;

			oqi = osc_oqi_alloc(qid[type]);
			if (oqi == NULL) {
				rc = -ENOMEM;
				break;
			}

			rc = cfs_hash_add_unique(cli->cl_quota_hash[type],
						 &qid[type], &oqi->oqi_hash);
			/* race with others? */
			if (rc == -EALREADY) {
				rc = 0;
				OBD_SLAB_FREE_PTR(oqi, osc_quota_kmem);
			}

			CDEBUG(D_QUOTA, "%s: setdq to insert for %s %d (%d)\n",
			       cli_name(cli), qtype_name(type), qid[type], rc);
		} else {
			/* This ID is now off the hook, let's remove it from
			 * the hash table */
			if (oqi == NULL)
				continue;

			oqi = cfs_hash_del_key(cli->cl_quota_hash[type],
					       &qid[type]);
			if (oqi)
				OBD_SLAB_FREE_PTR(oqi, osc_quota_kmem);

			CDEBUG(D_QUOTA, "%s: setdq to remove for %s %d (%p)\n",
			       cli_name(cli), qtype_name(type), qid[type], oqi);
		}
	}

out_unlock:
	mutex_unlock(&cli->cl_quota_mutex);
	RETURN(rc);
}

/*
 * Hash operations for uid/gid <-> osc_quota_info
 */
static unsigned
oqi_hashfn(struct cfs_hash *hs, const void *key, unsigned mask)
{
	return cfs_hash_32(*((__u32 *)key), 0) & mask;
}

static int
oqi_keycmp(const void *key, struct hlist_node *hnode)
{
	struct osc_quota_info *oqi;
	u32 uid;

	LASSERT(key != NULL);
	uid = *((u32 *)key);
	oqi = hlist_entry(hnode, struct osc_quota_info, oqi_hash);

	return uid == oqi->oqi_id;
}

static void *
oqi_key(struct hlist_node *hnode)
{
	struct osc_quota_info *oqi;
	oqi = hlist_entry(hnode, struct osc_quota_info, oqi_hash);
	return &oqi->oqi_id;
}

static void *
oqi_object(struct hlist_node *hnode)
{
	return hlist_entry(hnode, struct osc_quota_info, oqi_hash);
}

static void
oqi_get(struct cfs_hash *hs, struct hlist_node *hnode)
{
}

static void
oqi_put_locked(struct cfs_hash *hs, struct hlist_node *hnode)
{
}

static void
oqi_exit(struct cfs_hash *hs, struct hlist_node *hnode)
{
	struct osc_quota_info *oqi;

	oqi = hlist_entry(hnode, struct osc_quota_info, oqi_hash);

        OBD_SLAB_FREE_PTR(oqi, osc_quota_kmem);
}

#define HASH_QUOTA_BKT_BITS 5
#define HASH_QUOTA_CUR_BITS 5
#define HASH_QUOTA_MAX_BITS 15

static struct cfs_hash_ops quota_hash_ops = {
	.hs_hash	= oqi_hashfn,
	.hs_keycmp	= oqi_keycmp,
	.hs_key		= oqi_key,
	.hs_object	= oqi_object,
	.hs_get		= oqi_get,
	.hs_put_locked	= oqi_put_locked,
	.hs_exit	= oqi_exit,
};

int osc_quota_setup(struct obd_device *obd)
{
	struct client_obd *cli = &obd->u.cli;
	int i, type;
	ENTRY;

	mutex_init(&cli->cl_quota_mutex);

	for (type = 0; type < LL_MAXQUOTAS; type++) {
		cli->cl_quota_hash[type] = cfs_hash_create("QUOTA_HASH",
							   HASH_QUOTA_CUR_BITS,
							   HASH_QUOTA_MAX_BITS,
							   HASH_QUOTA_BKT_BITS,
							   0,
							   CFS_HASH_MIN_THETA,
							   CFS_HASH_MAX_THETA,
							   &quota_hash_ops,
							   CFS_HASH_DEFAULT);
		if (cli->cl_quota_hash[type] == NULL)
			break;
	}

	if (type == LL_MAXQUOTAS)
		RETURN(0);

	for (i = 0; i < type; i++)
		cfs_hash_putref(cli->cl_quota_hash[i]);

	RETURN(-ENOMEM);
}

int osc_quota_cleanup(struct obd_device *obd)
{
	struct client_obd     *cli = &obd->u.cli;
	int type;
	ENTRY;

	for (type = 0; type < LL_MAXQUOTAS; type++)
		cfs_hash_putref(cli->cl_quota_hash[type]);

	RETURN(0);
}

int osc_quotactl(struct obd_device *unused, struct obd_export *exp,
                 struct obd_quotactl *oqctl)
{
        struct ptlrpc_request *req;
        struct obd_quotactl   *oqc;
        int                    rc;
        ENTRY;

        req = ptlrpc_request_alloc_pack(class_exp2cliimp(exp),
                                        &RQF_OST_QUOTACTL, LUSTRE_OST_VERSION,
                                        OST_QUOTACTL);
        if (req == NULL)
                RETURN(-ENOMEM);

        oqc = req_capsule_client_get(&req->rq_pill, &RMF_OBD_QUOTACTL);
        *oqc = *oqctl;

        ptlrpc_request_set_replen(req);
        ptlrpc_at_set_req_timeout(req);
        req->rq_no_resend = 1;

        rc = ptlrpc_queue_wait(req);
        if (rc)
                CERROR("ptlrpc_queue_wait failed, rc: %d\n", rc);

        if (req->rq_repmsg &&
            (oqc = req_capsule_server_get(&req->rq_pill, &RMF_OBD_QUOTACTL))) {
                *oqctl = *oqc;
        } else if (!rc) {
                CERROR ("Can't unpack obd_quotactl\n");
                rc = -EPROTO;
        }
        ptlrpc_req_finished(req);

        RETURN(rc);
}
