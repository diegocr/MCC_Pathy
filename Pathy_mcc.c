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


/**:ts=4
 * Pathy.mcc 1.0, a cloned Dirlist.mui custom class with extra features.
 * (c)2006 Diego Casorran, all right reserved
 ****************************************************************************
 * Extra Features
 * - Suppport 64 bit integers (real)
 * - Avility to resolve softlink to show the real bytes/datestamp/etc
 * - uses ExAll() to scan whole dirs
 * - do not uses a subtask...(erm, feature or bug?, this custom class need
 *   5s to load 3000 files, while dopus for exmaple needs 34s, though)
 * 
 ****************************************************************************
 * 
 * MUI Images:
 * 		6:22	Directory
 * 		6:23	HardDisk
 * 		6:24	Device/Disk
 * 		6:25	RAM Chip
 * 		6:26	Volume
 * 		6:28	Network
 * 		6:29	Assign
 */
#include <proto/exec.h>
#include <proto/utility.h>
#include <MUI/NList_mcc.h>

#include "debug.h"
#include "util.h"
#include "i64_inline.h"
#include "Pathy_mcc.h"

/***************************************************************************/

struct ClassData
{
	ULONG MagicID;
	#define magicid	0x9F111147
	
	APTR mempool;
	
	/* cloned DirlistObject behaviour */
	
	struct Hook construct_hook;
	struct Hook destruct_hook;
	struct Hook display_hook;
	struct Hook compare_hook;
	struct Hook *filterhook;
	
	STRPTR acceptpattern;
	STRPTR rejectpattern;
	
	STRPTR directory;
	
	bigint numbytes;
	ULONG numdrawers;
	ULONG numfiles;
	
	BOOL drawersonly;
	BOOL filesonly;
	BOOL filterdrawers;
	BOOL multiseldirs;
	BOOL rejecticons;
	BOOL sorthighlow;
	
	BYTE sortdirs;
	BYTE sorttype;
	BYTE status;
	
	UBYTE size_string[40];
	UBYTE date_string[20];
	UBYTE time_string[20];
	UBYTE prot_string[8];
	
	STRPTR path;
	
	
	/* This Custom Class new features */
	
	UBYTE name_string[120];
	
	ULONG numsoftlinks;
	ULONG numlinkdirs;
	
	BOOL AcceptPatternIsMine;
	BOOL RejectPatternIsMine;
	BOOL WindowSleep;
	
	/* DateTime fields used on NList_DisplayHook2 */
	UBYTE	dat_Format;		/* controls appearance of dat_StrDate */
	UBYTE	dat_Flags;		/* see BITDEF's below */
	
};

#define REPLACEMENT 0
#if REPLACEMENT
# define CLASS		"Dirlist.mui"
#else
# define CLASS		"Pathy.mcc"
#endif
#define VERSION		1
#define REVISION	0
#define BUILD_DATE	"02.07.2006"
#define SUPERCLASS	MUIC_NList
#define INSTDATA	ClassData

#include "mccheader.c"

/***************************************************************************/
/* private methods */

#define TAGBASE_PATHYPRI		(TAG_USER | (4444 << 16) | 0x10000099)
#define MUIM_PATHY_CHANGEDIR	TAGBASE_PATHYPRI + 1

#define MUIA_ClearDirectoryData	TAGBASE_PATHYPRI + 10

#if MUIMASTER_VLATEST < 20
# define MUIV_Dirlist_SortType_Comment	3
# define MUIV_Dirlist_SortType_Flags	4
# define MUIV_Dirlist_SortType_Type		5
# define MUIV_Dirlist_SortType_Used		6
# define MUIV_Dirlist_SortType_Count	7
#endif

#define PushMethod( obj, numargs, args... )	\
	DoMethod(_app(obj), MUIM_Application_PushMethod, (obj),(numargs), args )
#define PushSet( obj, numargs, args... ) \
	PushMethod((obj), ((numargs)+1), MUIM_Set, args )

/***************************************************************************/

#if 0
static struct ExAllData * FibToExAllData( struct FileInfoBlock * fib )
{
	static struct ExAllData ead;
	
	ead.ed_Name    = fib->fib_FileName;
	ead.ed_Type    = fib->fib_DirEntryType;
	ead.ed_Size    = fib->fib_Size;
	ead.ed_Prot    = fib->fib_Protection;
	ead.ed_Days    = fib->fib_Date.ds_Days;
	ead.ed_Mins    = fib->fib_Date.ds_Minute;
	ead.ed_Ticks   = fib->fib_Date.ds_Tick;
	ead.ed_Comment = fib->fib_Comment;
	
	return (struct ExAllData *) &ead;
}

static struct FileInfoBlock * ExAllDataToFib( struct ExAllData * ead )
{
	static struct FileInfoBlock fib;
	
	fib.fib_FileName       = ead->ed_Name;
	fib.fib_DirEntryType   = ead->ed_Type;
	fib.fib_Size           = ead->ed_Size;
	fib.fib_Protection     = ead->ed_Prot;
	fib.fib_Date.ds_Days   = ead->ed_Days;
	fib.fib_Date.ds_Minute = ead->ed_Mins;
	fib.fib_Date.ds_Tick   = ead->ed_Ticks;
	fib.fib_Comment        = ead->ed_Comment;
	
	return (struct FileInfoBlock *) &fib;
}
#endif

/***************************************************************************/
//------------------------------------ NList related helpful functions -----

struct NList_Entry
{
	/* My ExAllData "emulation" */
    struct ExAllData *ed_Next;
    STRPTR            ed_Name;
    LONG              ed_Type;
    ULONG             ed_Size;
    ULONG             ed_Prot;
    ULONG             ed_Days;
    ULONG             ed_Mins;
    ULONG             ed_Ticks;
    STRPTR            ed_Comment;
    UWORD             ed_OwnerUID;
    UWORD             ed_OwnerGID;
    UBYTE             ed_NameA[108];
    UBYTE             ed_CommentA[80];
    ULONG             ed_UserData;
};

typedef enum
{
	NAME,
	SIZE,
	DATE,
	TIME,
	PROT,
	COMM
} NLEntry_Type;

//------------------------------------ Convert Dirlist type to Nlist Col ---

INLINE long GetSortTypeToCol( struct ClassData *data )
{
	switch( data->sorttype )
	{
		default:
		case MUIV_Dirlist_SortType_Name:
			break;
		
		case MUIV_Dirlist_SortType_Date:
			return DATE;
		
		case MUIV_Dirlist_SortType_Size:
			return SIZE;
		
		case MUIV_Dirlist_SortType_Comment:
			return COMM;
		
/*		case MUIV_Dirlist_SortType_Flags:
		case MUIV_Dirlist_SortType_Type:
		case MUIV_Dirlist_SortType_Used:
		case MUIV_Dirlist_SortType_Count:
		TODO */
	}
	
	return NAME;
}

//---------------------------------------- Get SoftLink file attributes ----

INLINE VOID resolve_softlink( struct ClassData *data, struct NList_Entry ** ead )
{
	struct DevProc * dvp = NULL;
	UBYTE path[4096];
	
	path[0] = '\0';
	AddPart( path, data->directory, sizeof(path)-1);
	AddPart( path,(*ead)->ed_NameA, sizeof(path)-1);
	
	if((dvp = GetDeviceProc( path, NULL)) != NULL)
	{
		DBG_POINTER(dvp);
		
		if((long)ReadLink(dvp->dvp_Port,dvp->dvp_Lock,path,path,sizeof(path)-1) > 0)
		{
			BPTR lock;
			
			if((lock = Lock( path, SHARED_LOCK)))
			{
				struct FileInfoBlock fib;
				
				if(Examine(lock, &fib))
				{
					(*ead)->ed_Size    = fib.fib_Size;
					(*ead)->ed_Prot    = fib.fib_Protection;
					(*ead)->ed_Days    = fib.fib_Date.ds_Days;
					(*ead)->ed_Mins    = fib.fib_Date.ds_Minute;
					(*ead)->ed_Ticks   = fib.fib_Date.ds_Tick;
				}
				
				UnLock(lock);
			}
		}
		
		FreeDeviceProc(dvp);
	}
}

//---------------------------------------------- NList Construct Hook ------

HOOKPROTONO( ConstructFunc, APTR, struct NList_ConstructMessage * msg )
{
	struct ClassData *data = hook->h_Data;
	struct NList_Entry * nle, * entry = (struct NList_Entry *) msg->entry;
	
	if((nle = AllocPooled( data->mempool, sizeof(*nle))))
	{
		*nle = *entry;
		
		memcpy( nle->ed_NameA, entry->ed_Name, sizeof(nle->ed_NameA)-1);
		
		nle->ed_CommentA[0] = '\0';
		if(entry->ed_Comment && *(entry->ed_Comment))
			memcpy( nle->ed_CommentA, entry->ed_Comment, sizeof(nle->ed_CommentA)-1);
		
		nle->ed_Name    = nle->ed_NameA;
		nle->ed_Comment = nle->ed_CommentA;
		
		if(EAD_IS_LINK(nle))
			resolve_softlink( data, &nle );
	}
	return (APTR) nle;
}
MakeStaticHook( ConstructHook, ConstructFunc );

//---------------------------------------------- NList Destruct Hook -------

HOOKPROTONO( DestructFunc, VOID, struct NList_DestructMessage * msg )
{
	struct ClassData *data = hook->h_Data;
	
	if(!data || data->MagicID != magicid)
	{
		DBG(" **** WARNING: wrong DATA !\n");
		return;
	}
	
	if(data->mempool == NULL)
	{
		DBG(" **** WARNING: pool isnt valid ! +--------\n");
	}
	
	if(data->mempool != NULL)
		FreePooled(data->mempool, msg->entry, sizeof(struct NList_Entry));
}
MakeStaticHook( DestructHook, DestructFunc );

//---------------------------------------------- NList Display Hook --------

HOOKPROTONO( DisplayFunc, long, struct NList_DisplayMessage * msg )
{
	struct ClassData *data = hook->h_Data;
	struct NList_Entry * entry = (struct NList_Entry *) msg->entry;
	struct DateTime dt;
	
	if(!data || data->MagicID != magicid)
	{
		DBG(" **** WARNING: wrong DATA !\n");
		return 0;
	}
	
	/* MUI: name | size | Date  | Time  | Protection | Comment */
	
	if( entry == NULL )
	{
		msg->strings[NAME] = "Name";
		msg->strings[SIZE] = "Size";
		msg->strings[DATE] = "Date";
		msg->strings[TIME] = "Time";
		msg->strings[PROT] = "Protection";
		msg->strings[COMM] = "Comment";
		
		return 0;
	}
	
	if(EAD_IS_LINK(entry))
	{
		SNPrintf(data->name_string, sizeof(data->name_string)-1,
			"\033u%s", entry->ed_Name );
		
		msg->strings[NAME] = data->name_string;
	}
	else
	{
		msg->strings[NAME] = entry->ed_Name;
	}
	
	if (EAD_IS_DRAWER(entry))
	{
		msg->strings[SIZE] = "\033r\033I[6:22]";
	}
	else if(EAD_IS_LINKDIR(entry)||(EAD_IS_SOFTLINK(entry) && !entry->ed_Size))
	{
		msg->strings[SIZE] = "\033u\033r\033I[6:22]";
	}
	else
	{
		SNPrintf(data->size_string, sizeof(data->size_string), 
			"\033r%lu", (ULONG) entry->ed_Size );
		
		msg->strings[SIZE] = data->size_string;
	}
	
	dt.dat_Stamp.ds_Days   = entry->ed_Days;
	dt.dat_Stamp.ds_Minute = entry->ed_Mins;
	dt.dat_Stamp.ds_Tick   = entry->ed_Ticks;
	
	dt.dat_Format  = data->dat_Format;
	dt.dat_Flags   = data->dat_Flags;
	dt.dat_StrDay  = NULL;
	dt.dat_StrDate = data->date_string;
	dt.dat_StrTime = data->time_string;
	
	DateToStr(&dt);
	
	msg->strings[DATE] = data->date_string;
	msg->strings[TIME] = data->time_string;
	
	data->prot_string[0] = (entry->ed_Prot & FIBF_SCRIPT)  ? 's' : '-';
	data->prot_string[1] = (entry->ed_Prot & FIBF_PURE)    ? 'p' : '-';
	data->prot_string[2] = (entry->ed_Prot & FIBF_ARCHIVE) ? 'a' : '-';
	data->prot_string[3] = (entry->ed_Prot & FIBF_READ)    ? '-' : 'r';
	data->prot_string[4] = (entry->ed_Prot & FIBF_WRITE)   ? '-' : 'w';
	data->prot_string[5] = (entry->ed_Prot & FIBF_EXECUTE) ? '-' : 'e';
	data->prot_string[6] = (entry->ed_Prot & FIBF_DELETE ) ? '-' : 'd';
	data->prot_string[7] = '\0';
	
	msg->strings[PROT] = data->prot_string;
	msg->strings[COMM] = entry->ed_Comment;
	
	return 0;
}
MakeStaticHook( DisplayHook, DisplayFunc );

//---------------------------------------------- NList Compare Hook --------

STATIC LONG ead_cmp2col( struct ClassData *data,
	struct NList_Entry *ead1, struct NList_Entry *ead2, ULONG column )
{
	switch (column)
	{
		default:
		case NAME:
			
#if 0
			if(EAD_IS_SOMEDRAWER(ead1) && EAD_IS_SOMEDRAWER(ead2))
			{
				switch(data->sortdirs)
				{
					case MUIV_Dirlist_SortDirs_First:
						return -1;
					
					case MUIV_Dirlist_SortDirs_Last:
						return 1;
					
					case MUIV_Dirlist_SortDirs_Mix:
					default:
						break;
				}
			}
#endif
			return Stricmp( ead1->ed_Name, ead2->ed_Name );
			
		case SIZE:
			return ead1->ed_Size - ead2->ed_Size;
			
		case DATE:
		case TIME:
		{
			struct DateStamp ds1, ds2;
			
			ds1.ds_Days   = ead1->ed_Days;
			ds1.ds_Minute = ead1->ed_Mins;
			ds1.ds_Tick   = ead1->ed_Ticks;
			ds2.ds_Days   = ead2->ed_Days;
			ds2.ds_Minute = ead2->ed_Mins;
			ds2.ds_Tick   = ead2->ed_Ticks;
			
			return -CompareDates(&ds1,&ds2);
		}
		case PROT: return ead1->ed_Prot - ead2->ed_Prot; // hmmm...
		case COMM: return Stricmp( ead1->ed_Comment, ead2->ed_Comment );
	}
}

HOOKPROTONO( CompareFunc, LONG, struct NList_CompareMessage * msg )
{
	struct ClassData *data = hook->h_Data;
	struct NList_Entry *ead1, *ead2;
	ULONG col1, col2;
	LONG result;

	ead1 = (struct NList_Entry *)msg->entry1;
	ead2 = (struct NList_Entry *)msg->entry2;
	col1 = msg->sort_type & MUIV_NList_TitleMark_ColMask;
	col2 = msg->sort_type2 & MUIV_NList_TitleMark2_ColMask;

	if (msg->sort_type == (LONG)MUIV_NList_SortType_None) return 0;

	if (msg->sort_type & MUIV_NList_TitleMark_TypeMask) {
	    result = ead_cmp2col(data, ead2, ead1, col1);
	} else {
	    result = ead_cmp2col(data, ead1, ead2, col1);
	}

	if (result != 0 || col1 == col2) return result;

	if (msg->sort_type2 & MUIV_NList_TitleMark2_TypeMask) {
	    result = ead_cmp2col(data, ead2, ead1, col2);
	} else {
	    result = ead_cmp2col(data, ead1, ead2, col2);
	}

	return result;
}
MakeStaticHook( CompareHook, CompareFunc );

/***************************************************************************/

INLINE ULONG pathy_new(struct IClass *cl, Object *obj, struct opSet *msg)
{
	struct ClassData *data;
	
	obj = (Object *)DoSuperNew(cl,obj,
		MUIA_NList_Format, (ULONG)",,,,,",
	//	MUIA_NList_Title, TRUE,
	TAG_MORE, msg->ops_AttrList);
	
	if(!obj || !(data = INST_DATA(cl,obj)))
		return 0;
	
	bzero( data, sizeof(struct ClassData));
	
	data->MagicID		= magicid;
	data->dat_Format	= FORMAT_DEF;	/* use default format, or FORMAT_DOS */
	data->dat_Flags		= DTF_SUBST;	/* substitute Today, Tomorrow, etc. */
	data->sorttype		= MUIV_Dirlist_SortType_Name;
	data->sortdirs		= MUIV_Dirlist_SortDirs_First;
	
	data->mempool = CreatePool(MEMF_PUBLIC|MEMF_CLEAR, sizeof(struct NList_Entry)*20, sizeof(struct NList_Entry)*8);
	if(!data->mempool) {
	//	CoerceMethod(cl, obj, OM_DISPOSE);
		return 0;
	}
	
	InitHook( &(data->construct_hook), ConstructHook, data );
	InitHook( &(data->destruct_hook),  DestructHook,  data );
	InitHook( &(data->display_hook),   DisplayHook,   data );
	InitHook( &(data->compare_hook),   CompareHook,   data );
	
	SetAttrs(obj,
		MUIA_NList_ConstructHook2,  (ULONG) &data->construct_hook,
		MUIA_NList_DestructHook2,   (ULONG) &data->destruct_hook,
		MUIA_NList_DisplayHook2,    (ULONG) &data->display_hook,
		MUIA_NList_CompareHook2,    (ULONG) &data->compare_hook,
	TAG_DONE);
	
	set( obj, MUIA_ClearDirectoryData, TRUE );
	
	// Dirlist__OM_SET(cl, obj, msg);
	SetAttrsA( obj, msg->ops_AttrList );
	
	DoMethod( obj, MUIM_Notify, MUIA_NList_TitleClick,  MUIV_EveryTime, obj, 4, MUIM_NList_Sort3, MUIV_TriggerValue, MUIV_NList_SortTypeAdd_2Values, MUIV_NList_Sort3_SortType_Both);
	DoMethod( obj, MUIM_Notify, MUIA_NList_TitleClick2, MUIV_EveryTime, obj, 4, MUIM_NList_Sort3, MUIV_TriggerValue, MUIV_NList_SortTypeAdd_2Values, MUIV_NList_Sort3_SortType_2);
	DoMethod( obj, MUIM_Notify, MUIA_NList_SortType,    MUIV_EveryTime, obj, 3, MUIM_Set, MUIA_NList_TitleMark,MUIV_TriggerValue);
	DoMethod( obj, MUIM_Notify, MUIA_NList_SortType2,   MUIV_EveryTime, obj, 3, MUIM_Set, MUIA_NList_TitleMark2,MUIV_TriggerValue);
	
	DoMethod( obj, MUIM_Notify, MUIA_NList_DoubleClick,
		TRUE, obj, 1, MUIM_PATHY_CHANGEDIR );
	
	DBG("\n\tCreated new object, obj = 0x%08lx, data = 0x%08lx\n\n", obj, data );
	
	return((ULONG)obj);
}

/***************************************************************************/

INLINE ULONG pathy_dispose(struct IClass *cl,Object *obj,Msg msg)
{
	struct ClassData *data = INST_DATA(cl,obj);
	
	DoSuperMethodA(cl, obj, msg);
	
	if(data->AcceptPatternIsMine && data->acceptpattern)
		FreeVec( data->acceptpattern );
	
	if(data->RejectPatternIsMine && data->rejectpattern)
		FreeVec( data->rejectpattern );
	
	if(data->path)
		FreeVec(data->path);
	
	if(data->mempool)
	{
		DeletePool(data->mempool);
		data->mempool = NULL;
	}
	
	return(TRUE);
}

/***************************************************************************/

#define NUM_OF_BUFS		128

INLINE VOID ReadDirectory( Object *obj, struct ClassData *data)
{
	register struct ExAllControl * eaControl;
	register struct ExAllData    * eaBuffer, * eaData;
	register LONG more, eaBuffSize = NUM_OF_BUFS * sizeof(struct ExAllData);
	register BPTR lock;
	
	if((lock = Lock(data->directory, SHARED_LOCK)))
	{
		struct FileInfoBlock fib;
		BOOL success, isdir;
		
		success = Examine(lock, &fib);
		
		if(success && (isdir = (fib.fib_DirEntryType > 0)))
		{
			success = FALSE;
			
			eaControl = (struct ExAllControl *) AllocDosObject(DOS_EXALLCONTROL, NULL);
			
			if( eaControl != NULL )
			{
				eaControl->eac_LastKey = 0;
				eaControl->eac_MatchString = (UBYTE *) NULL;
				eaControl->eac_MatchFunc = (struct Hook *) NULL;
				
				eaBuffer = (struct ExAllData *) AllocPooled( data->mempool, eaBuffSize );
				
				if( eaBuffer != NULL)
				{
					success = TRUE;
					
					do {
						
						more = ExAll( lock, eaBuffer, eaBuffSize, ED_COMMENT, eaControl);
						
						if( (!more) && (IoErr() != ERROR_NO_MORE_ENTRIES) )
						{
							DBG("ExAll failed abnormally!\n");
							success = FALSE;
							break;
						}
						
						if (eaControl->eac_Entries == 0)
						{
							DBG("no more entries\n");
							continue;
						}
						
						eaData = (struct ExAllData *) eaBuffer;
						
						while( eaData != NULL )
						{
							if(data->filterhook)
							{
								// HEY, how to pass obj to eaControl->eac_MatchFunc !?
								if (!CallHookPkt(data->filterhook, obj, eaData)) continue;
							}
							else if((EAD_IS_DRAWER(eaData) && data->filesonly)
								|| (!EAD_IS_DRAWER(eaData) && data->drawersonly))
							{
								continue;
							}
							else
							{
								if(data->rejecticons)
								{
									WORD len = strlen(eaData->ed_Name);
									
									if (len >= 5)
									{
										if(Stricmp(eaData->ed_Name + len - 5, ".info") == 0) continue;
									}
								}
								
								if (!EAD_IS_DRAWER(eaData) || data->filterdrawers)
								{
									if (data->acceptpattern)
									{
							    		if (!MatchPatternNoCase(data->acceptpattern, eaData->ed_Name)) continue;
									}
									
									if (data->rejectpattern)
									{
							    		if (MatchPatternNoCase(data->rejectpattern, eaData->ed_Name)) continue;
									}
								}
								
								if (EAD_IS_DRAWER(eaData))
								{
									data->numdrawers++;
								}
								else if(EAD_IS_LINKDIR(eaData))
								{
									data->numlinkdirs++;
								}
								else if(EAD_IS_SOFTLINK(eaData))
								{
									data->numsoftlinks++;
								}
								else
								{
									data->numfiles++;
									data->numbytes  = i64_add( data->numbytes, i64_uset((ULONG) eaData->ed_Size ));
								}
								
							//	*((ULONG *) eaData->ed_OwnerUID ) = (ULONG) data;
								
								DoMethod(obj, MUIM_NList_InsertSingle, (ULONG)eaData, 
									/*MUIV_List_Insert_Sorted ); */ MUIV_NList_Insert_Bottom);
							}
							
							eaData = eaData->ed_Next;
						}
						
					} while(more);
					
					FreePooled( data->mempool, eaBuffer, eaBuffSize );
				}
				
				FreeDosObject(DOS_EXALLCONTROL, eaControl);
			}
			
			if(success)
				set(obj, MUIA_Dirlist_Status, MUIV_Dirlist_Status_Valid);
			
			DBG_VALUE(data->numdrawers);
			DBG_VALUE(data->numfiles);
			DBG_VALUE(data->numbytes.hi|data->numbytes.lo);
		}
		
		UnLock(lock);
	}
}


INLINE ULONG pathy_set(struct IClass *cl, Object *obj, Msg msg)
{
	register struct ClassData * data = INST_DATA(cl,obj);
	struct TagItem *tag, *tags = (((struct opSet *)msg)->ops_AttrList);
	BOOL directory_changed = FALSE;
	
	while(( tag = (struct TagItem *) NextTagItem( &tags )))
	{
		ULONG tidata = tag->ti_Data;
		
		DBG("tag->ti_Tag = 0x%08lx,\"%s\"\n", 
			tag->ti_Tag, GetMUIAttributes(tag->ti_Tag));
		
		switch (tag->ti_Tag)
		{
			case MUIA_ClearDirectoryData:
				set(obj, MUIA_Dirlist_Status, MUIV_Dirlist_Status_Invalid );
				data->numdrawers   = 0;
				data->numfiles     = 0;
				data->numbytes     = i64_uset(0);
				data->numsoftlinks = 0;
				data->numlinkdirs  = 0;
				return TRUE;
				
			case MUIA_Dirlist_RejectPattern:
				data->rejectpattern       = (STRPTR) tidata;
				data->RejectPatternIsMine = FALSE;
				break;
			
			case MUIA_Pathy_RejectPattern:
			{
				STRPTR upat = (STRPTR) tidata;
				
				 // ParsePattern() needs at least 2*source+2 bytes buffer
				long slen;
				
				if((slen = strlen(upat) * 2 + 2) > 3)
				{
					data->rejectpattern = AllocVec(slen,MEMF_PUBLIC|MEMF_CLEAR);
					
					if( data->rejectpattern != NULL )
					{
						if(ParsePatternNoCase( upat, data->rejectpattern, slen) == -1)
						{
							DBG("ParsePatternNoCase() failed\n");
							
							FreeVec( data->rejectpattern );
							data->rejectpattern = NULL;
						}
						else data->RejectPatternIsMine = TRUE;
					}
				}
			}	break;
			
			case MUIA_Dirlist_AcceptPattern:
				data->acceptpattern       = (STRPTR) tidata;
				data->AcceptPatternIsMine = FALSE;
				break;
			
			case MUIA_Pathy_AcceptPattern:
			{
				STRPTR upat = (STRPTR) tidata;
				
				 // ParsePattern() needs at least 2*source+2 bytes buffer
				long slen;
				
				if((slen = strlen(upat) * 2 + 2) > 3)
				{
					data->acceptpattern = AllocVec(slen,MEMF_PUBLIC|MEMF_CLEAR);
					
					if( data->acceptpattern != NULL )
					{
						if(ParsePatternNoCase( upat, data->acceptpattern, slen) == -1)
						{
							DBG("ParsePatternNoCase() failed\n");
							
							FreeVec( data->acceptpattern );
							data->acceptpattern = NULL;
						}
						else data->AcceptPatternIsMine = TRUE;
					}
				}
			}	break;
			
			case MUIA_Dirlist_Directory:
				data->directory = (STRPTR)tidata;
				directory_changed = TRUE;
				break;
			
			case MUIA_Dirlist_DrawersOnly:
				data->drawersonly = tidata ? TRUE : FALSE;
				break;
			
			case MUIA_Dirlist_FilesOnly:
				data->filesonly = tidata ? TRUE : FALSE;
				break;
			
			case MUIA_Dirlist_FilterDrawers:
				data->filterdrawers = tidata ? TRUE : FALSE;
				break;
			
			case MUIA_Dirlist_FilterHook:
				data->filterhook = (struct Hook *)tidata;
				break;
			
			case MUIA_Dirlist_MultiSelDirs:
				data->multiseldirs = tidata ? TRUE : FALSE;
				break;
			
			case MUIA_Dirlist_RejectIcons:
				data->rejecticons = tidata ? TRUE : FALSE;
				break;
			
			case MUIA_Dirlist_SortDirs:
				data->sortdirs = tidata ? TRUE : FALSE;
				break;
			
			case MUIA_Dirlist_SortHighLow:
				data->sorthighlow = tidata ? TRUE : FALSE;
				break;
			
			case MUIA_Dirlist_SortType:
				data->sorttype = tidata;
				break;
			
    	    case MUIA_Dirlist_Status:
				data->status = tidata;
				break;
			
			case MUIA_Pathy_DateTimeFormat:
				data->dat_Format = tidata & 0xff;
				break;
			
			case MUIA_Pathy_DateTimeFlags:
				data->dat_Flags = tidata & 0xff;
				break;
			
			case MUIA_Pathy_WindowSleep:
				data->WindowSleep = tidata ? TRUE : FALSE;
				break;
		}
	}
	
	if (directory_changed)
	{
		LONG col = GetSortTypeToCol(data);
		
		DBG("Directory changed...\n");
		
		set( obj, MUIA_NList_Quiet, TRUE );
		
		if(data->status == MUIV_Dirlist_Status_Valid)
		{
			DBG("Status is valid, cleaning...\n");
			
			DoMethod(obj, MUIM_NList_Clear );
			DoMethod(obj, MUIM_NList_Sort3, col, 
				MUIV_NList_SortTypeAdd_2Values, MUIV_NList_Sort3_SortType_Both);
			
			set( obj, MUIA_ClearDirectoryData, TRUE );
		}
		
		if(data->directory)
		{
			DBG("Reading directory \"%s\"...\n", data->directory );
			
			set(obj, MUIA_Dirlist_Status, MUIV_Dirlist_Status_Reading );
			
			if(data->WindowSleep)
				set(_win(obj), MUIA_Window_Sleep, TRUE );
			
			ReadDirectory(obj, data);
			
			DoMethod(obj, MUIM_NList_Sort3, col, 
				MUIV_NList_SortTypeAdd_2Values, MUIV_NList_Sort3_SortType_Both);
			
			if(data->WindowSleep)
				set(_win(obj), MUIA_Window_Sleep, FALSE );
		}
		
		set( obj, MUIA_NList_Quiet, FALSE );
	}
	
	return(DoSuperMethodA(cl, obj, msg));
}

/***************************************************************************/

INLINE ULONG pathy_get(struct IClass *cl, Object *obj, struct opGet * msg)
{
	struct ClassData *data = INST_DATA( cl, obj);
	
	#define STORE *(msg->opg_Storage)
	
	DBG("msg->opg_AttrID = 0x%08lx,\"%s\"\n", 
		msg->opg_AttrID, GetMUIAttributes(msg->opg_AttrID));
	
	switch(msg->opg_AttrID)
	{
		case MUIA_Dirlist_AcceptPattern:
		case MUIA_Pathy_AcceptPattern:
			STORE = (ULONG)data->acceptpattern;
			return 1;
			
		case MUIA_Dirlist_RejectPattern:
		case MUIA_Pathy_RejectPattern:
			STORE = (ULONG)data->rejectpattern;
			return 1;
			
		case MUIA_Dirlist_Directory:
			STORE = (ULONG)data->directory;
			return 1;
			
		case MUIA_Dirlist_DrawersOnly:
			STORE = data->drawersonly;
			return 1;
			
		case MUIA_Dirlist_FilesOnly:
			STORE = data->filesonly;
			return 1;
			
		case MUIA_Dirlist_FilterDrawers:
			STORE = data->filterdrawers;
			return 1;
			
		case MUIA_Dirlist_FilterHook:
			STORE = (ULONG)data->filterhook;
			return 1;
			
		case MUIA_Dirlist_MultiSelDirs:
			STORE = data->multiseldirs;
			return 1;
			
		case MUIA_Pathy_NumBytes:
		{
			struct TagItem ti;
			
			ti.ti_Tag  = data->numbytes.hi;
			ti.ti_Data = data->numbytes.lo;
			
			STORE = (ULONG) &ti;
			
		}	return TRUE;
		
		case MUIA_Dirlist_NumBytes:
		{
			unsigned long long numbytes;
			
			numbytes = (unsigned long long)(data->numbytes.hi|data->numbytes.lo);
			
			STORE = (ULONG) (numbytes & 0xffffffff);
			
		}	return 1;
			
		case MUIA_Dirlist_NumFiles:
			STORE = data->numfiles;
			return 1;
			
		case MUIA_Dirlist_NumDrawers:
			STORE = data->numdrawers;
			return 1;
			
		case MUIA_Dirlist_Path:
			if (data->path)
			{
				FreeVec(data->path);
				data->path = NULL;
			}
			
			STORE = 0;
			
			if (data->status == MUIV_Dirlist_Status_Valid)
			{
				struct NList_Entry * entry = NULL;
				
				DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry);
				
				if (entry)
				{
				    WORD len = strlen(entry->ed_NameA) + strlen(data->directory) + 3;
				    
			    	data->path = AllocVec(len, MEMF_PUBLIC|MEMF_CLEAR);
			    	if (data->path)
			    	{
			    		if(AddPart(data->path, data->directory, len)
						&& AddPart(data->path, entry->ed_NameA, len))
						{
							STORE = (ULONG)data->path;
						}
					}
				}
			}
			return 1;
			
		case MUIA_Dirlist_RejectIcons:
			STORE = data->rejecticons;
			return 1;
			
		case MUIA_Dirlist_SortDirs:
			STORE = data->sortdirs;
			return 1;
			
		case MUIA_Dirlist_SortHighLow:
			STORE = data->sorthighlow;
			return 1;
			
		case MUIA_Dirlist_SortType:
			STORE = data->sorttype;
			return 1;

		case MUIA_Dirlist_Status:
			STORE = data->status;
			return 1;
			
		case MUIA_Pathy_DateTimeFormat:
			STORE = data->dat_Format;
			return 1;
		
		case MUIA_Pathy_DateTimeFlags:
			STORE = data->dat_Flags;
			return 1;
		
		case MUIA_Pathy_WindowSleep:
			STORE = data->WindowSleep;
			break;
	}
	return(DoSuperMethodA(cl, obj, (Msg)msg));
}

/***************************************************************************/

INLINE ULONG pathy_setup(struct IClass *cl, Object *obj, Msg msg)
{
	register struct ClassData * data = INST_DATA(cl,obj);
	
	if(!DoSuperMethodA(cl, obj, msg))
		return FALSE;
	
	return TRUE;
}

/***************************************************************************/

INLINE ULONG pathy_cleanup(struct IClass *cl, Object *obj, Msg msg)
{
	register struct ClassData * data = INST_DATA(cl,obj);
	
	return(DoSuperMethodA(cl, obj, msg));
}

/***************************************************************************/

INLINE ULONG pathy_askminmax(struct IClass *cl, Object *obj,struct MUIP_AskMinMax *msg)
{
	register struct ClassData * data = INST_DATA(cl,obj);
	
	DoSuperMethodA(cl, obj,(Msg) msg);
	
	return 0;
}

/***************************************************************************/

INLINE ULONG pathy_show(struct IClass *cl, Object *obj, Msg msg)
{
	register struct ClassData * data = INST_DATA(cl,obj);
	
	return(DoSuperMethodA(cl, obj, msg));
}

/***************************************************************************/

INLINE ULONG pathy_hide(struct IClass *cl, Object *obj, Msg msg)
{
	register struct ClassData * data = INST_DATA(cl,obj);
	
	return(DoSuperMethodA(cl, obj, msg));
}

/***************************************************************************/

INLINE ULONG pathy_draw(struct IClass *cl, Object *obj,struct MUIP_Draw *msg)
{
	register struct ClassData * data = INST_DATA(cl,obj);
	
	DoSuperMethodA(cl, obj,(Msg) msg);
	
	return 0;
}

/***************************************************************************/

#if 0
INLINE ULONG pathy_(struct IClass *cl, Object *obj, Msg msg)
{
	register struct ClassData * data = INST_DATA(cl,obj);
	
	return(DoSuperMethodA(cl, obj, msg));
}
#endif

/***************************************************************************/

DISPATCHERPROTO(_Dispatcher)
{
	#ifdef DEBUG
	if(msg->MethodID != 0x9D510090)
	{
		DBG("msg->MethodID = 0x%08lx,\"%s\"\n", 
			msg->MethodID, GetMUIMethods(msg->MethodID));
	}
	#endif
	
	switch(msg->MethodID) {
		
		case OM_NEW:     return( pathy_new     (cl, obj, (APTR)msg));
		case OM_DISPOSE: return( pathy_dispose (cl, obj, (APTR)msg));
		case OM_SET:     return( pathy_set     (cl, obj, (APTR)msg));
		case OM_GET:     return( pathy_get     (cl, obj, (APTR)msg));
		
		case MUIM_PATHY_CHANGEDIR:
		{
			
		}	return TRUE;
		
#if 0
		case MUIM_Setup:
			return( pathy_setup     (cl, obj, (APTR)msg));
		
		case MUIM_Cleanup:
			return( pathy_cleanup   (cl, obj, (APTR)msg));
		
		case MUIM_AskMinMax:
			return( pathy_askminmax (cl, obj, (APTR)msg));
		
		case MUIM_Show:
			return( pathy_show      (cl, obj, (APTR)msg));
		
		case MUIM_Hide:
			return( pathy_hide      (cl, obj, (APTR)msg));
		
		case MUIM_Draw:
			return( pathy_draw      (cl, obj, (APTR)msg));
#endif
	}
	return(DoSuperMethodA(cl,obj,msg));
}

