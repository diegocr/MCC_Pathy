/* ***** BEGIN LICENSE BLOCK *****
 * Version: BSD License
 * 
 * Copyright (c) 2006, Diego Casorran <dcasorran@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 ** Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  
 ** Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ***** END LICENSE BLOCK ***** */

/**
 * $Id: Pathy_mcc.h,v 0.1 2006/07/02 02:10:20 diegocr Exp $
 */

#ifndef PATHY_MCC_H
#define PATHY_MCC_H

#define MUIC_Pathy	"Pathy.mcc"
#define PathyObject	MUI_NewObject(MUIC_Pathy

#define TAGBASE_PATHY	(TAG_USER | (4444 << 16))
#define TBPATHY(num)	(TAGBASE_PATHY + (num))

#define MUIA_Pathy_AcceptPattern	TBPATHY( 1)	/* [ISG.] STRPTR */
#define MUIA_Pathy_RejectPattern	TBPATHY( 2)	/* [ISG.] STRPTR */
#define MUIA_Pathy_NumBytes		TBPATHY( 3)	/* [..G.] struct TagItem * */
#define MUIA_Pathy_DateTimeFormat	TBPATHY( 4)	/* [ISG.] UBYTE */
#define MUIA_Pathy_DateTimeFlags	TBPATHY( 5)	/* [ISG.] UBYTE */
#define MUIA_Pathy_WindowSleep		TBPATHY( 6)	/* [ISG.] BOOL */

#endif /* PATHY_MCC_H */
