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


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <MUI/NListview_mcc.h>
#include "Pathy_mcc.h"

struct Library *MUIMasterBase = NULL;

#ifndef MAKE_ID
# define MAKE_ID(a,b,c,d)	\
	((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#define ColorObject( R, G, B )	\
	ColorfieldObject, \
		MUIA_Colorfield_Red,	R, \
		MUIA_Colorfield_Green,	G, \
		MUIA_Colorfield_Blue,	B, End

int main(int argc,char *argv[])
{
	Object * app, * window, * pathy, * path, * page;

	if (!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME,MUIMASTER_VMIN)))
	{
		PutStr("Cannot open " MUIMASTER_NAME "\n");
		return 10;
	}

	app = ApplicationObject,
		MUIA_Application_Title      , "Pathy",
		MUIA_Application_Version    , "$VER: Pathy 1.0 (02.07.2006)",
		MUIA_Application_Copyright  , "©2006, Diego Casorran",
		MUIA_Application_Author     , "Diego Casorran",
		MUIA_Application_Description, "Pathy.mcc demostration.",
		MUIA_Application_Base       , "PATHY",
		SubWindow, window = WindowObject,
			WindowContents, VGroup,
				Child, NListviewObject,
					MUIA_NListview_NList, pathy = PathyObject,
						MUIA_Dirlist_Directory, "ram:",
					End,
				End,
				Child, HGroup,
					Child, path = StringObject, StringFrame,
						MUIA_String_Contents, "SYS:",
					End,
					Child, page = PageGroup,
						MUIA_Weight, 0,
						MUIA_FixWidthTxt, "XYZ",
						Child, ColorObject( 0xFFFFFFFF,0,0),
						Child, ColorObject( 0xFFFFFFFF,0xFFFFFFFF,0),
						Child, ColorObject( 0,0x66666666,0),
					End,
				End,
			End,
		End,
	End;

	if( ! app ) {
		PutStr("Failed to create Application.");
		goto done;
	}

	DoMethod(window,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,
		app,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);
	
	DoMethod(path, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		pathy, 3, MUIM_Set, MUIA_Dirlist_Directory, MUIV_TriggerValue);
	
	DoMethod(pathy, MUIM_Notify, MUIA_Dirlist_Status, MUIV_EveryTime,
		page, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);
	
	set(window,MUIA_Window_Open,TRUE);

	{
		ULONG sigs = 0;

		while (DoMethod(app,MUIM_Application_NewInput,&sigs) != MUIV_Application_ReturnID_Quit)
		{
			if (sigs)
			{
				sigs = Wait(sigs | SIGBREAKF_CTRL_C);
				if (sigs & SIGBREAKF_CTRL_C) break;
			}
		}
	}

	set(window,MUIA_Window_Open,FALSE);


done:
	if (app)
		MUI_DisposeObject(app);
	if (MUIMasterBase)
		CloseLibrary(MUIMasterBase);
	
	return(0);
}

