/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

/* Copyright (c) 1995, 1997 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1994 Adam Glass, Gordon Ross
 * All rights reserved.
 *
 * The NEXTSTEP Software License Agreement specifies the terms
 * and conditions for redistribution.
 *
 *  History:
 *
 *  14-March-97	Dieter Siegmund (dieter@next.com)
 *	Use BOOTP instead of RARP to get the IP address at boot time
 *
 *  23-May-97  Umesh Vaishampayan  (umeshv@apple.com)
 *	Added the ability to mount "/private" separately.
 *
 *  30-May-97	Dieter Siegmund	(dieter@next.com)
 *	- Clear out the ireq structure before using it to prevent
 *	  our sending using a bogus source IP address, we should use
 *	  an IP address of all zeroes
 *	- Right after BOOTP, get the correct netmask using AUTONETMASK
 *  18-Jul-97	Dieter Siegmund	(dieter@apple.com)
 *	- we can't restrict the netmask until we have a default route,
 *	  removed AUTONETMASK call (ifdef'd out)
 *  5-Aug-97	Dieter Siegmund (dieter@apple.com)
 *	- use the default route from the bpwhoami call, enabled autonetmask
 *	  again
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/reboot.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsdiskless.h>
#include <nfs/krpc.h>

#include "ether.h"

#if NETHER == 0

int nfs_boot_init(nd, procp)
	struct nfs_diskless *nd;
	struct proc *procp;
{
	panic("nfs_boot_init: no ether");
}

#else /* NETHER */

/*
 * Support for NFS diskless booting, specifically getting information
 * about where to boot from, what pathnames, etc.
 *
 * This implememtation uses RARP and the bootparam RPC.
 * We are forced to implement RPC anyway (to get file handles)
 * so we might as well take advantage of it for bootparam too.
 *
 * The diskless boot sequence goes as follows:
 * (1) Use RARP to get our interface address
 * (2) Use RPC/bootparam/whoami to get our hostname,
 *     our IP address, and the server's IP address.
 * (3) Use RPC/bootparam/getfile to get the root path
 * (4) Use RPC/mountd to get the root file handle
 * (5) Use RPC/bootparam/getfile to get the swap path
 * (6) Use RPC/mountd to get the swap file handle
 *
 * (This happens to be the way Sun does it too.)
 */

/* bootparam RPC */
static int bp_whoami __P((struct sockaddr_in *bpsin,
	struct in_addr *my_ip, struct in_addr *gw_ip));
static int bp_getfile __P((struct sockaddr_in *bpsin, char *key,
	struct sockaddr_in *mdsin, char *servname, char *path));

/* mountd RPC */
static int md_mount __P((struct sockaddr_in *mdsin, char *path,
	u_char *fh));

/* other helpers */
static void get_path_and_handle __P((struct sockaddr_in *bpsin,
	char *key, struct nfs_dlmount *ndmntp));

char	*nfsbootdevname;

/*
 * Called with an empty nfs_diskless struct to be filled in.
 */
int
nfs_boot_init(nd, procp)
	struct nfs_diskless *nd;
	struct proc *procp;
{
	struct ifreq ireq;
	struct in_addr my_ip, gw_ip;
	struct sockaddr_in bp_sin;
	struct sockaddr_in *sin;
	struct ifnet *ifp;
	struct socket *so;
	int error;

	/* clear out the request structure */
	bzero(&ireq, sizeof(ireq));

	/*
	 * Find an interface, rarp for its ip address, stuff it, the
	 * implied broadcast addr, and netmask into a nfs_diskless struct.
	 *
	 * This was moved here from nfs_vfsops.c because this procedure
	 * would be quite different if someone decides to write (i.e.) a
	 * BOOTP version of this file (might not use RARP, etc.)
	 */

	/*
	 * Find a network interface.
	 */
	if (nfsbootdevname)
		ifp = ifunit(nfsbootdevname);
	else
		for (ifp = ifnet; ifp; ifp = ifp->if_next)
			if ((ifp->if_flags &
			     (IFF_LOOPBACK|IFF_POINTOPOINT)) == 0)
				break;
	if (ifp == NULL)
		panic("nfs_boot: no suitable interface");
	sprintf(ireq.ifr_name, "%s%d", ifp->if_name, ifp->if_unit);
	printf("nfs_boot: using network interface '%s'\n", ireq.ifr_name);

	/*
	 * Bring up the interface.
	 */
	if ((error = socreate(AF_INET, &so, SOCK_DGRAM, 0)) != 0)
		panic("nfs_boot: socreate, error=%d", error);
	ireq.ifr_flags = IFF_UP;
	error = ifioctl(so, SIOCSIFFLAGS, (caddr_t)&ireq, procp);
	if (error)
		panic("nfs_boot: SIFFLAGS, error=%d", error);

#if NeXT
	/*
	 * Do BOOTP to get the interface address
	 */
	{
	    int	message	= 0;
	    
	    ireq.ifr_addr.sa_family = AF_INET;
	    while ((error = ifioctl(so, SIOCAUTOADDR, (caddr_t)&ireq, procp))) {
		if (error == ETIMEDOUT) {
		    if (!message) {
			printf("nfs_boot: BOOTP timed out, still trying...\n");
			message++;
		    }
		} 
		else {
		    printf("nfs_boot: autoaddr failed, error = %d\n", error);
		    panic("nfs_boot: SIOCAUTOADDR failed");
		}
	    }

	    sin = (struct sockaddr_in *)&(ireq.ifr_addr);
	    my_ip = sin->sin_addr;
	}
	printf("nfs_boot: client_addr=0x%x\n", ntohl(my_ip.s_addr));

	/*
	 * get the netmask
	 * XXX - if BOOTP returns a netmask, we should use that instead
 	 *       of icmp (DWS)
	 */
	if ((error = ifioctl(so, SIOCAUTONETMASK, (caddr_t)&ireq, procp))) {
	    printf("nfs_boot: autonetmask failed, error = %d\n", error);
	}

#else NeXT
	/*
	 * Do RARP for the interface address.
	 */
	if ((error = revarpwhoami(&my_ip, ifp)) != 0)
		panic("revarp failed, error=%d", error);
	printf("nfs_boot: client_addr=0x%x\n", ntohl(my_ip.s_addr));

	/*
	 * Do enough of ifconfig(8) so that the chosen interface
	 * can talk to the servers.  (just set the address)
	 */
	sin = (struct sockaddr_in *)&ireq.ifr_addr;
	bzero((caddr_t)sin, sizeof(*sin));
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = my_ip.s_addr;
	error = ifioctl(so, SIOCSIFADDR, (caddr_t)&ireq, procp);
	if (error)
		panic("nfs_boot: set if addr, error=%d", error);
#endif NeXT

	soclose(so);

	/*
	 * Get client name and gateway address.
	 * RPC: bootparam/whoami
	 * Use the old broadcast address for the WHOAMI
	 * call because we do not yet know our netmask.
	 * The server address returned by the WHOAMI call
	 * is used for all subsequent booptaram RPCs.
	 */
	bzero((caddr_t)&bp_sin, sizeof(bp_sin));
	bp_sin.sin_len = sizeof(bp_sin);
	bp_sin.sin_family = AF_INET;
	bp_sin.sin_addr.s_addr = INADDR_BROADCAST;
	hostnamelen = MAXHOSTNAMELEN;

	/* this returns gateway IP address */
	error = bp_whoami(&bp_sin, &my_ip, &gw_ip);
	if (error) {
	    printf("nfs_boot: bootparam whoami, error=%d", error);
	    panic("nfs_boot: bootparam whoami\n");
	}
	printf("nfs_boot: server_addr=0x%x\n",
		   ntohl(bp_sin.sin_addr.s_addr));
	printf("nfs_boot: hostname=%s\n", hostname);
#define NFS_BOOT_GATEWAY 	1
#ifdef	NFS_BOOT_GATEWAY
	/*
	 * XXX - This code is conditionally compiled only because
	 * many bootparam servers (in particular, SunOS 4.1.3)
	 * always set the gateway address to their own address.
	 * The bootparam server is not necessarily the gateway.
	 * We could just believe the server, and at worst you would
	 * need to delete the incorrect default route before adding
	 * the correct one, but for simplicity, ignore the gateway.
	 * If your server is OK, you can turn on this option.
	 *
	 * If the gateway address is set, add a default route.
	 * (The mountd RPCs may go across a gateway.)
	 */
	if (gw_ip.s_addr) {
		struct sockaddr dst, gw, mask;
		/* Destination: (default) */
		bzero((caddr_t)&dst, sizeof(dst));
		dst.sa_len = sizeof(dst);
		dst.sa_family = AF_INET;
		/* Gateway: */
		bzero((caddr_t)&gw, sizeof(gw));
		sin = (struct sockaddr_in *)&gw;
		sin->sin_len = sizeof(gw);
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = gw_ip.s_addr;
		/* Mask: (zero length) */
		bzero(&mask, sizeof(mask));

		printf("nfs_boot: gateway=0x%x\n", ntohl(gw_ip.s_addr));
		/* add, dest, gw, mask, flags, 0 */
		error = rtrequest(RTM_ADD, &dst, (struct sockaddr *)&gw,
		    &mask, (RTF_UP | RTF_GATEWAY | RTF_STATIC), NULL);
		if (error)
			printf("nfs_boot: add route, error=%d\n", error);
	}
#endif

	get_path_and_handle(&bp_sin, "root", &nd->nd_root);
#if !defined(NO_MOUNT_PRIVATE)
	get_path_and_handle(&bp_sin, "private", &nd->nd_private);
#endif /* NO_MOUNT_PRIVATE */

	return (0);
}

static void
get_path_and_handle(bpsin, key, ndmntp)
	struct sockaddr_in *bpsin;	/* bootparam server */
	char *key;			/* root or swap */
	struct nfs_dlmount *ndmntp;	/* output */
{
	char pathname[MAXPATHLEN];
	char *sp, *dp, *endp;
	int error;

	/*
	 * Get server:pathname for "key" (root or swap)
	 * using RPC to bootparam/getfile
	 */
	error = bp_getfile(bpsin, key, &ndmntp->ndm_saddr,
	    ndmntp->ndm_host, pathname);
	if (error) {
		printf("nfs_boot: bootparam get %s: %d\n", key, error);
		panic("nfs_boot: bootparam get %s: %d", key, error);
	}
	/*
	 * Get file handle for "key" (root or swap)
	 * using RPC to mountd/mount
	 */
	error = md_mount(&ndmntp->ndm_saddr, pathname, ndmntp->ndm_fh);
	if (error)
		panic("nfs_boot: mountd %s, error=%d", key, error);

	/* Construct remote path (for getmntinfo(3)) */
	dp = ndmntp->ndm_host;
	endp = dp + MNAMELEN - 1;
	dp += strlen(dp);
	*dp++ = ':';
	for (sp = pathname; *sp && dp < endp;)
		*dp++ = *sp++;
	*dp = '\0';

}


/*
 * Get an mbuf with the given length, and
 * initialize the pkthdr length field.
 */
static struct mbuf *
m_get_len(int msg_len)
{
	struct mbuf *m;
	m = m_gethdr(M_WAIT, MT_DATA);
	if (m == NULL)
		return NULL;
	if (msg_len > MHLEN) {
		if (msg_len > MCLBYTES)
			panic("nfs_boot: msg_len > MCLBYTES");
		MCLGET(m, M_WAIT);
		if (m == NULL)
			return NULL;
	}
	m->m_len = msg_len;
	m->m_pkthdr.len = m->m_len;
	return (m);
}


/*
 * String representation for RPC.
 */
struct rpc_string {
	u_long len;		/* length without null or padding */
	u_char data[4];	/* data (longer, of course) */
    /* data is padded to a long-word boundary */
};
/* Compute space used given string length. */
#define	RPC_STR_SIZE(slen) (4 + ((slen + 3) & ~3))

/*
 * Inet address in RPC messages
 * (Note, really four longs, NOT chars.  Blech.)
 */
struct bp_inaddr {
	u_long  atype;
	long	addr[4];
};


/*
 * RPC: bootparam/whoami
 * Given client IP address, get:
 *	client name	(hostname)
 *	domain name (domainname)
 *	gateway address
 *
 * The hostname and domainname are set here for convenience.
 *
 * Note - bpsin is initialized to the broadcast address,
 * and will be replaced with the bootparam server address
 * after this call is complete.  Have to use PMAP_PROC_CALL
 * to make sure we get responses only from a servers that
 * know about us (don't want to broadcast a getport call).
 */
static int
bp_whoami(bpsin, my_ip, gw_ip)
	struct sockaddr_in *bpsin;
	struct in_addr *my_ip;
	struct in_addr *gw_ip;
{
	/* RPC structures for PMAPPROC_CALLIT */
	struct whoami_call {
		u_long call_prog;
		u_long call_vers;
		u_long call_proc;
		u_long call_arglen;
		struct bp_inaddr call_ia;
	} *call;

	struct rpc_string *str;
	struct bp_inaddr *bia;
	struct mbuf *m, *from;
	struct sockaddr_in *sin;
	int error, msg_len;
	int cn_len, dn_len;
	u_char *p;
	long *lp;

	/*
	 * Get message buffer of sufficient size.
	 */
	msg_len = sizeof(*call);
	m = m_get_len(msg_len);
	if (m == NULL)
		return ENOBUFS;

	/*
	 * Build request message for PMAPPROC_CALLIT.
	 */
	call = mtod(m, struct whoami_call *);
	call->call_prog = htonl(BOOTPARAM_PROG);
	call->call_vers = htonl(BOOTPARAM_VERS);
	call->call_proc = htonl(BOOTPARAM_WHOAMI);
	call->call_arglen = htonl(sizeof(struct bp_inaddr));

	/* client IP address */
	call->call_ia.atype = htonl(1);
	p = (u_char*)my_ip;
	lp = call->call_ia.addr;
	*lp++ = htonl(*p);	p++;
	*lp++ = htonl(*p);	p++;
	*lp++ = htonl(*p);	p++;
	*lp++ = htonl(*p);	p++;

	/* RPC: portmap/callit */
	bpsin->sin_port = htons(PMAPPORT);
	from = NULL;
	error = krpc_call(bpsin, PMAPPROG, PMAPVERS,
			PMAPPROC_CALLIT, &m, &from);
	if (error)
		return error;

	/*
	 * Parse result message.
	 */
	msg_len = m->m_len;
	lp = mtod(m, long *);

	/* bootparam server port (also grab from address). */
	if (msg_len < sizeof(*lp))
		goto bad;
	msg_len -= sizeof(*lp);
	bpsin->sin_port = htons((short)ntohl(*lp++));
	sin = mtod(from, struct sockaddr_in *);
	bpsin->sin_addr.s_addr = sin->sin_addr.s_addr;

	/* length of encapsulated results */
	if (msg_len < (ntohl(*lp) + sizeof(*lp)))
		goto bad;
	msg_len = ntohl(*lp++);
	p = (char*)lp;

	/* client name */
	if (msg_len < sizeof(*str))
		goto bad;
	str = (struct rpc_string *)p;
	cn_len = ntohl(str->len);
	if (msg_len < cn_len)
		goto bad;
	if (cn_len >= MAXHOSTNAMELEN)
		goto bad;
	bcopy(str->data, hostname, cn_len);
	hostname[cn_len] = '\0';
	hostnamelen = cn_len;
	p += RPC_STR_SIZE(cn_len);
	msg_len -= RPC_STR_SIZE(cn_len);

	/* domain name */
	if (msg_len < sizeof(*str))
		goto bad;
	str = (struct rpc_string *)p;
	dn_len = ntohl(str->len);
	if (msg_len < dn_len)
		goto bad;
	if (dn_len >= MAXHOSTNAMELEN)
		goto bad;
	bcopy(str->data, domainname, dn_len);
	domainname[dn_len] = '\0';
	domainnamelen = dn_len;
	p += RPC_STR_SIZE(dn_len);
	msg_len -= RPC_STR_SIZE(dn_len);

	/* gateway address */
	if (msg_len < sizeof(*bia))
		goto bad;
	bia = (struct bp_inaddr *)p;
	if (bia->atype != htonl(1))
		goto bad;
	p = (u_char*)gw_ip;
	*p++ = ntohl(bia->addr[0]);
	*p++ = ntohl(bia->addr[1]);
	*p++ = ntohl(bia->addr[2]);
	*p++ = ntohl(bia->addr[3]);
	goto out;

bad:
	printf("nfs_boot: bootparam_whoami: bad reply\n");
	error = EBADRPC;

out:
	if (from)
		m_freem(from);
	m_freem(m);
	return(error);
}


/*
 * RPC: bootparam/getfile
 * Given client name and file "key", get:
 *	server name
 *	server IP address
 *	server pathname
 */
static int
bp_getfile(bpsin, key, md_sin, serv_name, pathname)
	struct sockaddr_in *bpsin;
	char *key;
	struct sockaddr_in *md_sin;
	char *serv_name;
	char *pathname;
{
	struct rpc_string *str;
	struct mbuf *m;
	struct bp_inaddr *bia;
	struct sockaddr_in *sin;
	u_char *p, *q;
	int error, msg_len;
	int cn_len, key_len, sn_len, path_len;

	/*
	 * Get message buffer of sufficient size.
	 */
	cn_len = hostnamelen;
	key_len = strlen(key);
	msg_len = 0;
	msg_len += RPC_STR_SIZE(cn_len);
	msg_len += RPC_STR_SIZE(key_len);
	m = m_get_len(msg_len);
	if (m == NULL)
		return ENOBUFS;

	/*
	 * Build request message.
	 */
	p = mtod(m, u_char *);
	bzero(p, msg_len);
	/* client name (hostname) */
	str = (struct rpc_string *)p;
	str->len = htonl(cn_len);
	bcopy(hostname, str->data, cn_len);
	p += RPC_STR_SIZE(cn_len);
	/* key name (root or swap) */
	str = (struct rpc_string *)p;
	str->len = htonl(key_len);
	bcopy(key, str->data, key_len);

	/* RPC: bootparam/getfile */
	error = krpc_call(bpsin, BOOTPARAM_PROG, BOOTPARAM_VERS,
			BOOTPARAM_GETFILE, &m, NULL);
	if (error)
		return error;

	/*
	 * Parse result message.
	 */
	p = mtod(m, u_char *);
	msg_len = m->m_len;

	/* server name */
	if (msg_len < sizeof(*str))
		goto bad;
	str = (struct rpc_string *)p;
	sn_len = ntohl(str->len);
	if (msg_len < sn_len)
		goto bad;
	if (sn_len >= MNAMELEN)
		goto bad;
	bcopy(str->data, serv_name, sn_len);
	serv_name[sn_len] = '\0';
	p += RPC_STR_SIZE(sn_len);
	msg_len -= RPC_STR_SIZE(sn_len);

	/* server IP address (mountd) */
	if (msg_len < sizeof(*bia))
		goto bad;
	bia = (struct bp_inaddr *)p;
	if (bia->atype != htonl(1))
		goto bad;
	sin = md_sin;
	bzero((caddr_t)sin, sizeof(*sin));
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	q = (u_char*) &sin->sin_addr;
	*q++ = ntohl(bia->addr[0]);
	*q++ = ntohl(bia->addr[1]);
	*q++ = ntohl(bia->addr[2]);
	*q++ = ntohl(bia->addr[3]);
	p += sizeof(*bia);
	msg_len -= sizeof(*bia);

	/* server pathname */
	if (msg_len < sizeof(*str))
		goto bad;
	str = (struct rpc_string *)p;
	path_len = ntohl(str->len);
	if (msg_len < path_len)
		goto bad;
	if (path_len >= MAXPATHLEN)
		goto bad;
	bcopy(str->data, pathname, path_len);
	pathname[path_len] = '\0';
	goto out;

bad:
	printf("nfs_boot: bootparam_getfile: bad reply\n");
	error = EBADRPC;

out:
	m_freem(m);
	return(0);
}


/*
 * RPC: mountd/mount
 * Given a server pathname, get an NFS file handle.
 * Also, sets sin->sin_port to the NFS service port.
 */
static int
md_mount(mdsin, path, fhp)
	struct sockaddr_in *mdsin;		/* mountd server address */
	char *path;
	u_char *fhp;
{
	/* The RPC structures */
	struct rpc_string *str;
	struct rdata {
		u_long	errno;
		u_char	fh[NFSX_V2FH];
	} *rdata;
	struct mbuf *m;
	int error, mlen, slen;

	/* Get port number for MOUNTD. */
	error = krpc_portmap(mdsin, RPCPROG_MNT, RPCMNT_VER1,
						 &mdsin->sin_port);
	if (error) return error;

	slen = strlen(path);
	mlen = RPC_STR_SIZE(slen);

	m = m_get_len(mlen);
	if (m == NULL)
		return ENOBUFS;
	str = mtod(m, struct rpc_string *);
	str->len = htonl(slen);
	bcopy(path, str->data, slen);

	/* Do RPC to mountd. */
	error = krpc_call(mdsin, RPCPROG_MNT, RPCMNT_VER1,
			RPCMNT_MOUNT, &m, NULL);
	if (error)
		return error;	/* message already freed */

	mlen = m->m_len;
	if (mlen < sizeof(*rdata))
		goto bad;
	rdata = mtod(m, struct rdata *);
	error = ntohl(rdata->errno);
	if (error)
		goto bad;
	bcopy(rdata->fh, fhp, NFSX_V2FH);

	/* Set port number for NFS use. */
	error = krpc_portmap(mdsin, NFS_PROG, NFS_VER2,
						 &mdsin->sin_port);
	goto out;

bad:
	error = EBADRPC;

out:
	m_freem(m);
	return error;
}

#endif /* NETHER */
