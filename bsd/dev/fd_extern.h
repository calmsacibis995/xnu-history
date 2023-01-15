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

/*	@(#)fd_extern.h	 2.0	01/25/90	(c) 1990 NeXT	
 *
 * fd_vars.h -- Externally used data structures and constants for Floppy 
 *		Disk driver
 *
 * KERNEL VERSION
 *
 * HISTORY
 * 13-May-94	Dean Reece
 *	Added define and struct for FDIOCGCAPLIST to return possible
 *	format densities.  Enables workspace to build a menu.
 * 20-Mar-91	Doug Mitchell
 *	Made #importing  pmap.h and vm_map.h dependent on #ifdef KERNEL.
 * 25-Jan-90	Doug Mitchell at NeXT
 *	Created.
 *
 */
 
#ifndef	_FDEXTERN_
#define _FDEXTERN_

#import <sys/types.h>
#import <sys/ioctl.h>
#import <mach/boolean.h>
#import <mach/vm_param.h>
#define FD_CMDSIZE	0x10		/* max size of command passed to 
					 * controller */
#define FD_STATSIZE	0x10		/* max size of status array returned
					 * from controller */
#define	FD_MAX_CAP_LIST	8
					 
typedef	int fd_return_t;		/* see FDR_xxx, below */

struct fd_drive_stat {
	u_char		media_id:2,	/* media type currently inserted. See
					 * FD_MID_xxx, below. */
	 		motor_on:1,	/* state of motor at I/O complete 
					 * 1 = on; 0 = off */
			write_prot:1,	/* write protect 1 = Write Protected */
			drive_present:1,
			rsvd1:3;	/* reserved */
};

/*
 * Format Capacity List struct.  Returned by FDIOCGCAPLIST ioctl as a list
 * of possible format capacities.  Entries are possible capacities
 * in 1k (1024) units.  List is null terminated, so the useful size is
 * (FD_MAX_CAP_LIST - 1).
 */
struct fd_cap_list {
	int		capacity[FD_MAX_CAP_LIST];
};

/*
 * I/O request struct. Used in DKIOCREQ ioctl to specify one command sequence
 * to be executed.
 */
struct fd_ioreq {
	/* 
	 * inputs to driver:
	 */
	u_char		density;	/* see FD_DENS_xxx, below */
	int		timeout;	/* I/O timeout in milliseconds. Used
					 * in FDCMD_CMD_XFR commands only. */
	int		command;	/* see FDCMD_xxx, below */
	u_char		cmd_blk[FD_CMDSIZE];
					/* actual command bytes */
	u_int		num_cmd_bytes;	/* expected # of bytes in cmd_blk[] to
					 * transfer */
	caddr_t		addrs;		/* source/dest of data */
	u_int		byte_count;	/* max # of data bytes to move */
	u_char		stat_blk[FD_STATSIZE];
					/* status returned from controller */
	u_int		num_stat_bytes;	/* number of status bytes to transfer
					 * to stat_blk[] */
	int 		flags;		/* see FD_IOF_xxx, below */
	
	/*
	 * outputs from driver:
	 */
	fd_return_t	status;		/* FDR_SUCCESS, etc. */
	u_int		cmd_bytes_xfr;	/* # of command bytes actually moved */
	u_int		bytes_xfr;	/* # of data bytes actually moved */
	u_int		stat_bytes_xfr;	/* # if status bytes moved to 
					 * stat_blk[] */
	struct fd_drive_stat drive_stat; /* media ID, etc. */

	/*
	 * used internally by driver
	 */
	struct pmap	*pmap;		/* DMA is mapped by this */
	struct vm_map	*map;		/* map of requestor's task */
	u_char		unit;		/* drive # (relative to controller) */
};

typedef struct fd_ioreq fdIoReq_t;	
typedef struct fd_ioreq *fd_ioreq_t;

/*
 * fd_ioreq.command values 
 */
#define FDCMD_BAD		0x00	/* not used */
#define FDCMD_CMD_XFR		0x01	/* transfer command in fd_ioreq.cmd_blk
					 */
#define FDCMD_EJECT		0x02	/* eject disk */
#define FDCMD_MOTOR_ON		0x03	/* motor on */
#define FDCMD_MOTOR_OFF		0x04	/* motor off */
#define FDCMD_GET_STATUS	0x05	/* Get status (media ID, motor state,
					 * write protect) */

/*
 * fd_ioreq.density values. Some routines rely on these being in
 * cardinal order...
 */
#define FD_DENS_NONE	0		/* unformatted */
#define FD_DENS_1	1		/* 1 MByte/disk unformatted */
#define FD_DENS_2	2		/* 2 MByte/disk unformatted */
#define FD_DENS_4	3		/* 4 MByte/disk unformatted */

#if 	i386
#define FD_DENS_DEFAULT	FD_DENS_2
#else
#define FD_DENS_DEFAULT FD_DENS_4
#endif

/*
 * fd_ioreq.flags values
 */
#define FD_IOF_DMA_DIR		0x00000002
#define FD_IOF_DMA_RD		0x00000002	/* DMA direction = device to
						 * host */
#define FD_IOF_DMA_WR		0x00000000	/* DMA direction = host to
						 * device */
						 
/*
 * fdr_return_t values
 */
#define FDR_SUCCESS		0	/* OK */
#define FDR_TIMEOUT		1	/* fd_ioreq.timeout exceeded */
#define FDR_MEMALLOC		2	/* couldn't allocate memory */
#define FDR_MEMFAIL		3	/* memory transfer error */
#define FDR_REJECT		4	/* bad field in fd_ioreq */
#define FDR_BADDRV		5	/* drive not present */
#define FDR_DATACRC		6	/* media error - data CRC */
#define FDR_HDRCRC		7	/* media error - header CRC */
#define FDR_MEDIA		8	/* misc. media error */
#define FDR_SEEK 		9	/* seek error */
#define FDR_BADPHASE		10	/* controller changed phase 
					 * unexpectedly */
#define FDR_DRIVE_FAIL		11	/* Basic Drive Failure */
#define FDR_NOHDR		12	/* Header Not Found */
#define FDR_WRTPROT		13	/* Disk Write Protected */
#define FDR_NO_ADDRS_MK		14	/* Missing Address Mark */
#define FDR_CNTRL_MK		15	/* Missing Control Mark */
#define FDR_NO_DATA_MK		16	/* Missing Data Mark */
#define FDR_CNTRL_REJECT	17	/* controller rejected command */
#define FDR_CNTRLR		18	/* Controller Handshake Error */
#define FDR_DMAOURUN		19	/* DMA Over/underrun */
#define FDR_VOLUNAVAIL		20	/* Requested Volume not available */
#define FDR_ALIGN		21	/* DMA alignment error */
#define FDR_DMA			22	/* DMA error */
#define FDR_SPURIOUS		23	/* spurious interrupt */

/*
 * fd_drive_stat.media_id values
 */
#define FD_MID_NONE		0	/* no disk inserted */
#define FD_MID_1MB		3	/* 1 MByte unformatted */
#define FD_MID_2MB		2	/* 2 MByte unformatted */
#define FD_MID_4MB		1	/* 4 MBytes unformatted */

#if i386 || hppa
#define FD_MID_DEFAULT	FD_MID_2MB
#ifdef KERNEL				   /* Maximum Number of bytes of data
					  	that can be transfered via DMA 
					   	using FDIOCREQ ioctl command
					   */

#define FD_MAX_DMA_SIZE		PAGE_SIZE 
#else
#define FD_MAX_DMA_SIZE		vm_page_size 
#endif /* KERNEL */
#endif /* i386 || hppa */

/*
 * ioctl's specific to floppy disk
 */
#define	FDIOCREQ	_IOWR('f', 0, struct fd_ioreq)	/* cmd request */
#define FDIOCGFORM	_IOR ('f', 1, struct fd_format_info)
							/* get format */
#define FDIOCSDENS	_IOW ('f', 2, int)		/* set density */
#define FDIOCSSIZE	_IOW ('f', 3, int)		/* set sector size */
#define FDIOCSGAPL	_IOW ('f', 4, int)		/* set Gap 3 length */
#define FDIOCRRW	_IOWR('f', 5, struct fd_rawio)	/* raw read/write */
#define FDIOCSIRETRY	_IOW ('f', 6, int)		/* set inner retry loop
							 * count */
#define FDIOCGIRETRY	_IOR ('f', 7, int)		/* get inner retry loop
							 * count */
#define FDIOCSORETRY	_IOW ('f', 8, int)		/* set outer retry loop
							 * count */
#define FDIOCGORETRY	_IOR ('f', 9, int)		/* get outer retry loop
							 * count */
#define	FDIOCGCAPLIST	_IOR ('f', 10, struct fd_cap_list)
				/* get list of possible	format capacities */
/*
 * 	software registers passed to controller during command sequences 
 */
 
struct fd_rw_cmd {
	/*
	 * 9 command bytes passed at start of read/write data
	 */
	 
#if 	__LITTLE_ENDIAN__

	u_char  opcode:5,
		sk:1,			/* skip sectors with deleted AM */
		mfm:1,			/* MFM encoding */
		mt:1; 			/* multi track */
	u_char	drive_sel:2,		/* drive select */
		hds:1,			/* head select */
	 	rsvd1:5;		/* reserved */
	u_char	cylinder;
	u_char	head;
	u_char	sector;
	u_char	sector_size;		/* 'n' in the Intel spec. */
	u_char	eot;			/* sector # at end of track */
	u_char	gap_length;		/* gap length */
	u_char	dtl;			/* special sector size */

#elif	__BIG_ENDIAN__

	u_char 	mt:1,			/* multitrack */
		mfm:1,			/* MFM encoding */
		sk:1,			/* skip sectors with deleted AM */
		opcode:5;
	u_char	rsvd1:5,		/* reserved */
		hds:1,			/* head select */
		drive_sel:2;		/* drive select */
	u_char	cylinder;
	u_char	head;
	u_char	sector;
	u_char	sector_size;		/* 'n' in the Intel spec. */
	u_char	eot;			/* sector # at end of track */
	u_char	gap_length;		/* gap length */
	u_char	dtl;			/* special sector size */
	
#else
#error	Floppy command / data structures are compiler sensitive
#endif
};

#define SIZEOF_RW_CMD	9		/* compiler yields 9 as sizeof this
					 * struct */
					 
struct fd_rw_stat {
	/* 
	 * 7 status bytes passed at completion of read/write data 
	 */

	u_char	stat0;
	u_char	stat1;
	u_char 	stat2;
	u_char	cylinder;
	u_char	head;
	u_char	sector;
	u_char	sector_size;		/* 'n' in the Intel spec. */
};
 
#define SIZEOF_RW_STAT	7

struct fd_int_stat {
	/* 
	 * result of Sense Interrupt Status command
	 */
	u_char 	stat0;	
	u_char	pcn;			/* present cylinder & */
};

struct fd_seek_cmd {
	/* 
	 * Seek command. Uses Sense Interrupt Status to get results.
	 */
	 
#if 	__LITTLE_ENDIAN__

	u_char	opcode:6,		/* will be FCCMD_SEEK */
		dir:1,			/* 1 == towards spindle for 
						relative seek*/
		relative:1;		/* 1 = relative */
	u_char	drive_sel:2,		/* drive select */
		hds:1,			/* head select */
		rsvd1:5;		/* reserved */
	u_char	cyl;			/* cylinder # or offset */
	
#elif	__BIG_ENDIAN__

	u_char	relative:1,		/* 1 = relative */
		dir:1,			/* 1 == towards spindle */
		opcode:6;		/* will be FCCMD_SEEK */
	u_char	rsvd1:5,		/* reserved */
		hds:1,			/* head select */
		drive_sel:2;		/* drive select */
	u_char	cyl;			/* cylinder # or offset */
	
#else
#error	Floppy command / data structures are compiler sensitive
#endif
};

#define SEEK_DIR_IN		1		/* seek towards spindle */
#define SEEK_DIR_OUT		0		/* seek away from spindle */

#define SIZEOF_SEEK_CMD		3

struct fd_recal_cmd {
	/* 
	 * Recalibrate command. Uses Sense Interrupt Status to get results.
	 */
	u_char	opcode;			/* will be FCCMD_RECAL */
	
#if 	__LITTLE_ENDIAN__

	u_char	drive_sel:2,		/* drive select */
		rsvd1:6;		/* reserved */
	
#elif	__BIG_ENDIAN__

	u_char	rsvd1:6,		/* reserved */
		drive_sel:2;		/* drive select */
	
#else
#error	Floppy command / data structures are compiler sensitive
#endif
};
	
struct fd_configure_cmd {
	/*
	 * configure command. No result bytes are returned.
	 */
	u_char 	opcode;			/* will be FCCMD_CONFIGURE */
	u_char	rsvd1;			/* must be 0 */
	u_char 	conf_2;			/* EIS, EFIFO, etc. */
	u_char	pretrk;			/* write precomp track # */
};

/*
 * configure command fields 
 */
#define CF2_EIS			0x40	/* enable implied seek */
#define CF2_EFIFO		0x20	/* enable FIFO. True Low. */
#define CF2_DPOLL		0x10	/* disable polling */
#define CF2_FIFO_DEFAULT	0x08	/* OUR default FIFO threshold */
#define CF_PRETRACK		0x00	/* OUR default precom track */
#define I82077_FIFO_SIZE	0x10	/* size of FIFO */

struct fd_specify_cmd {
	/*
	 * Specify command. No result bytes are returned.
	 */
	u_char 	opcode;			/* will be FCCMD_SPECIFY */
	
#if 	__LITTLE_ENDIAN__

	u_char 	hut:4,			/* head unload time */
		srt:4;			/* step rate */
	u_char	nd:1,			/* Non-DMA mode */
		hlt:7;			/* head load time */
	
#elif	__BIG_ENDIAN__

	u_char	srt:4,			/* step rate */
	 	hut:4;			/* head unload time */
	u_char	hlt:7,			/* head load time */
		nd:1;			/* Non-DMA mode */
	
#else
#error	Floppy command / data structures are compiler sensitive
#endif
};

#define SIZEOF_SPECIFY_CMD	3

struct fd_readid_cmd {
	/* 
	 * Read ID command. Returns status in an fd_rw_stat.
	 */
	 
#if 	__LITTLE_ENDIAN__

	u_char	opcode:6,		/* Will be FCCMD_READID */
		mfm:1,
	 	rsvd1:1;
	u_char	drive_sel:2,		/* drive select */
		hds:1,			/* head select */
		rsvd2:5;		/* reserved */
	
#elif	__BIG_ENDIAN__

	u_char 	rsvd1:1,
		mfm:1,
		opcode:6;		/* Will be FCCMD_READID */
	u_char	rsvd2:5,		/* reserved */
		hds:1,			/* head select */
		drive_sel:2;		/* drive select */
	
#else
#error	Floppy command / data structures are compiler sensitive
#endif
};

struct fd_perpendicular_cmd {
	u_char	opcode;			/* will be FCCMD_PERPENDICULAR */
	
#if 	__LITTLE_ENDIAN__

	u_char	gap:1,
		wgate:1,
		rsvd1:6;		/* must be 0 */
	
#elif	__BIG_ENDIAN__

	u_char	rsvd1:6,		/* must be 0 */
		wgate:1,
		gap:1;
	
#else
#error	Floppy command / data structures are compiler sensitive
#endif
};

struct fd_format_cmd {
	/*
	 * Format track command. Returns status in an fd_rw_stat
	 * (with undefined address fields).
	 */
	 
#if 	__LITTLE_ENDIAN__

	u_char	opcode:6,		/* will be FCCMD_FORMAT */
		mfm:1,
		rsvd1:1;
	u_char	drive_sel:2,		/* drive select */
		hds:1,			/* head select */
		rsvd2:5;		/* reserved */
	
#elif	__BIG_ENDIAN__

	u_char	rsvd1:1,
		mfm:1,
		opcode:6;		/* will be FCCMD_FORMAT */
	u_char	rsvd2:5,		/* reserved */
		hds:1,			/* head select */
		drive_sel:2;		/* drive select */
	
#else
#error	Floppy command / data structures are compiler sensitive
#endif
	u_char	n;			/* sector size (2**n * 128 = sect_size)
					 */
	u_char	sects_per_trk;
	u_char	gap_length;
	u_char	filler_data;		/* data field written with this byte */
};

struct fd_sense_drive_status_cmd {
	/*
	 * returns one result byte which is basically status register 3
	 */

	u_char	opcode; 	/* will be FCCMD_DRIVE_STATUS */
	
#if 	__LITTLE_ENDIAN__

	u_char	drive_sel:2,	/* drive select */
		hds:1,		/* head select */
	 	resvd2:5; 	/* always zero */
	
#elif	__BIG_ENDIAN__

	u_char	resvd2:5, 	/* always zero */
		hds:1,		/* head select */
	 	drive_sel:2;	/* drive select */
	
#else
#error	Floppy command / data structures are compiler sensitive
#endif
};

/*
 *	software register values
 */
 
/*
 * cmd (command byte). Not all of these opcodes are used with the fd_rw_cmd 
 * struct...
 */
#define FCCMD_OPCODE_MASK	0x1F
#define	FCCMD_MULTITRACK	0x80
#define FCCMD_MFM		0x40		/* MFM flag */
#define FCCMD_SK		0x20		/* skip flag */
#define FCCMD_READ		0x06
#define FCCMD_READ_DELETE	0x0C
#define FCCMD_WRITE		0x05
#define FCCMD_WRITE_DELETE	0x09
#define FCCMD_READ_TRACK	0x02
#define FCCMD_VERIFY		0x16
#define FCCMD_VERSION		0x10
#define FCCMD_FORMAT		0x0D
#define FCCMD_RECAL		0x07
#define FCCMD_INTSTAT		0x08		/* sense interrupt status */
#define FCCMD_SPECIFY		0x03
#define FCCMD_DRIVE_STATUS	0x04
#define FCCMD_SEEK		0x0F
#define FCCMD_CONFIGURE		0x13
#define FCCMD_DUMPREG		0x0E
#define FCCMD_READID		0x0A		/* Read ID */
#define FCCMD_PERPENDICULAR	0x12		/* perpendicular recording mode
						 */
/* 
 * rws_stat0 (status register 0)
 */
#define SR0_INTCODE		0xC0	/* interrupt code - see INTCODE_xxx */
#define INTCODE_COMPLETE	0x00	/* normal I/O complete */
#define INTCODE_ABNORMAL	0x40	/* abnormal termination */
#define INTCODE_INVALID		0x80	/* Invalid command */
#define INTCODE_POLL_TERM	0xC0	/* abnormal termination caused by 
					 * polling */
#define SR0_SEEKEND		0x20	/* Seek End */
#define SR0_EQ_CHECK		0x10	/* Equipment check (recal failed,
					 *    seek beyond track 0, etc.) */
#define SR0_HEAD		0x04	/* current head address */
#define SR0_DRVSEL		0x03
#define SR0_DRVSEL1		0x02	/* Drive Select 1 */
#define SR0_DRVSEL0		0x01	/* Drive Select 0 */

/*
 * rws_stat1 (status register 1)
 */
#define SR1_EOCYL		0x80	/* end of cylinder */
#define SR1_CRCERR		0x20	/* data or ID CRC error */
#define SR1_OURUN		0x10	/* DMA over/underrun */
#define SR1_NOHDR		0x04	/* Header Not Found */
#define SR1_NOT_WRT		0x02	/* Not writable */
#define SR1_MISS_AM		0x01	/* Missing Address mark */

/*
 * rws_stat2 (status register 2)
 */
#define SR2_CNTRL_MK		0x40	/* control mark */
#define SR2_DATACRC		0x20	/* Data CRC error */
#define SR2_WRONG_CYL		0x10	/* wrong cylinder */
#define SR2_BAD_CYL		0x02	/* Bad cylinder */
#define SR2_MISS_AM		0x01	/* missing data mark */

/*
 * rws_stat3 (status register 3)
 */
#define SR3_WP			0x40	/* write protected */
#define SR3_TRACK0		0x10	/* Track 0 */
#define SR3_HEAD		0x04	/* Head # */
#define SR3_DRVSEL1		0x02	/* same as status register 0?? */
#define SR3_DEVSEL0		0x01	

/*
 * disk info - maps media_id to tracks_per_cyl and num_cylinders.
 */
struct fd_disk_info {
	u_int 	media_id;			/* FD_MID_1MB, etc. */
	u_char	tracks_per_cyl;			/* # of heads */
	u_int	num_cylinders;	
	u_int	max_density;			/* maximum legal density.
						 * (FD_DENS_1, etc.) */
};

/*
 * sector size info - maps sector size to 82077 sector size code and
 * gap3 length.  Note that the controller must be programmed for different
 * gap3 size when formatting as opposed to reading or writing.  Since gap3
 * sizes may be dependent on the actual disk hardware, the fmt gap is
 * maintained here to avoid requiring the formatter to determine
 * the disk device type.
 */
struct fd_sectsize_info {
	u_int 		sect_size;		/* in bytes */
	u_char		n;			/* 82077 sectsize code */
	u_int		sects_per_trk;		/* physical sectors per trk */
	u_char		rw_gap_length;		/* Gap 3 for rw cmds */ 
	u_char		fmt_gap_length;		/* Gap 3 for fmt cmds */ 
};

typedef struct fd_sectsize_info fd_sectsize_info_t;

/*
 * density info - maps density to capacity. Note that a disk may be formatted
 * with a lower density than its max possible density.
 */
struct fd_density_info {
	u_int 		density;		/* FD_DENS_1, etc. */
	u_int 		capacity;		/* in bytes */
	boolean_t	mfm;			/* TRUE = MFM encoding */
};

typedef struct fd_density_info fd_density_info_t;

/*
 * disk format info. Used with ioctl(FDIOCGFORM).
 */
struct fd_format_info {
	/*
	 * the disk_info struct is always valid as long as a disk is
	 * inserted.
	 */
	struct fd_disk_info	disk_info;
	
	int			flags;		/* See FFI_xxx, below */

	/*
	 * the remainder is only valid if (flags & FFI_FORMATTED) is true.
	 */
	struct fd_density_info	density_info;
	struct fd_sectsize_info	sectsize_info;
	u_int			total_sects;	/* total # of sectors on 
						 * disk */
};

/*
 * fd_format_info.flags fields
 */
#define FFI_FORMATTED		0x00000001	/* known disk format */
#define FFI_LABELVALID		0x00000002	/* Valid NeXT file system label
						 * present */
#define FFI_WRITEPROTECT	0x00000004	/* disk is write protected */
						 
struct format_data {
	/* 
	 * one of these per sector. A Format command involves DMA'ing one of 
	 * these for each sector on a track. The format command is executed
	 * with a FDIOCREQ ioctl.
	 */
	u_char cylinder;
	u_char head;
	u_char sector;
	u_char n;			/* as in sector_size = 2**n * 128 */
};

/*
 * result of FCCMD_DUMPREG command
 */
struct fd_82077_regs {
	u_char	pcn[4];		/* cylinder # for drives 0..3 */
	u_char	srt:4,
		hut:4;
	u_char	hlt:7,
		nd:1;
	u_char	sc_eot;
	u_char	rsvd1;
	u_char	rsvd2:1,
		eis:1,
		efifo:1,
		poll:1,
		fifothr:4;
	u_char	pretrk;
};

/*
 * Used for FDIOCRRW - raw disk I/O 
 * -- no bad block mapping
 * -- no label required
 * -- no front porch 
 * -- block size = physical sector size
 */
struct fd_rawio {
	/*
	 * Passed to driver
	 */
	u_int 		sector;
	u_int		sector_count;
	caddr_t		dma_addrs;
	boolean_t	read;		/* TRUE = read; FALSE = write */
	/*
	 * Returned from driver
	 */
	fd_return_t	status;		/* FDR_xxx (see above) */
	u_int 		sects_xfr;	/* sectors actually moved */
};

/*
 * misc. hardware constants
 */
#define NUM_FD_HEADS 	2		/* number of tracks/cylinder */
#define NUM_FD_CYL	80		/* cylinders/disk (is this actually a 
					 * constant for all densities?) */
#define FD_PARK_TRACK	79		/* track to which to seek before
					 * eject */
#define NUM_UNITS	4		/* max # of drives per controller */ 

#endif /* _FDEXTERN_ */
