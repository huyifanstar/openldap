/* modrdn.c - ldap backend modrdn function */
/* $OpenLDAP$ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/* This is an altered version */
/*
 * Copyright 1999, Howard Chu, All rights reserved. <hyc@highlandsun.com>
 * 
 * Permission is granted to anyone to use this software for any purpose
 * on any computer system, and to alter it and redistribute it, subject
 * to the following restrictions:
 * 
 * 1. The author is not responsible for the consequences of use of this
 *    software, no matter how awful, even if they arise from flaws in it.
 * 
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits should appear in the documentation.
 * 
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits should appear in the documentation.
 * 
 * 4. This notice may not be removed or altered.
 *
 *
 *
 * Copyright 2000, Pierangelo Masarati, All rights reserved. <ando@sys-net.it>
 * 
 * This software is being modified by Pierangelo Masarati.
 * The previously reported conditions apply to the modified code as well.
 * Changes in the original code are highlighted where required.
 * Credits for the original code go to the author, Howard Chu.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "back-ldap.h"

int
ldap_back_modrdn(
    Operation	*op,
    SlapReply	*rs )
{
	struct ldapinfo	*li = (struct ldapinfo *) op->o_bd->be_private;
	struct ldapconn *lc;
	ber_int_t msgid;
	dncookie dc;

	struct berval mdn = { 0, NULL }, mnewSuperior = { 0, NULL };

	lc = ldap_back_getconn( op, rs );
	if ( !lc || !ldap_back_dobind(lc, op, rs) ) {
		return( -1 );
	}

	dc.rwmap = &li->rwmap;
#ifdef ENABLE_REWRITE
	dc.conn = op->o_conn;
	dc.rs = rs;
#else
	dc.tofrom = 1;
	dc.normalized = 0;
#endif
	if (op->orr_newSup) {
		int version = LDAP_VERSION3;
		ldap_set_option( lc->ld, LDAP_OPT_PROTOCOL_VERSION, &version);
		
		/*
		 * Rewrite the new superior, if defined and required
	 	 */
#ifdef ENABLE_REWRITE
		dc.ctx = "newSuperiorDn";
#endif
		if ( ldap_back_dn_massage( &dc, op->orr_newSup,
			&mnewSuperior ) ) {
			send_ldap_result( op, rs );
			return -1;
		}
	}

	/*
	 * Rewrite the modrdn dn, if required
	 */
#ifdef ENABLE_REWRITE
	dc.ctx = "modrDn";
#endif
	if ( ldap_back_dn_massage( &dc, &op->o_req_dn, &mdn ) ) {
		send_ldap_result( op, rs );
		return -1;
	}

	rs->sr_err = ldap_rename( lc->ld, mdn.bv_val,
			op->orr_newrdn.bv_val, mnewSuperior.bv_val,
			op->orr_deleteoldrdn, op->o_ctrls,
			NULL, &msgid );

	if ( mdn.bv_val != op->o_req_dn.bv_val ) {
		free( mdn.bv_val );
	}
	if ( mnewSuperior.bv_val != NULL
		&& mnewSuperior.bv_val != op->oq_modrdn.rs_newSup->bv_val ) {
		free( mnewSuperior.bv_val );
	}
	
	return( ldap_back_op_result( lc, op, rs, msgid, 1 ) );
}

