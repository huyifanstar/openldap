/* delete.c - ldbm backend delete routine */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "back-ldbm.h"
#include "proto-back-ldbm.h"

extern Attribute	*attr_find();

int
ldbm_back_delete(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    char	*dn
)
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	char		*matched = NULL;
	Entry		*e;

	if ( (e = dn2entry_w( be, dn, &matched )) == NULL ) {
		send_ldap_result( conn, op, LDAP_NO_SUCH_OBJECT, matched, "" );
		if ( matched != NULL ) {
			free( matched );
		}
		return( -1 );
	}

        /* check for deleted */

	if ( has_children( be, e ) ) {
		send_ldap_result( conn, op, LDAP_NOT_ALLOWED_ON_NONLEAF, "",
		    "" );
                goto error_return;
	}

	if ( ! access_allowed( be, conn, op, e, "entry", NULL, op->o_dn,
	    ACL_WRITE ) ) {
		send_ldap_result( conn, op, LDAP_INSUFFICIENT_ACCESS, "", "" );
                goto error_return;
	}

	/* XXX delete from parent's id2children entry XXX */

	/* delete from dn2id mapping */
	if ( dn2id_delete( be, e->e_dn ) != 0 ) {
		send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR, "", "" );
                goto error_return;
	}

	/* delete from disk and cache */
	if ( id2entry_delete( be, e ) != 0 ) {
		send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR, "", "" );
                goto error_return;
	}
	cache_return_entry_w( &li->li_cache, e );

	send_ldap_result( conn, op, LDAP_SUCCESS, "", "" );

	return( 0 );

error_return:;

        cache_return_entry_w( &li->li_cache, e );
        return( -1 );

}
