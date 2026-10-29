/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/**************************************************************************************
 * Fixed-point MP3 decoder
 * Jon Recker (jrecker@real.com), Ken Cooke (kenc@real.com)
 * June 2003
 *
 * buffers.c - allocation and freeing of internal MP3 decoder buffers
 *
 * All memory allocation for the codec is done in this file, so if you don't want 
 *  to use other the default system malloc() and free() for heap management this is 
 *  the only file you'll need to change.
 *
 * Modified: Support multiple decoder instances (non-static buffers)
 **************************************************************************************/

#include "coder.h"
#include <stdlib.h>
#include <string.h>

/**************************************************************************************
 * Function:    AllocateBuffers
 *
 * Description: allocate all the memory needed for the MP3 decoder
 *
 * Inputs:      none
 *
 * Outputs:     none
 *
 * Return:      pointer to MP3DecInfo structure (initialized with pointers to all 
 *                the internal buffers needed for decoding, all other members of 
 *                MP3DecInfo structure set to 0)
 *
 * Notes:       if one or more mallocs fail, function frees any buffers already
 *                allocated before returning
 **************************************************************************************/
MP3DecInfo *AllocateBuffers(void)
{
	MP3DecInfo *mp3DecInfo;
	FrameHeader *fh;
	SideInfo *si;
	ScaleFactorInfo *sfi;
	HuffmanInfo *hi;
	DequantInfo *di;
	IMDCTInfo *mi;
	SubbandInfo *sbi;

	/* Allocate main decoder info structure */
	mp3DecInfo = (MP3DecInfo *)malloc(sizeof(MP3DecInfo));
	if (!mp3DecInfo)
		return 0;

	memset(mp3DecInfo, 0, sizeof(MP3DecInfo));

	/* Allocate internal buffers */
	fh  = (FrameHeader *)malloc(sizeof(FrameHeader));
	si  = (SideInfo *)malloc(sizeof(SideInfo));
	sfi = (ScaleFactorInfo *)malloc(sizeof(ScaleFactorInfo));
	hi  = (HuffmanInfo *)malloc(sizeof(HuffmanInfo));
	di  = (DequantInfo *)malloc(sizeof(DequantInfo));
	mi  = (IMDCTInfo *)malloc(sizeof(IMDCTInfo));
	sbi = (SubbandInfo *)malloc(sizeof(SubbandInfo));

	/* Check if any allocation failed, free all and return NULL */
	if (!fh || !si || !sfi || !hi || !di || !mi || !sbi) {
		free(fh);
		free(si);
		free(sfi);
		free(hi);
		free(di);
		free(mi);
		free(sbi);
		free(mp3DecInfo);
		return 0;
	}

	mp3DecInfo->FrameHeaderPS =     (void *)fh;
	mp3DecInfo->SideInfoPS =        (void *)si;
	mp3DecInfo->ScaleFactorInfoPS = (void *)sfi;
	mp3DecInfo->HuffmanInfoPS =     (void *)hi;
	mp3DecInfo->DequantInfoPS =     (void *)di;
	mp3DecInfo->IMDCTInfoPS =       (void *)mi;
	mp3DecInfo->SubbandInfoPS =     (void *)sbi;

	/* important to do this - DSP primitives assume a bunch of state variables are 0 on first use */
	memset(fh,  0, sizeof(FrameHeader));
	memset(si,  0, sizeof(SideInfo));
	memset(sfi, 0, sizeof(ScaleFactorInfo));
	memset(hi,  0, sizeof(HuffmanInfo));
	memset(di,  0, sizeof(DequantInfo));
	memset(mi,  0, sizeof(IMDCTInfo));
	memset(sbi, 0, sizeof(SubbandInfo));

	/* mainBuf is part of MP3DecInfo, clear it */
	memset(mp3DecInfo->mainBuf, 0, sizeof(mp3DecInfo->mainBuf));

	return mp3DecInfo;
}

/**************************************************************************************
 * Function:    FreeBuffers
 *
 * Description: free all the memory allocated for the MP3 decoder
 *
 * Inputs:      pointer to MP3DecInfo structure
 *
 * Outputs:     none
 *
 * Return:      none
 **************************************************************************************/
void FreeBuffers(MP3DecInfo *mp3DecInfo)
{
	if (!mp3DecInfo)
		return;

	/* Free internal buffers */
	if (mp3DecInfo->FrameHeaderPS)
		free(mp3DecInfo->FrameHeaderPS);
	if (mp3DecInfo->SideInfoPS)
		free(mp3DecInfo->SideInfoPS);
	if (mp3DecInfo->ScaleFactorInfoPS)
		free(mp3DecInfo->ScaleFactorInfoPS);
	if (mp3DecInfo->HuffmanInfoPS)
		free(mp3DecInfo->HuffmanInfoPS);
	if (mp3DecInfo->DequantInfoPS)
		free(mp3DecInfo->DequantInfoPS);
	if (mp3DecInfo->IMDCTInfoPS)
		free(mp3DecInfo->IMDCTInfoPS);
	if (mp3DecInfo->SubbandInfoPS)
		free(mp3DecInfo->SubbandInfoPS);

	/* Free main decoder info structure */
	free(mp3DecInfo);
}
