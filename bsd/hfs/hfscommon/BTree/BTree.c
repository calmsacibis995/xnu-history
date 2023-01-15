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

/*
	File:		BTree.c

	Contains:	Implementation of public interface routines for B-tree manager.

	Version:	HFS Plus 1.0

	Written by:	Gordon Sheridan and Bill Bruffey

	Copyright:	� 1992-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Don Brady

		Other Contact:		Mark Day

		Technology:			File Systems

	Writers:

		(msd)	Mark Day
		(DSH)	Deric Horn
		(djb)	Don Brady

	Change History (most recent first):
	   <Rhap>	 6/30/98	djb		In BTOpenPath make sure nodes are contiguous on disk (radar #2249539).
	   <Rhap>	 4/15/98	djb		In BTOpenPath need to clear nodeRec.buffer if GetBlockProc fails.
	   <Rhap>	 4/11/98	djb		Add RequireFileLock checking to all external entry points.

	   <Rhap>	03/23/98	djb		In BTOpenPath use kTrashBlock option when releasing the header so
	   								that we get a full node when we call GetNode. 

	   <CS9>	12/12/97	djb		Radar #2202682, BTIterateRecord with kBTreeCurrentRecord was not
									checking if we had a record and could call BlockMove with an
									uninitialize source pointer (causing a bus error).
	   <CS8>	10/24/97	msd		In BTIterateRecord, when moving to the previous or next record
									and we have to move to another node, see if we need to release
									the node about to be "shifted out" (opposite sibling of the
									direction we need to move).
	   <CS7>	 7/25/97	DSH		BTSearchRecord now takes a heuristicHint, nodeNum, and tries it
									before calling SearchBTree
	   <CS6>	 7/24/97	djb		GetBlockProc now take a file refnum instead of an FCB ptr.
	   <CS5>	 7/22/97	djb		Move trace points from BTreeWrapper.c to here.
	   <CS4>	 7/21/97	djb		LogEndTime now takes an error code.
	   <CS3>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	   <CS2>	 5/19/97	djb		Add summary traces to BTIterateRecord.
	   <CS1>	 4/23/97	djb		first checked in

	  <HFS7>	 2/19/97	djb		Enable variable sized index keys for HFS+ volumes. Added node
									cache to support nodes larger than 512 bytes.
	  <HFS6>	 1/27/97	djb		Calls to InsertTree and DeleteTree are now recursive (to support
									variable sized index keys).
	  <HFS5>	 1/13/97	djb		Added support for getting current record to BTIterateRecord.
	  <HFS4>	  1/6/97	djb		Initialize "BigKeys" attribute in BTOpen.
	  <HFS3>	  1/3/97	djb		Added support for large keys.
	  <HFS2>	12/23/96	djb		On exit map fsBTEmptyErr and fsBTEndOfIterationErr to
									fsBTRecordNotFoundErr.
	  <HFS1>	12/19/96	djb		first checked in

	History applicable to original Scarecrow Design:

		<13>	10/25/96	ser		Changing for new VFPI
		<12>	10/18/96	ser		Converting over VFPI changes
		<11>	 9/17/96	dkh		More BTree statistics. Modified hint checks to not bail out when
									an error is returned from GetNode.
		<10>	 9/16/96	dkh		Revised BTree statistics.
		 <9>	 8/23/96	dkh		Remove checks for multiple paths to BTree file. Need to add
									equivalent mechanism later.
		 <8>	 6/20/96	dkh		Radar #1358740. Switch from using Pools to debug MemAllocators.
		 <7>	 3/14/96	jev		Fix BTreeSetRecord, recordFound was not set for the case of a
									simple replace causing the leafRecords count to get bumped even
									though we didn't have to add a record.
		 <6>	  3/1/96	prp		Fix lint problems. Bug in BTSetRecord that does not initialize
									recordFound.
		 <5>	 1/22/96	dkh		Add #include Memory.h
		 <4>	 1/10/96	msd		Use the real function names from Math64.i.
		 <3>	  1/4/96	jev		Fix BTItererateRecord for the condition when the iterator
									position routine does not find the record and we are looking for
									the next record. In such a case, if the node's forrward link is
									non-zero, we have to keep iterating next and not return
									fsBTEndOfIterationErr error.
		 <2>	 12/7/95	dkh		D10E2 build. Changed usage of Ref data type to LogicalAddress.
		 <1>	10/18/95	rst		Moved from Scarecrow project.

		<24>	 7/18/95	mbb		Change MoveData & ClearBytes to BlockMoveData & BlockZero.
		<23>	 1/31/95	prp		GetBlockProc interface uses a 64 bit node number.
		<22>	 1/12/95	wjk		Adopt Model FileSystem changes in D5.
		<21>	11/16/94	prp		Add IsItAHint routine and use it whenever hint's node number was
									used for testing.
		<20>	11/10/94	prp		BTGetInfo name collides with the same name in FileManagerPriv.i.
									Change it to BTGetInformation.
		<19>	 9/30/94	prp		Get in sync with D2 interface changes.
		<18>	 7/22/94	wjk		Convert to the new set of header files.
		<17>	 12/9/93	wjk		Cleanup usage of char, Byte, int8, UInt8, etc.
		<16>	 12/2/93	wjk		Move from Makefiles to BuildFiles. Fit into the ModernOS and
									NRCmds environments.
		<15>	11/30/93	wjk		Move from Makefiles to BuildFiles. Fit into the ModernOS and
									NRCmds environments.
		<14>	 9/30/93	gs		Rename E_NoGetNodeProc and E_NoReleaseNodeProc to
									E_NoXxxxBlockProc.
		<13>	 8/31/93	prp		Use Set64U instead of Set64.
		<12>	 8/16/93	prp		In BTSearchRecord, if the input hint found the node and record,
									set the local nodeNum variable correctly so that the resultant
									iterator gets set correctly.
		<11>	  7/1/93	gs		Fix bug in BTIterateRecord related to kBTreePrevRecord
									operation.
		<10>	  6/2/93	gs		Update for changes to FSErrors.h and add some comments.
		 <9>	 5/24/93	gs		Fix bug in BTInsert/Set/ReplaceRecord which didn't set node hint
									properly in some cases.
		 <8>	 5/24/93	gs		Do NOT map fsBTEmptyErr to fsBTRecordNotFoundErr in BTSearchRecord.
		 <7>	 5/24/93	gs		Rename BTFlush to BTFlushPath.
		 <6>	 5/21/93	gs		Add hint optimization to Set/Replace routines.
		 <5>	 5/10/93	gs		Remove Panic from BTInitialize for small logicalEOF. Implement
									Insert, Set, Replace, and Delete.
		 <4>	 3/23/93	gs		Finish BTInitialize.
		 <3>	  2/8/93	gs		Implement BTSearchRecord and BTIterateRecord.
		 <2>	 12/8/92	gs		Implement Open and Close routines.
		 <1>	11/15/92	gs		first checked in

*/

#if TARGET_OS_MAC
#include <Memory.h>
#else
#include "../headers/system/MacOSTypes.h"
#endif 	/* TARGET_OS_MAC */


#include "../headers/FileMgrInternal.h"
#include "../headers/HFSBtreesPriv.h"

#include "../headers/BTreesPrivate.h"

#include "../headers/system/HFSInstrumentation.h"


#if TARGET_OS_RHAPSODY && DIAGNOSTIC
	#if DIAGNOSTIC
		extern void RequireFileLock(FileReference file, int shareable);
	#else
		#define RequireFileLock(f,s)
	#endif
#endif


//////////////////////////////////// Globals ////////////////////////////////////


/////////////////////////// BTree Module Entry Points ///////////////////////////


/*-------------------------------------------------------------------------------
Routine:	InitBTreeModule	-	Initialize BTree Module Global(s).

Function:	Initialize BTree code, if necessary

Input:		none

Output:		none

Result:		noErr		- success
			!= noErr	- can't happen
-------------------------------------------------------------------------------*/

OSStatus	InitBTreeModule(void)
{
	return noErr;
}


/*-------------------------------------------------------------------------------
Routine:	BTInitialize	-	Initialize a fork for access as a B*Tree.

Function:	Write Header node and create any map nodes necessary to map the fork
			as a B*Tree. If the fork is not large enough for the header node, the
			FS Agent is called to extend the LEOF. Additional map nodes will be
			allocated if necessary to represent the size of the fork. This allows
			the FS Agent to specify the initial size of B*Tree files.


Input:		pathPtr			- pointer to path control block
			maxKeyLength	- maximum length that will be used for any key in this B*Tree
			nodeSize		- node size for B*Tree (must be 2^n, 9 >= n >= 15)
			btreeType		- type of B*Tree
			keyDescPtr		- pointer to key descriptor (optional if key compare proc is used)

Output:		none

Result:		noErr		- success
			paramErr		- mandatory parameter was missing
			E_NoGetBlockProc		- FS Agent CB has no GetNodeProcPtr
			E_NoReleaseBlockProc	- FS Agent CB has no ReleaseNodeProcPtr
			E_NoSetEndOfForkProc	- FS Agent CB has no SetEndOfForkProcPtr
			E_NoSetBlockSizeProc	- FS Agent CB has no SetBlockSizeProcPtr
			fsBTrFileAlreadyOpenErr	- fork is already open as a B*Tree
			fsBTInvalidKeyLengthErr		- maximum key length is out of range
			E_BadNodeType			- node size is an illegal value
			fsBTUnknownVersionErr	- the B*Tree type is unknown by this module
			memFullErr			- not enough memory to initialize B*Tree
			!= noErr	- failure
-------------------------------------------------------------------------------*/
#if 0
OSStatus	BTInitialize		(FCB					*filePtr,
								 UInt16					 maxKeyLength,
								 UInt16					 nodeSize,
								 UInt8					 btreeType,
								 KeyDescriptorPtr		 keyDescPtr )
{
	OSStatus				err;
	FSForkControlBlockPtr	 	forkPtr;
	BTreeControlBlockPtr	btreePtr;
	BlockDescriptor			headerNode;
	HeaderPtr				header;
	Ptr						pos;
	FSSize					minEOF, maxEOF;
	SetEndOfForkProcPtr		setEndOfForkProc;
	SetBlockSizeProcPtr		setBlockSizeProc;

	////////////////////// Preliminary Error Checking ///////////////////////////

	headerNode.buffer	= nil;

	if (pathPtr										== nil)	return  paramErr;

	setEndOfForkProc	= pathPtr->agentPtr->agent.setEndOfForkProc;
	setBlockSizeProc	= pathPtr->agentPtr->agent.setBlockSizeProc;

	if (pathPtr->agentPtr->agent.getBlockProc		== nil)	return  E_NoGetBlockProc;
	if (pathPtr->agentPtr->agent.releaseBlockProc	== nil)	return  E_NoReleaseBlockProc;
	if (setEndOfForkProc							== nil)	return  E_NoSetEndOfForkProc;
	if (setBlockSizeProc							== nil)	return  E_NoSetBlockSizeProc;

	forkPtr = pathPtr->path.forkPtr;

	if (forkPtr->fork.btreePtr != nil)						return	fsBTrFileAlreadyOpenErr;

	if ((maxKeyLength == 0) ||
		(maxKeyLength >  kMaxKeyLength))					return	fsBTInvalidKeyLengthErr;

	if ( M_IsEven (maxKeyLength))	++maxKeyLength;			// len byte + even bytes + pad byte

	switch (nodeSize)										// node size == 512*2^n
	{
		case   512:
		case  1024:
		case  2048:
		case  4096:
		case  8192:
		case 16384:
		case 32768:		break;
		default:		return	E_BadNodeType;
	}

	switch (btreeType)
	{
		case kHFSBTreeType:
		case kUserBTreeType:
		case kReservedBTreeType:	break;

		default:					return	fsBTUnknownVersionErr;		//�� right?
	}


	//////////////////////// Allocate Control Block /////////////////////////////

	M_RESIDENT_ALLOCATE_FIXED_CLEAR( &btreePtr, sizeof( BTreeControlBlock ), kFSBTreeControlBlockType );
	if (btreePtr == nil)
	{
		err = memFullErr;
		goto ErrorExit;
	}

	btreePtr->version			= kBTreeVersion;			//�� what is the version?
	btreePtr->reserved1			= 0;
	btreePtr->flags				= 0;
	btreePtr->attributes		= 0;
	btreePtr->forkPtr			= forkPtr;
	btreePtr->keyCompareProc	= nil;
	btreePtr->keyDescPtr		= keyDescPtr;
	btreePtr->btreeType			= btreeType;
	btreePtr->treeDepth			= 0;
	btreePtr->rootNode			= 0;
	btreePtr->leafRecords		= 0;
	btreePtr->firstLeafNode		= 0;
	btreePtr->lastLeafNode		= 0;
	btreePtr->nodeSize			= nodeSize;
	btreePtr->maxKeyLength		= maxKeyLength;
	btreePtr->totalNodes		= 1;			// so ExtendBTree adds maps nodes properly
	btreePtr->freeNodes			= 0;
	btreePtr->writeCount		= 0;

	// set block size = nodeSize
	err = setBlockSizeProc (forkPtr, nodeSize);
	M_ExitOnError (err);

	////////////////////////////// Check LEOF ///////////////////////////////////

	minEOF	= S64Set ((SInt32) nodeSize);	//�� until real 64-bit math wjk 94.12.27
	if ( S64Compare (forkPtr->fork.logicalEOF, minEOF) < 0)
	{
		// allocate more space if necessary

		//��
		//�� until real 64-bit math wjk 94.12.27
		//��	???? Does this B*Tree pack stop working when LEOF > 2^32-1?
		//��
		maxEOF.hi = 0;
		maxEOF.lo = ((UInt32)0xFFFFFFFFL);

		err = setEndOfForkProc (forkPtr, minEOF, maxEOF);
		M_ExitOnError (err);
	};


	//////////////////////// Initialize Header Node /////////////////////////////

	err = GetNewNode (btreePtr, kHeaderNodeNum, &headerNode);
	M_ExitOnError (err);

	header = headerNode.buffer;

	header->node.type			= kHeaderNode;
	header->node.numRecords		= 3;			// header rec, key desc, map rec

	header->nodeSize			= nodeSize;
	header->maxKeyLength		= maxKeyLength;
	header->btreeType			= btreeType;
	header->totalNodes			= btreePtr->totalNodes;
	header->freeNodes			= btreePtr->totalNodes - 1;
	// ignore					  header->clumpSize;	//�� rename this field?

	// mark header node allocated in map record
	pos		= ((Ptr)headerNode.buffer) + kHeaderMapRecOffset;
	*pos	= 0x80;

	// set node offsets ( nodeSize-8, F8, 78, 0E)
	pos	 = ((Ptr)headerNode.buffer) + nodeSize;
	pos	-= 2;		*((UInt16 *)pos) = kHeaderRecOffset;
	pos	-= 2;		*((UInt16 *)pos) = kKeyDescRecOffset;
	pos -= 2;		*((UInt16 *)pos) = kHeaderMapRecOffset;
	pos -= 2;		*((UInt16 *)pos) = nodeSize - 8;


	///////////////////// Copy Key Descriptor To Header /////////////////////////
#if SupportsKeyDescriptors
	if (keyDescPtr != nil)
	{
		err = CheckKeyDescriptor (keyDescPtr, maxKeyLength);
		M_ExitOnError (err);

		// copy to header node
		pos = ((Ptr)headerNode.buffer) + kKeyDescRecOffset;
		BlockMoveData (keyDescPtr, pos, keyDescPtr->length + 1);
	}
#endif

	// write header node
	err = UpdateNode (btreePtr, &headerNode);
	M_ExitOnError (err);


	////////////////////////// Allocate Map Nodes ///////////////////////////////

	err = ExtendBTree (btreePtr, forkPtr->fork.logicalEOF.lo / nodeSize);	// sets totalNodes
	M_ExitOnError (err);


	////////////////////////////// Close BTree //////////////////////////////////

	err = UpdateHeader (btreePtr);
	M_ExitOnError (err);

	pathPtr->path.forkPtr->fork.btreePtr = nil;
	M_RESIDENT_DEALLOCATE_FIXED( btreePtr, sizeof( BTreeControlBlock ), kFSBTreeControlBlockType );

	return	noErr;


	/////////////////////// Error - Clean up and Exit ///////////////////////////

ErrorExit:

	(void) ReleaseNode (btreePtr, &headerNode);
	if (btreePtr != nil)
		M_RESIDENT_DEALLOCATE_FIXED( btreePtr, sizeof( BTreeControlBlock ), kFSBTreeControlBlockType );

	return	err;
}
#endif


/*-------------------------------------------------------------------------------
Routine:	BTOpenPath	-	Open a file for access as a B*Tree.

Function:	Create BTree control block for a file, if necessary. Validates the
			file to be sure it looks like a BTree file.


Input:		filePtr				- pointer to file to open as a B-tree
			keyCompareProc		- pointer to client's KeyCompare function
			getBlockProc		- pointer to client's GetBlock function
			releaseBlockProc	- pointer to client's ReleaseBlock function
			setEndOfForkProc	- pointer to client's SetEOF function

Result:		noErr				- success
			paramErr			- required ptr was nil
			fsBTInvalidFileErr				-
			memFullErr			-
			!= noErr			- failure
-------------------------------------------------------------------------------*/

OSStatus	BTOpenPath			(FCB					*filePtr,
								 KeyCompareProcPtr		 keyCompareProc,
								 GetBlockProcPtr		 getBlockProc,
								 ReleaseBlockProcPtr	 releaseBlockProc,
								 SetEndOfForkProcPtr	 setEndOfForkProc,
								 SetBlockSizeProcPtr	 setBlockSizeProc )
{
	OSStatus				err;
	BTreeControlBlockPtr	btreePtr;
	HeaderPtr				header;
	NodeRec					nodeRec;

	LogStartTime(kTraceOpenBTree);

	////////////////////// Preliminary Error Checking ///////////////////////////

	if ( filePtr == nil				||
		 getBlockProc == nil		||
		 releaseBlockProc == nil	||
		 setEndOfForkProc == nil	||
		 setBlockSizeProc == nil )
	{
		return  paramErr;
	}

	if ( filePtr->fcbBTCBPtr != nil )			// already has a BTreeCB
		return noErr;

												// is file large enough to contain header node?
	if ( filePtr->fcbEOF < kMinNodeSize )
		return fsBTInvalidFileErr;							//�� or E_BadHeader?


	//////////////////////// Allocate Control Block /////////////////////////////

	btreePtr = (BTreeControlBlock*) NewPtrSysClear( sizeof( BTreeControlBlock ) );
	if (btreePtr == nil)
	{
		Panic ("\pBTOpen: no memory for btreePtr.");
		return	memFullErr;
	}

	btreePtr->getBlockProc		= getBlockProc;
	btreePtr->releaseBlockProc	= releaseBlockProc;
	btreePtr->setEndOfForkProc	= setEndOfForkProc;
	btreePtr->keyCompareProc	= keyCompareProc;

	/////////////////////////// Read Header Node ////////////////////////////////

	nodeRec.buffer				= nil;				// so we can call ReleaseNode
	nodeRec.blockSize			= kMinNodeSize;
	btreePtr->fileRefNum		= GetFileRefNumFromFCB(filePtr);
	filePtr->fcbBTCBPtr			= (Ptr) btreePtr;	// attach btree cb to file

  #if TARGET_OS_RHAPSODY
	RequireFileLock(btreePtr->fileRefNum, false);
  #endif

	// it is now safe to call M_ExitOnError (err)

	err = setBlockSizeProc (btreePtr->fileRefNum, kMinNodeSize, 1);
	M_ExitOnError (err);


	err = getBlockProc (btreePtr->fileRefNum,
						kHeaderNodeNum,
						kGetBlock,
						&nodeRec );
	if (err != noErr)
	{
		nodeRec.buffer = nil;
		nodeRec.blockHeader	= nil;
		Panic("\pBTOpen: getNodeProc returned error getting header node.");
		goto ErrorExit;
	}

	header = nodeRec.buffer;


	///////////////////////////// verify header /////////////////////////////////

	err = VerifyHeader (filePtr, header);
	M_ExitOnError (err);


	///////////////////// Initalize fields from header //////////////////////////
	
    PanicIf ( (filePtr->fcbVPtr->vcbSigWord != 0x4244) && (btreePtr->nodeSize == 512), "\p BTOpenPath: wrong node size for HFS+ volume!");	// 0x4244 = 'BD'

	btreePtr->treeDepth			= header->treeDepth;
	btreePtr->rootNode			= header->rootNode;
	btreePtr->leafRecords		= header->leafRecords;
	btreePtr->firstLeafNode		= header->firstLeafNode;
	btreePtr->lastLeafNode		= header->lastLeafNode;
	btreePtr->nodeSize			= header->nodeSize;
	btreePtr->maxKeyLength		= header->maxKeyLength;
	btreePtr->totalNodes		= header->totalNodes;
	btreePtr->freeNodes			= header->freeNodes;
	// ignore					  header->clumpSize;	//�� rename this field?
	btreePtr->btreeType			= header->btreeType;

	btreePtr->attributes		= header->attributes;

	if ( btreePtr->maxKeyLength > 40 )
		btreePtr->attributes |= (kBTBigKeysMask + kBTVariableIndexKeysMask);	//�� we need a way to save these attributes

	/////////////////////// Initialize dynamic fields ///////////////////////////

	btreePtr->version			= kBTreeVersion;
	btreePtr->flags				= 0;
	btreePtr->writeCount		= 0;

	btreePtr->numGetNodes		= 1;		// for earlier call to getNodeProc

	/////////////////////////// Check Header Node ///////////////////////////////

	//�� set kBadClose attribute bit, and UpdateNode

	// if nodeSize is 512 then we don't need to release, just CheckNode

	if ( btreePtr->nodeSize == kMinNodeSize )
	{
		err = CheckNode (btreePtr, nodeRec.buffer);
		M_ExitOnError (err);
	}
	else
	{
		err = setBlockSizeProc (btreePtr->fileRefNum, btreePtr->nodeSize, 32);	//���we should try and get this down to 8
		M_ExitOnError (err);

#if TARGET_OS_RHAPSODY
		/*
		 * Need to use kTrashBlock option to force the
		 * buffer cache to read the entire node
		 */
		err = releaseBlockProc(btreePtr->fileRefNum, &nodeRec, kTrashBlock);
#else
		err = ReleaseNode (btreePtr, &nodeRec);
#endif
		M_ExitOnError (err);

		err = GetNode (btreePtr, kHeaderNodeNum, &nodeRec );		// calls CheckNode...
		M_ExitOnError (err);
	}

	//�� total nodes * node size <= LEOF?


	////////////////////////// Get Key Descriptor ///////////////////////////////
#if SupportsKeyDescriptors
	if ( keyCompareProc == nil )		// 	if no key compare proc then get key descriptor
	{
		err = GetKeyDescriptor (btreePtr, nodeRec.buffer);	//�� it should check amount of memory allocated...
		M_ExitOnError (err);

		err = CheckKeyDescriptor (btreePtr->keyDescPtr, btreePtr->maxKeyLength);
		M_ExitOnError (err);

	}
	else
#endif
	{
		btreePtr->keyDescPtr = nil;			// clear it so we don't dispose garbage later
	}

	err = ReleaseNode (btreePtr, &nodeRec);
	M_ExitOnError (err);

#if TARGET_OS_RHAPSODY
	/*
	 * Under Mac OS, b-tree nodes can be non-contiguous on disk when the
	 * allocation block size is smaller than the b-tree node size.
	 */
	if ( !NodesAreContiguous(filePtr->fcbVPtr, filePtr, btreePtr->nodeSize) )
		return fsBTInvalidNodeErr;
#endif

	//////////////////////////////// Success ////////////////////////////////////

	//�� align LEOF to multiple of node size?	- just on close

	LogEndTime(kTraceOpenBTree, noErr);

	return noErr;


	/////////////////////// Error - Clean up and Exit ///////////////////////////

ErrorExit:

	filePtr->fcbBTCBPtr = nil;
	(void) ReleaseNode (btreePtr, &nodeRec);
	DisposePtr( (Ptr) btreePtr );

	LogEndTime(kTraceOpenBTree, err);

	return err;
}



/*-------------------------------------------------------------------------------
Routine:	BTClosePath	-	Flush BTree Header and Deallocate Memory for BTree.

Function:	Flush the BTreeControlBlock fields to header node, and delete BTree control
			block and key descriptor associated with the file if filePtr is last
			path of type kBTreeType ('btre').


Input:		filePtr		- pointer to file to delete BTree control block for.

Result:		noErr			- success
			fsBTInvalidFileErr	-
			!= noErr		- failure
-------------------------------------------------------------------------------*/

OSStatus	BTClosePath			(FCB					*filePtr)
{
	OSStatus				err;
	BTreeControlBlockPtr	btreePtr;
#if 0
	FSPathControlBlockPtr	tempPath;
	Boolean					otherBTreePathsOpen;
#endif

	LogStartTime(kTraceCloseBTree);

	btreePtr = (BTreeControlBlockPtr) filePtr->fcbBTCBPtr;

	if (btreePtr == nil)
		return fsBTInvalidFileErr;

  #if TARGET_OS_RHAPSODY
	RequireFileLock(btreePtr->fileRefNum, false);
  #endif

	////////////////////// Check for other BTree Paths //////////////////////////

#if 0
//�� Need replacement field for pathType 
	otherBTreePathsOpen = false;
	tempPath = forkPtr->fork.pathList.head;
	while ( (tempPath != (FSPathControlBlockPtr) &forkPtr->fork.pathList) &&
			(otherBTreePathsOpen == false) )
	{
		if ((tempPath != pathPtr) && (tempPath->path.pathType == kBTreeType))
		{
			otherBTreePathsOpen = true;
			break;							// done with loop check
		}

		tempPath = tempPath->next;
	}

	////////////////////////// Update Header Node ///////////////////////////////


	if (otherBTreePathsOpen == true)
	{
		err = UpdateHeader (btreePtr);			// update header even if we aren't closing
		return err;								// we only clean up after the last user...
	}
#endif

	btreePtr->attributes &= ~kBTBadCloseMask;		// clear "bad close" attribute bit
	err = UpdateHeader (btreePtr);
	M_ExitOnError (err);

#if SupportsKeyDescriptors
	if (btreePtr->keyDescPtr != nil)			// deallocate keyDescriptor, if any
	{
		DisposePtr( (Ptr) btreePtr->keyDescPtr );
	}
#endif

	DisposePtr( (Ptr) btreePtr );
	filePtr->fcbBTCBPtr = nil;

	LogEndTime(kTraceCloseBTree, noErr);

	return	noErr;

	/////////////////////// Error - Clean Up and Exit ///////////////////////////

ErrorExit:

	LogEndTime(kTraceCloseBTree, err);

	return	err;
}



/*-------------------------------------------------------------------------------
Routine:	BTSearchRecord	-	Search BTree for a record with a matching key.

Function:	Search for position in B*Tree indicated by searchKey. If a valid node hint
			is provided, it will be searched first, then SearchTree will be called.
			If a BTreeIterator is provided, it will be set to the position found as
			a result of the search. If a record exists at that position, and a BufferDescriptor
			is supplied, the record will be copied to the buffer (as much as will fit),
			and recordLen will be set to the length of the record.

			If an error other than fsBTRecordNotFoundErr occurs, the BTreeIterator, if any,
			is invalidated, and recordLen is set to 0.


Input:		pathPtr			- pointer to path for BTree file.
			searchKey		- pointer to search key to match.
			hintPtr			- pointer to hint (may be nil)

Output:		record			- pointer to BufferDescriptor containing record
			recordLen		- length of data at recordPtr
			iterator		- pointer to BTreeIterator indicating position result of search

Result:		noErr			- success, record contains copy of record found
			fsBTRecordNotFoundErr	- record was not found, no data copied
			fsBTInvalidFileErr	- no BTreeControlBlock is allocated for the fork
			fsBTInvalidKeyLengthErr		-
			!= noErr		- failure
-------------------------------------------------------------------------------*/

OSStatus	BTSearchRecord		(FCB						*filePtr,
								 BTreeIterator				*searchIterator,
								 UInt32						heuristicHint,
								 FSBufferDescriptor			*record,
								 UInt16						*recordLen,
								 BTreeIterator				*resultIterator )
{
	OSStatus				err;
	BTreeControlBlockPtr	btreePtr;
	TreePathTable			treePathTable;
	UInt32					nodeNum;
	BlockDescriptor			node;
	UInt16					index;
	BTreeKeyPtr				keyPtr;
	RecordPtr				recordPtr;
	UInt16					len;
	Boolean					foundRecord;
	Boolean					validHint;


	LogStartTime(kTraceSearchBTree);

	if (filePtr == nil)									return	paramErr;
	if (searchIterator == nil)							return	paramErr;

	btreePtr = (BTreeControlBlockPtr) filePtr->fcbBTCBPtr;
	if (btreePtr == nil)								return	fsBTInvalidFileErr;

  #if TARGET_OS_RHAPSODY
	RequireFileLock(btreePtr->fileRefNum, true);
  #endif

#if SupportsKeyDescriptors
	if (btreePtr->keyCompareProc == nil)		// CheckKey if we using Key Descriptor
	{
		err = CheckKey (&searchIterator->key, btreePtr->keyDescPtr, btreePtr->maxKeyLength);
		M_ExitOnError (err);
	}
#endif

	foundRecord = false;

	////////////////////////////// Take A Hint //////////////////////////////////

	err = IsItAHint (btreePtr, searchIterator, &validHint);
	M_ExitOnError (err);

	if (validHint)
	{
		nodeNum = searchIterator->hint.nodeNum;
		
		err = GetNode (btreePtr, nodeNum, &node);
		if( err == noErr )
		{
			if ( ((BTNodeDescriptor*) node.buffer)->type		== kLeafNode &&
				 ((BTNodeDescriptor*) node.buffer)->numRecords	>  0 )
			{
				foundRecord = SearchNode (btreePtr, node.buffer, &searchIterator->key, &index);

				//�� if !foundRecord, we could still skip tree search if ( 0 < index < numRecords )
			}

			if (foundRecord == false)
			{
				err = ReleaseNode (btreePtr, &node);
				M_ExitOnError (err);
			}
			else
			{
				++btreePtr->numValidHints;
			}
		}
		
		if( foundRecord == false )
			(void) BTInvalidateHint( searchIterator );
	}

	////////////////////////////// Try the heuristicHint //////////////////////////////////

	if ( (foundRecord == false) && (heuristicHint != kInvalidMRUCacheKey) && (nodeNum != heuristicHint) )
	{
		LogStartTime(kHeuristicHint);
		nodeNum = heuristicHint;
		
		err = GetNode (btreePtr, nodeNum, &node);
		if( err == noErr )
		{
			if ( ((BTNodeDescriptor*) node.buffer)->type		== kLeafNode &&
				 ((BTNodeDescriptor*) node.buffer)->numRecords	>  0 )
			{
				foundRecord = SearchNode (btreePtr, node.buffer, &searchIterator->key, &index);
			}

			if (foundRecord == false)
			{
				err = ReleaseNode (btreePtr, &node);
				M_ExitOnError (err);
			}
		}
		LogEndTime(kHeuristicHint, (foundRecord == false));
	}

	//////////////////////////// Search The Tree ////////////////////////////////

	if (foundRecord == false)
	{
		err = SearchTree ( btreePtr, &searchIterator->key, treePathTable, &nodeNum, &node, &index);
		switch (err)
		{
			case noErr:			foundRecord = true;				break;
			case fsBTRecordNotFoundErr:									break;
			default:				goto ErrorExit;
		}
	}


	//////////////////////////// Get the Record /////////////////////////////////

	if (foundRecord == true)
	{
		//XXX Should check for errors! Or BlockMove could choke on recordPtr!!!
		GetRecordByIndex (btreePtr, node.buffer, index, &keyPtr, &recordPtr, &len);

		if (recordLen != nil)			*recordLen = len;

		if (record != nil)
		{
			ByteCount recordSize;

			recordSize = record->itemCount * record->itemSize;
			
			PanicIf(len > recordSize, "\pBTSearchRecord: truncating record!");

			if (len > recordSize)	len = recordSize;

			BlockMoveData (recordPtr, record->bufferAddress, len);
		}
	}


	/////////////////////// Success - Update Iterator ///////////////////////////

	if (resultIterator != nil)
	{
		resultIterator->hint.writeCount	= btreePtr->writeCount;
		resultIterator->hint.nodeNum	= nodeNum;
		resultIterator->hint.index		= index;
		resultIterator->hint.reserved1	= 0;
		resultIterator->hint.reserved2	= 0;

		resultIterator->version			= 0;
		resultIterator->reserved		= 0;

		// copy the key in the BTree when found rather than searchIterator->key to get proper case/diacriticals
		if (foundRecord == true)
			BlockMoveData ((Ptr)keyPtr, (Ptr)&resultIterator->key, CalcKeySize(btreePtr, keyPtr));
		else
			BlockMoveData ((Ptr)&searchIterator->key, (Ptr)&resultIterator->key, CalcKeySize(btreePtr, &searchIterator->key));
	}

	err = ReleaseNode (btreePtr, &node);
	M_ExitOnError (err);

	LogEndTime(kTraceSearchBTree, (foundRecord == false));

	if (foundRecord == false)	return	fsBTRecordNotFoundErr;
	else						return	noErr;


	/////////////////////// Error - Clean Up and Exit ///////////////////////////

ErrorExit:

	if (recordLen != nil)
		*recordLen = 0;

	if (resultIterator != nil)
	{
		resultIterator->hint.writeCount	= 0;
		resultIterator->hint.nodeNum	= 0;
		resultIterator->hint.index		= 0;
		resultIterator->hint.reserved1	= 0;
		resultIterator->hint.reserved2	= 0;

		resultIterator->version			= 0;
		resultIterator->reserved		= 0;
		resultIterator->key.length16	= 0;	// zero out two bytes to cover both types of keys
	}

	if ( err == fsBTEmptyErr )
		err = fsBTRecordNotFoundErr;

	LogEndTime(kTraceSearchBTree, err);

	return err;
}



/*-------------------------------------------------------------------------------
Routine:	BTIterateRecord	-	Find the first, next, previous, or last record.

Function:	Find the first, next, previous, or last record in the BTree

Input:		pathPtr			- pointer to path iterate records for.
			operation		- iteration operation (first,next,prev,last)
			iterator		- pointer to iterator indicating start position

Output:		iterator		- iterator is updated to indicate new position
			newKeyPtr		- pointer to buffer to copy key found by iteration
			record			- pointer to buffer to copy record found by iteration
			recordLen		- length of record

Result:		noErr			- success
			!= noErr		- failure
-------------------------------------------------------------------------------*/

OSStatus	BTIterateRecord		(FCB						*filePtr,
								 BTreeIterationOperation	 operation,
								 BTreeIterator				*iterator,
								 FSBufferDescriptor			*record,
								 UInt16						*recordLen )
{
	OSStatus					err;
	BTreeControlBlockPtr		btreePtr;
	BTreeKeyPtr					keyPtr;
	RecordPtr					recordPtr;
	UInt16						len;

	Boolean						foundRecord;
	UInt32						nodeNum;

	BlockDescriptor				left,		node,		right;
	UInt16						index;


	LogStartTime(kTraceGetBTreeRecord);

	////////////////////////// Priliminary Checks ///////////////////////////////

	left.buffer		= nil;
	right.buffer	= nil;
	node.buffer		= nil;


	if (filePtr == nil)
	{
		return	paramErr;
	}

	btreePtr = (BTreeControlBlockPtr) filePtr->fcbBTCBPtr;
	if (btreePtr == nil)
	{
		return	fsBTInvalidFileErr;			//�� handle properly
	}

  #if TARGET_OS_RHAPSODY
	RequireFileLock(btreePtr->fileRefNum, true);
  #endif

	if ((operation != kBTreeFirstRecord)	&&
		(operation != kBTreeNextRecord)		&&
		(operation != kBTreeCurrentRecord)	&&
		(operation != kBTreePrevRecord)		&&
		(operation != kBTreeLastRecord))
	{
		err = fsInvalidIterationMovmentErr;
		goto ErrorExit;
	}

	/////////////////////// Find First or Last Record ///////////////////////////

	if ((operation == kBTreeFirstRecord) || (operation == kBTreeLastRecord))
	{
		if (operation == kBTreeFirstRecord)		nodeNum = btreePtr->firstLeafNode;
		else									nodeNum = btreePtr->lastLeafNode;

		if (nodeNum == 0)
		{
			err = fsBTEmptyErr;
			goto ErrorExit;
		}

		err = GetNode (btreePtr, nodeNum, &node);
		M_ExitOnError (err);

		if ( ((NodeDescPtr) node.buffer)->type			!= kLeafNode ||
			 ((NodeDescPtr) node.buffer)->numRecords	<=  0 )
		{
			err = ReleaseNode (btreePtr, &node);
			M_ExitOnError (err);

			err = fsBTInvalidNodeErr;
			goto ErrorExit;
		}

		if (operation == kBTreeFirstRecord)		index = 0;
		else									index = ((BTNodeDescriptor*) node.buffer)->numRecords - 1;

		goto CopyData;						//�� is there a cleaner way?
	}


	//////////////////////// Find Iterator Position /////////////////////////////

	err = FindIteratorPosition (btreePtr, iterator,
								&left, &node, &right, &nodeNum, &index, &foundRecord);
	M_ExitOnError (err);


	///////////////////// Find Next Or Previous Record //////////////////////////

	if (operation == kBTreePrevRecord)
	{
		if (index > 0)
		{
			--index;
		}
		else
		{
			if (left.buffer == nil)
			{
				nodeNum = ((NodeDescPtr) node.buffer)->bLink;
				if ( nodeNum > 0)
				{
					err = GetNode (btreePtr, nodeNum, &left);
					M_ExitOnError (err);
				} else {
					err = fsBTStartOfIterationErr;
					goto ErrorExit;
				}
			}
			//	Before we stomp on "right", we'd better release it if needed
			if (right.buffer != nil) {
				err = ReleaseNode(btreePtr, &right);
				M_ExitOnError(err);
			}
			right		= node;
			node		= left;
			left.buffer	= nil;
			index 		= ((NodeDescPtr) node.buffer)->numRecords -1;
		}
	}
	else if (operation == kBTreeNextRecord)
	{
		if ((foundRecord != true) &&
			(((NodeDescPtr) node.buffer)->fLink == 0) &&
			(index == ((NodeDescPtr) node.buffer)->numRecords))
		{
			err = fsBTEndOfIterationErr;
			goto ErrorExit;
		} 
	
		// we did not find the record but the index is already positioned correctly
		if ((foundRecord == false) && (index != ((NodeDescPtr) node.buffer)->numRecords)) 
			goto CopyData;

		// we found the record OR we have to look in the next node
		if (index < ((NodeDescPtr) node.buffer)->numRecords -1)
		{
			++index;
		}
		else
		{
			if (right.buffer == nil)
			{
				nodeNum = ((NodeDescPtr) node.buffer)->fLink;
				if ( nodeNum > 0)
				{
					err = GetNode (btreePtr, nodeNum, &right);
					M_ExitOnError (err);
				} else {
					err = fsBTEndOfIterationErr;
					goto ErrorExit;
				}
			}
			//	Before we stomp on "left", we'd better release it if needed
			if (left.buffer != nil) {
				err = ReleaseNode(btreePtr, &left);
				M_ExitOnError(err);
			}
			left		 = node;
			node		 = right;
			right.buffer = nil;
			index		 = 0;
		}
	}
	else // operation == kBTreeCurrentRecord
	{
		// make sure we have something... <CS9>
		if ((foundRecord != true) &&
			(index >= ((NodeDescPtr) node.buffer)->numRecords))
		{
			err = fsBTEndOfIterationErr;
			goto ErrorExit;
		} 
	}

	//////////////////// Copy Record And Update Iterator ////////////////////////

CopyData:

	// added check for errors <CS9>
	err = GetRecordByIndex (btreePtr, node.buffer, index, &keyPtr, &recordPtr, &len);
	M_ExitOnError (err);

	if (recordLen != nil)			*recordLen = len;

	if (record != nil)
	{
		ByteCount recordSize;

		recordSize = record->itemCount * record->itemSize;

		PanicIf(len > recordSize, "\pBTIterateRecord: truncating record!");
	
		if (len > recordSize)	len = recordSize;

		BlockMoveData (recordPtr, record->bufferAddress, len);
	}

	if (iterator != nil)						// first & last do not require iterator
	{
		iterator->hint.writeCount	= btreePtr->writeCount;
		iterator->hint.nodeNum		= nodeNum;
		iterator->hint.index		= index;
		iterator->hint.reserved1	= 0;
		iterator->hint.reserved2	= 0;

		iterator->version			= 0;
		iterator->reserved			= 0;

		BlockMoveData ((Ptr)keyPtr, (Ptr)&iterator->key, CalcKeySize(btreePtr, keyPtr));
	}


	///////////////////////////// Release Nodes /////////////////////////////////

	err = ReleaseNode (btreePtr, &node);
	M_ExitOnError (err);

	if (left.buffer != nil)
	{
		err = ReleaseNode (btreePtr, &left);
		M_ExitOnError (err);
	}

	if (right.buffer != nil)
	{
		err = ReleaseNode (btreePtr, &right);
		M_ExitOnError (err);
	}

	LogEndTime(kTraceGetBTreeRecord, noErr);

	return noErr;

	/////////////////////// Error - Clean Up and Exit ///////////////////////////

ErrorExit:

	(void)	ReleaseNode (btreePtr, &left);
	(void)	ReleaseNode (btreePtr, &node);
	(void)	ReleaseNode (btreePtr, &right);

	if (recordLen != nil)
		*recordLen = 0;

	if (iterator != nil)
	{
		iterator->hint.writeCount	= 0;
		iterator->hint.nodeNum		= 0;
		iterator->hint.index		= 0;
		iterator->hint.reserved1	= 0;
		iterator->hint.reserved2	= 0;

		iterator->version			= 0;
		iterator->reserved			= 0;
		iterator->key.length16		= 0;
	}

	if ( err == fsBTEmptyErr || err == fsBTEndOfIterationErr )
		err = fsBTRecordNotFoundErr;

	LogEndTime(kTraceGetBTreeRecord, err);

	return err;
}


//////////////////////////////// BTInsertRecord /////////////////////////////////

OSStatus	BTInsertRecord		(FCB						*filePtr,
								 BTreeIterator				*iterator,
								 FSBufferDescriptor			*record,
								 UInt16						 recordLen )
{
	OSStatus				err;
	BTreeControlBlockPtr	btreePtr;
	TreePathTable			treePathTable;
	SInt32					nodesNeeded;
	BlockDescriptor			nodeRec;
	UInt32					insertNodeNum;
	UInt16					index;
	Boolean					recordFit;


	////////////////////////// Priliminary Checks ///////////////////////////////

	nodeRec.buffer = nil;					// so we can call ReleaseNode

	err = CheckInsertParams (filePtr, iterator, record, recordLen);
	if (err != noErr)
		return	err;

	LogStartTime(kTraceInsertBTreeRecord);

	btreePtr = (BTreeControlBlockPtr) filePtr->fcbBTCBPtr;

  #if TARGET_OS_RHAPSODY
	RequireFileLock(btreePtr->fileRefNum, false);
  #endif

	///////////////////////// Find Insert Position //////////////////////////////

	// always call SearchTree for Insert
	err = SearchTree (btreePtr, &iterator->key, treePathTable, &insertNodeNum, &nodeRec, &index);

	switch (err)				// set/replace/insert decision point
	{
		case noErr:			err = fsBTDuplicateRecordErr;
								goto ErrorExit;

		case fsBTRecordNotFoundErr:	break;

		case fsBTEmptyErr:	// if tree empty add 1st leaf node

								if (btreePtr->freeNodes == 0)
								{
									err = ExtendBTree (btreePtr, btreePtr->totalNodes + 1);
									M_ExitOnError (err);
								}

								err = AllocateNode (btreePtr, &insertNodeNum);
								M_ExitOnError (err);

								err = GetNewNode (btreePtr, insertNodeNum, &nodeRec);
								M_ExitOnError (err);

								((NodeDescPtr)nodeRec.buffer)->type		= kLeafNode;
								((NodeDescPtr)nodeRec.buffer)->height	= 1;

								recordFit = InsertKeyRecord (btreePtr, nodeRec.buffer, 0,
															 &iterator->key, KeyLength(btreePtr, &iterator->key),
															 record->bufferAddress, recordLen );
								if (recordFit != true)
								{
									err = fsBTRecordTooLargeErr;
									goto ErrorExit;
								}

								err = UpdateNode (btreePtr, &nodeRec);
								M_ExitOnError (err);

								// update BTreeControlBlock
								btreePtr->treeDepth	 		= 1;
								btreePtr->rootNode	 		= insertNodeNum;
								btreePtr->firstLeafNode		= insertNodeNum;
								btreePtr->lastLeafNode		= insertNodeNum;
								M_BTreeHeaderDirty (btreePtr);

								goto Success;

		default:				goto ErrorExit;
	}

	if (index > 0)
	{
		recordFit = InsertKeyRecord (btreePtr, nodeRec.buffer, index,
										&iterator->key, KeyLength(btreePtr, &iterator->key),
										record->bufferAddress, recordLen);
		if (recordFit == true)
		{
			err = UpdateNode (btreePtr, &nodeRec);
			M_ExitOnError (err);

			goto Success;
		}
	}

	/////////////////////// Extend File If Necessary ////////////////////////////

	nodesNeeded =  btreePtr->treeDepth + 1 - btreePtr->freeNodes;	//�� math limit
	if (nodesNeeded > 0)
	{
		nodesNeeded += btreePtr->totalNodes;
		if (nodesNeeded > CalcMapBits (btreePtr))	// we'll need to add a map node too!
			++nodesNeeded;

		err = ExtendBTree (btreePtr, nodesNeeded);
		M_ExitOnError (err);
	}

	// no need to delete existing record

	err = InsertTree (btreePtr, treePathTable, &iterator->key, record->bufferAddress,
					  recordLen, &nodeRec, index, 1, kInsertRecord, &insertNodeNum);
	M_ExitOnError (err);


	//////////////////////////////// Success ////////////////////////////////////

Success:
	++btreePtr->leafRecords;
	M_BTreeHeaderDirty (btreePtr);

	// create hint
	iterator->hint.writeCount 	= btreePtr->writeCount;		//�� currently unused
	iterator->hint.nodeNum		= insertNodeNum;
	iterator->hint.index		= 0;						// unused
	iterator->hint.reserved1	= 0;
	iterator->hint.reserved2	= 0;

	LogEndTime(kTraceInsertBTreeRecord, noErr);

	return noErr;


	////////////////////////////// Error Exit ///////////////////////////////////

ErrorExit:

	(void) ReleaseNode (btreePtr, &nodeRec);

	iterator->hint.writeCount 	= 0;
	iterator->hint.nodeNum		= 0;
	iterator->hint.index		= 0;
	iterator->hint.reserved1	= 0;
	iterator->hint.reserved2	= 0;
	
	if (err == fsBTEmptyErr)
		err = fsBTRecordNotFoundErr;

	LogEndTime(kTraceInsertBTreeRecord, err);

	return err;
}



////////////////////////////////// BTSetRecord //////////////////////////////////

OSStatus	BTSetRecord			(FCB						*filePtr,
								 BTreeIterator				*iterator,
								 FSBufferDescriptor			*record,
								 UInt16						 recordLen )
{
	OSStatus				err;
	BTreeControlBlockPtr	btreePtr;
	TreePathTable			treePathTable;
	SInt32					nodesNeeded;
	BlockDescriptor			nodeRec;
	UInt32					insertNodeNum;
	UInt16					index;
	Boolean					recordFound = false;
	Boolean					recordFit;
	Boolean					operation;
	Boolean					validHint;


	////////////////////////// Priliminary Checks ///////////////////////////////

	nodeRec.buffer = nil;					// so we can call ReleaseNode

	err = CheckInsertParams (filePtr, iterator, record, recordLen);
	if (err != noErr)
		return	err;

	btreePtr = (BTreeControlBlockPtr) filePtr->fcbBTCBPtr;

  #if TARGET_OS_RHAPSODY
	RequireFileLock(btreePtr->fileRefNum, false);
  #endif

	///////////////////////// Find Insert Position //////////////////////////////

	err = IsItAHint (btreePtr, iterator, &validHint);
	M_ExitOnError (err);

	if (validHint)
	{
		insertNodeNum = iterator->hint.nodeNum;

		err = GetNode (btreePtr, insertNodeNum, &nodeRec);
		if( err == noErr )
		{
			err = TrySimpleReplace (btreePtr, nodeRec.buffer, iterator, record, recordLen, &recordFit);
			M_ExitOnError (err);

			if (recordFit)
			{
				err = UpdateNode (btreePtr, &nodeRec);
				M_ExitOnError (err);
				
				recordFound = true;
				++btreePtr->numValidHints;
				goto Success;
			} 											// else
			else
			{
				(void) BTInvalidateHint( iterator );
			}
			
			err = ReleaseNode (btreePtr, &nodeRec);
			M_ExitOnError (err);
		}
	}

	err = SearchTree (btreePtr, &iterator->key, treePathTable, &insertNodeNum, &nodeRec, &index);

	switch (err)				// set/replace/insert decision point
	{
		case noErr:			recordFound = true;
								break;

		case fsBTRecordNotFoundErr:	break;

		case fsBTEmptyErr:	// if tree empty add 1st leaf node

								if (btreePtr->freeNodes == 0)
								{
									err = ExtendBTree (btreePtr, btreePtr->totalNodes + 1);
									M_ExitOnError (err);
								}

								err = AllocateNode (btreePtr, &insertNodeNum);
								M_ExitOnError (err);

								err = GetNewNode (btreePtr, insertNodeNum, &nodeRec);
								M_ExitOnError (err);

								((NodeDescPtr)nodeRec.buffer)->type		= kLeafNode;
								((NodeDescPtr)nodeRec.buffer)->height	= 1;

								recordFit = InsertKeyRecord (btreePtr, nodeRec.buffer, 0,
															 &iterator->key, KeyLength(btreePtr, &iterator->key),
															 record->bufferAddress, recordLen );
								if (recordFit != true)
								{
									err = fsBTRecordTooLargeErr;
									goto ErrorExit;
								}

								err = UpdateNode (btreePtr, &nodeRec);
								M_ExitOnError (err);

								// update BTreeControlBlock
								btreePtr->rootNode	 = insertNodeNum;
								btreePtr->treeDepth	 = 1;
								btreePtr->flags		|= kBTHeaderDirty;

								goto Success;

		default:				goto ErrorExit;
	}


	if (recordFound == true)		// Simple Replace - optimization avoids unecessary ExtendBTree
	{
		err = TrySimpleReplace (btreePtr, nodeRec.buffer, iterator, record, recordLen, &recordFit);
		M_ExitOnError (err);

		if (recordFit)
		{
			err = UpdateNode (btreePtr, &nodeRec);
			M_ExitOnError (err);

			goto Success;
		}
	}


	/////////////////////// Extend File If Necessary ////////////////////////////

	nodesNeeded =  btreePtr->treeDepth + 1 - btreePtr->freeNodes;	//�� math limit
	if (nodesNeeded > 0)
	{
		nodesNeeded += btreePtr->totalNodes;
		if (nodesNeeded > CalcMapBits (btreePtr))	// we'll need to add a map node too!
			++nodesNeeded;

		err = ExtendBTree (btreePtr, nodesNeeded);
		M_ExitOnError (err);
	}


	if (recordFound == true)							// Delete existing record
	{
		DeleteRecord (btreePtr, nodeRec.buffer, index);
		operation = kReplaceRecord;
	} else {
		operation = kInsertRecord;
	}

	err = InsertTree (btreePtr, treePathTable, &iterator->key, record->bufferAddress,
					  recordLen, &nodeRec, index, 1, operation, &insertNodeNum);
	M_ExitOnError (err);


Success:
	if (recordFound == false)
	{
		++btreePtr->leafRecords;
		M_BTreeHeaderDirty (btreePtr);
	}

	// create hint
	iterator->hint.writeCount 	= btreePtr->writeCount;		//�� currently unused
	iterator->hint.nodeNum		= insertNodeNum;
	iterator->hint.index		= 0;						// unused
	iterator->hint.reserved1	= 0;
	iterator->hint.reserved2	= 0;

	return noErr;


	////////////////////////////// Error Exit ///////////////////////////////////

ErrorExit:

	(void) ReleaseNode (btreePtr, &nodeRec);

	iterator->hint.writeCount 	= 0;
	iterator->hint.nodeNum		= 0;
	iterator->hint.index		= 0;
	iterator->hint.reserved1	= 0;
	iterator->hint.reserved2	= 0;

	return err;
}



//////////////////////////////// BTReplaceRecord ////////////////////////////////

OSStatus	BTReplaceRecord		(FCB						*filePtr,
								 BTreeIterator				*iterator,
								 FSBufferDescriptor			*record,
								 UInt16						 recordLen )
{
	OSStatus				err;
	BTreeControlBlockPtr	btreePtr;
	TreePathTable			treePathTable;
	SInt32					nodesNeeded;
	BlockDescriptor			nodeRec;
	UInt32					insertNodeNum;
	UInt16					index;
	Boolean					recordFit;
	Boolean					validHint;


	////////////////////////// Priliminary Checks ///////////////////////////////

	nodeRec.buffer = nil;					// so we can call ReleaseNode

	err = CheckInsertParams (filePtr, iterator, record, recordLen);
	if (err != noErr)
		return err;

	LogStartTime(kTraceReplaceBTreeRecord);

	btreePtr = (BTreeControlBlockPtr) filePtr->fcbBTCBPtr;

  #if TARGET_OS_RHAPSODY
	RequireFileLock(btreePtr->fileRefNum, false);
  #endif

	////////////////////////////// Take A Hint //////////////////////////////////

	err = IsItAHint (btreePtr, iterator, &validHint);
	M_ExitOnError (err);

	if (validHint)
	{
		insertNodeNum = iterator->hint.nodeNum;

		err = GetNode (btreePtr, insertNodeNum, &nodeRec);
		if( err == noErr )
		{
			err = TrySimpleReplace (btreePtr, nodeRec.buffer, iterator, record, recordLen, &recordFit);
			M_ExitOnError (err);

			if (recordFit)
			{
				err = UpdateNode (btreePtr, &nodeRec);
				M_ExitOnError (err);

				++btreePtr->numValidHints;

				goto Success;
			}
			else
			{
				(void) BTInvalidateHint( iterator );
			}
			
			err = ReleaseNode (btreePtr, &nodeRec);
			M_ExitOnError (err);
		}
		else
		{
			(void) BTInvalidateHint( iterator );
		}
	}


	////////////////////////////// Get A Clue ///////////////////////////////////

	err = SearchTree (btreePtr, &iterator->key, treePathTable, &insertNodeNum, &nodeRec, &index);
	M_ExitOnError (err);					// record must exit for Replace

	// optimization - if simple replace will work then don't extend btree
	// �� if we tried this before, and failed because it wouldn't fit then we shouldn't try this again...

	err = TrySimpleReplace (btreePtr, nodeRec.buffer, iterator, record, recordLen, &recordFit);
	M_ExitOnError (err);

	if (recordFit)
	{
		err = UpdateNode (btreePtr, &nodeRec);
		M_ExitOnError (err);

		goto Success;
	}


	//////////////////////////// Make Some Room /////////////////////////////////

	nodesNeeded =  btreePtr->treeDepth + 1 - btreePtr->freeNodes;	//�� math limit
	if (nodesNeeded > 0)
	{
		nodesNeeded += btreePtr->totalNodes;
		if (nodesNeeded > CalcMapBits (btreePtr))	// we'll need to add a map node too!
			++nodesNeeded;

		err = ExtendBTree (btreePtr, nodesNeeded);
		M_ExitOnError (err);
	}


	DeleteRecord (btreePtr, nodeRec.buffer, index);	// delete existing key/record

	err = InsertTree (btreePtr, treePathTable, &iterator->key, record->bufferAddress,
					  recordLen, &nodeRec, index, 1, kReplaceRecord, &insertNodeNum);
	M_ExitOnError (err);

Success:
	// create hint
	iterator->hint.writeCount 	= btreePtr->writeCount;		//�� currently unused
	iterator->hint.nodeNum		= insertNodeNum;
	iterator->hint.index		= 0;						// unused
	iterator->hint.reserved1	= 0;
	iterator->hint.reserved2	= 0;

	LogEndTime(kTraceReplaceBTreeRecord, noErr);

	return noErr;


	////////////////////////////// Error Exit ///////////////////////////////////

ErrorExit:

	(void) ReleaseNode (btreePtr, &nodeRec);

	iterator->hint.writeCount 	= 0;
	iterator->hint.nodeNum		= 0;
	iterator->hint.index		= 0;
	iterator->hint.reserved1	= 0;
	iterator->hint.reserved2	= 0;


	LogEndTime(kTraceReplaceBTreeRecord, err);

	return err;
}



//////////////////////////////// BTDeleteRecord /////////////////////////////////

OSStatus	BTDeleteRecord		(FCB						*filePtr,
								 BTreeIterator				*iterator )
{
	OSStatus				err;
	BTreeControlBlockPtr	btreePtr;
	TreePathTable			treePathTable;
	BlockDescriptor			nodeRec;
	UInt32					nodeNum;
	UInt16					index;

	LogStartTime(kTraceDeleteBTreeRecord);

	////////////////////////// Priliminary Checks ///////////////////////////////

	nodeRec.buffer = nil;					// so we can call ReleaseNode

	M_ReturnErrorIf (filePtr == nil, 	paramErr);
	M_ReturnErrorIf (iterator == nil,	paramErr);

	btreePtr = (BTreeControlBlockPtr) filePtr->fcbBTCBPtr;
	if (btreePtr == nil)
	{
		err = fsBTInvalidFileErr;
		goto ErrorExit;
	}

  #if TARGET_OS_RHAPSODY
	RequireFileLock(btreePtr->fileRefNum, false);
  #endif

#if SupportsKeyDescriptors
	if (btreePtr->keyDescPtr != nil)
	{
		err = CheckKey (&iterator->key, btreePtr->keyDescPtr, btreePtr->maxKeyLength);
		M_ExitOnError (err);
	}
#endif

	/////////////////////////////// Find Key ////////////////////////////////////

	//�� check hint for simple delete case (index > 0, numRecords > 2)

	err = SearchTree (btreePtr, &iterator->key, treePathTable, &nodeNum, &nodeRec, &index);
	M_ExitOnError (err);					// record must exit for Delete


	///////////////////////////// Delete Record /////////////////////////////////

	err = DeleteTree (btreePtr, treePathTable, &nodeRec, index, 1);
	M_ExitOnError (err);

	--btreePtr->leafRecords;
	M_BTreeHeaderDirty (btreePtr);

	iterator->hint.nodeNum	= 0;

	LogEndTime(kTraceDeleteBTreeRecord, noErr);

	return noErr;

	////////////////////////////// Error Exit ///////////////////////////////////

ErrorExit:
	(void) ReleaseNode (btreePtr, &nodeRec);

	LogEndTime(kTraceDeleteBTreeRecord, err);

	return	err;
}



OSStatus	BTGetInformation	(FCB					*filePtr,
								 UInt16					 version,
								 BTreeInfoRec			*info )
{
#pragma unused (version)

	BTreeControlBlockPtr	btreePtr;


	M_ReturnErrorIf (filePtr == nil, 	paramErr);

	btreePtr = (BTreeControlBlockPtr) filePtr->fcbBTCBPtr;

  #if TARGET_OS_RHAPSODY
	RequireFileLock(btreePtr->fileRefNum, true);
  #endif

	M_ReturnErrorIf (btreePtr == nil,	fsBTInvalidFileErr);
	M_ReturnErrorIf (info == nil,		paramErr);

	//�� check version?

	info->nodeSize		= btreePtr->nodeSize;
	info->maxKeyLength	= btreePtr->maxKeyLength;
	info->treeDepth		= btreePtr->treeDepth;
	info->numRecords	= btreePtr->leafRecords;
	info->numNodes		= btreePtr->totalNodes;
	info->numFreeNodes	= btreePtr->freeNodes;
	info->keyDescriptor	= btreePtr->keyDescPtr;	//�� this won't do at all...
	info->reserved		= 0;

	if (btreePtr->keyDescPtr == nil)
		info->keyDescLength	= 0;
	else
		info->keyDescLength	= (UInt32) btreePtr->keyDescPtr->length;

	return noErr;
}



/*-------------------------------------------------------------------------------
Routine:	BTFlushPath	-	Flush BTreeControlBlock to Header Node.

Function:	Brief_description_of_the_function_and_any_side_effects


Input:		pathPtr		- pointer to path control block for B*Tree file to flush

Output:		none

Result:		noErr		- success
			!= noErr	- failure
-------------------------------------------------------------------------------*/

OSStatus	BTFlushPath				(FCB					*filePtr)
{
	OSStatus				err;
	BTreeControlBlockPtr	btreePtr;


	LogStartTime(kTraceFlushBTree);

	M_ReturnErrorIf (filePtr == nil, 	paramErr);

	btreePtr = (BTreeControlBlockPtr) filePtr->fcbBTCBPtr;

	M_ReturnErrorIf (btreePtr == nil,	fsBTInvalidFileErr);

  #if TARGET_OS_RHAPSODY
	RequireFileLock(btreePtr->fileRefNum, false);
  #endif

	err = UpdateHeader (btreePtr);

	LogEndTime(kTraceFlushBTree, err);

	return	err;
}



/*-------------------------------------------------------------------------------
Routine:	BTInvalidateHint	-	Invalidates the hint within a BTreeInterator.

Function:	Invalidates the hint within a BTreeInterator.


Input:		iterator	- pointer to BTreeIterator

Output:		iterator	- iterator with the hint.nodeNum cleared

Result:		noErr			- success
			paramErr	- iterator == nil
-------------------------------------------------------------------------------*/


OSStatus	BTInvalidateHint	(BTreeIterator				*iterator )
{
	if (iterator == nil)
		return	paramErr;

	iterator->hint.nodeNum = 0;

	return	noErr;
}


