/***************************************************************************
	PROJ:   Nightingale, rev. for v.3.5
	DESC:   Higher-level routines to add music symbols to the score via mouse,
				including deciding where the inserted symbol should go, tracking,
				cancelling, etc.
		AddNewGRSync
		TrkInsSync				TrkInsNote				TrkInsGRSync
		TrkInsGRNote			InsertNote				InsertGRNote
		InsertArpSign			InsertLine				InsertGraphic			
		InsertMusicChar		InsertMODNR				InsertRptEnd
		InsertEnding			InsertMeasure			InsertPseudoMeas
		InsertClef				InsertKeySig			InsertTimeSig
		InsertDynamic			InsertSlur				InsertTempo
		InsertSpace				XLoadInsertSeg
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean AddNewGRSync(Document *, LINK, Point, short, short, short);
static Boolean TrkInsSync(Document *, LINK, Point, short *, short);
static Boolean TrkInsNote(Document *, Point, short *, short);
static Boolean TrkInsGRSync(Document *, LINK, Point, short *, short);
static Boolean TrkInsGRNote(Document *, Point, short *, short);
static LINK FindJIP(LINK,LINK);
static Boolean ChkInsMODNR(LINK,short);
static Boolean InsertHairpin(Document *,Point,LINK,short);

	
/* Utility to add new GRSync before an object found by TimeSearchRight in AddGRNote. */

static Boolean AddNewGRSync(Document *doc,
									LINK baseL,		/* base link */
									Point	pt,
									short sym,
									short voice, short clickStaff
									)
{
	LINK addToGRSyncL;

	/* Look for a GRSync immediately prior to the start link. If it
		exists, and has a note	in the voice, create a new GRSync before
		it; else add to it. */

	if (addToGRSyncL = FindGRSync(doc, pt, TRUE, clickStaff))	{	/* Does it already have note/rest in this voice? */
		if (GRSyncInVoice(addToGRSyncL, voice))
			return TrkInsGRSync(doc, addToGRSyncL, pt, &sym, clickStaff);
			
		HiliteInsertNode(doc, addToGRSyncL, clickStaff, TRUE);
		doc->selStartL = addToGRSyncL;
		doc->selEndL = RightLINK(doc->selStartL);
		if (TrkInsGRNote(doc, pt, &sym, clickStaff))
			return TRUE;

		doc->selStartL = doc->selEndL;								/* Insert cancelled. */
		MEAdjustCaret(doc, TRUE);
		InvalMeasure(baseL, clickStaff);
		return FALSE;
	}
	
	return TrkInsGRSync(doc, baseL, pt, &sym, clickStaff);	/* Create a new GRSync */
}


/* ----------------------------------------------------------------- AddChordSlash -- */
/* Insert a new "chord slash" to the left of <rightL>. More precisely, it inserts a
specialized note--a quarter note on the middle line of the staff, with slash
appearance, zero stemlength, and zero velocity. If necessary, it also adds a new
Sync for the note. */

static Boolean AddChordSlash(Document *, short, short, short);
static Boolean AddChordSlash(
						Document *doc,
						short x,	/* >=0 means new Sync, and this is its horiz. position in pixels, */	
									/* <0  means add note/rest to the existing Sync <doc->selStartL>. */
						short staff,
						short octType
						)
{
	LINK aNoteL; PANOTE aNote; short pitchLev;
	
	pitchLev = 4;
	aNoteL = AddNote(doc, x, qtrNoteInChar, staff, pitchLev, 0, octType);
	if (!aNoteL) return FALSE;
	
	NoteYSTEM(aNoteL) = NoteYD(aNoteL);
	aNote = GetPANOTE(aNoteL);
	aNote->headShape = SLASH_SHAPE;
	aNote->onVelocity = 0;
	
	return TRUE;
}


/* Insert a new Sync with one note to the left of <rightL>. Handles cancelling. */

static Boolean TrkInsSync(Document *doc, LINK rightL, Point pt, short *sym, short staff)
{
	LINK 	qL, octL;
	short octType=-1;
	short pitchLev, acc;

	if (!rightL) {
		MayErrMsg("TrkInsSync: FindSymRight or TimeSearchRight returned NULL.");
		return FALSE;	
	}

	qL = LocateInsertPt(rightL);
	doc->selStartL = doc->selEndL = qL;

	/* Check if the note is being added into an octava'd range; if so, get the octava
	 	type to correct for it when adding the note. */
	if (octL = HasOctavaAcrossPt(doc, pt, staff))
		octType = OctType(octL);

	/* Track the note insertion and if possible, add the new note to the data
	 	structure. */
	if (InsTrackPitch(doc, pt, sym, doc->selStartL, staff, &pitchLev, &acc, octType)) {
		if (symtable[*sym].subtype==2)
			AddChordSlash(doc, pt.h, staff, octType);
		else
			AddNote(doc, pt.h, symtable[*sym].symcode, staff, pitchLev, acc, octType);
		return TRUE;
	}

	if (MeasureTYPE(rightL)) rightL = LeftLINK(rightL);
	InvalMeasure(rightL, staff);									/* Insert cancelled. */
	return FALSE;
}


/* Track the note and add it to the existing sync <doc->selStartL>. */

static Boolean TrkInsNote(Document *doc, Point pt, short *sym, short staff)
{
	short pitchLev, acc; LINK octL;
	short octType=-1;

	if (octL = OctOnStaff(doc->selStartL, staff))
		octType = OctType(octL);

	if (InsTrackPitch(doc, pt, sym, doc->selStartL, staff, &pitchLev, &acc, octType)) {
		if (symtable[*sym].subtype==2)
			AddChordSlash(doc, -99, staff, octType);
		else
			AddNote(doc, -99, symtable[*sym].symcode, staff, pitchLev, acc, octType);
		return TRUE;
	}
	return FALSE;
}


/* Insert a new GRSYNC with one grace note to the right of <rightL>. Handles cancelling. */

static Boolean TrkInsGRSync(Document *doc, LINK rightL, Point pt, short *sym, short	staff)
{
	LINK 	qL, octL;
	short octType=-1;
	short pitchLev, acc;

	if (!rightL) {
		MayErrMsg("TrkInsGRSync: FindSymRight or TimeSearchRight returned NULL.");
		return FALSE;
	}

	qL = LocateInsertPt(rightL);

	doc->selStartL = doc->selEndL = qL;
	if (octL = HasOctavaAcrossPt(doc, pt, staff))
		octType = OctType(octL);

	if (InsTrackPitch(doc, pt, sym, doc->selStartL, staff, &pitchLev, &acc, octType)) {
		AddGRNote(doc, pt.h, symtable[*sym].symcode, staff, pitchLev, acc, octType);
		return TRUE;
	}

	if (MeasureTYPE(rightL)) rightL = LeftLINK(rightL);						
	InvalMeasure(rightL, staff);									/* Insert cancelled. */
	return FALSE;
}


/* Track the grace note and add it to the existing GRSYNC <doc->selStartL>. */

static Boolean TrkInsGRNote(Document *doc, Point pt, short *sym, short staff)
{
	short pitchLev, acc; LINK octL;
	short octType=-1;

	if (octL = OctOnStaff(doc->selStartL, staff))
		octType = OctType(octL);

	if (InsTrackPitch(doc, pt, sym, doc->selStartL, staff, &pitchLev, &acc, octType)) {
		AddGRNote(doc, -99, symtable[*sym].symcode, staff, pitchLev, acc, octType);
		return TRUE;
	}
	return FALSE;
}

/*	--------------------------------------------------------------------- FindJIP -- */
/* Return the first J_IP type object at or to the right of startL in the
range [startL,endL), or NILINK. */

static LINK FindJIP(LINK startL, LINK endL)
{
	LINK pL;
	
	for (pL=startL; pL!=endL; pL=RightLINK(pL)) {
		if (J_IPTYPE(pL)) return pL;
	}
	return NILINK;
}


/*	------------------------------------------------------------------ InsertNote -- */
/* Insert a note (or rest) at a place in the object list suitable for a
mousedown at the given point. Handles feedback and allows cancelling. (This
also handles chord slashes: the insertion user interface distinguishes chord
slashes, but they're really just funny notes.)

A special note on object order...
Several types of objects have links that connect them to Syncs, and, to
simplify things, Nightingale makes assumptions about their order in the
object list. Specifically:
1. After a Beamset with n elements, the next n Syncs with notes/rests in
	the Beamset's voice must be in the Beamset.
2. Tuplets and Octavas have the same requirements as Beamsets.
3.	After a Slur (including ties), the first Sync in the Slur's voice must be
	 the Slur's firstSync.
4.	The first note after a non-hairpin Dynamic on the Dynamic's staff must
	be the Dynamic's firstSyncL. (GetContext will misbehave for any note
	in between.)
Thus, say voice 1 has 3 triplet eighths and voice 2 has 2 ordinary eighths,
both voices are beamed, voice 2's notes are slurred, and there's a dynamic
for voice 1's staff before its first note; then the object list might
look like this (leading digits represent voice numbers):
	1BEAM 2BEAM 1TUPLE DYNAM 2SLUR 1&2SYNC 1SYNC 2SYNC 1SYNC
If a new note is now inserted in either voice to the left of the first sync,
it must go into the object list before the first Beamset:
	NEWSYNC 1BEAM 2BEAM 1TUPLE DYNAM 2SLUR 1&2SYNC 1SYNC 2SYNC 1SYNC
*/
 
/*
 * #1. First, traverse the range quickly to find if there are any J_IP objects
 * to consider. If there are, get the next object of any type to the right, ignoring
 * any J_D symbols. If it exists and is before (& not equal to) addToSyncL, start at
 * this object and traverse to addToSyncL, looking for any J_IP objects. If one exists,
 * insert before it.
 *
 */

Boolean InsertNote(
					Document *doc,
					Point pt,				/* location of mouse click */
					Boolean isGraphic		/* graphic ("by position") or temporal insert */
					)
{
	short			clickStaff,		/* staff user clicked in */
					sym,				/* index of current palChar in symtable[] */
					voice;			/* voice to insert in */
	LINK			addToSyncL,		/* sync to add note/rest to */
					nodeRightL,		/* node to right of insertion pt */
					pLPIL,
					jipL,rightL,pL;
	long			lTime;			/* logical time */
	
	/* Get the staff to insert on (and set doc->currentSystem) */

	clickStaff = FindStaff(doc, pt);
	if (clickStaff==NOONE) return FALSE;

	/* If mouseDown before System's initial (invisible) barline, force it after
		that barline. */
	pt.h = ForceInMeasure(doc, pt.h);
	
	/* Get the symbol and the voice and check the voice's validity on this staff. */
	sym = GetSymTableIndex(palChar);
	voice = USEVOICE(doc, clickStaff);
	if (!AddCheckVoiceTable(doc, clickStaff)) return FALSE;

	/* Determine if the note is to be added to an already existing sync. If so,
		check the sync for validity, set the selection range to equal this sync,
		determine if it is in an octava, track the new note, and add it to the sync. */

	addToSyncL = FindSync(doc, pt, isGraphic, clickStaff);
	if (addToSyncL) {
			if (!AddNoteCheck(doc, addToSyncL, clickStaff, sym)) {
				InvalMeasure(addToSyncL, clickStaff);
				return FALSE;
			}
			HiliteInsertNode(doc, addToSyncL, clickStaff, TRUE);
			doc->selStartL = addToSyncL;
			doc->selEndL = RightLINK(doc->selStartL);
	
			/* Determine if the new note is to be added into an existing octava'd range,
				track the note, and add it to the sync. */
			if (TrkInsNote(doc, pt, &sym, clickStaff))
				return TRUE;
	
			goto Cancelled;
		}

	/* The click wasn't close to an existing sync, so we have to decide where
		to insert it. In graphic mode, get first node to the right of pt.h, and
		insert a new sync in first valid slot in object list before this node.
		In temporal mode, determine if there is a sync simultaneous with the
		location of the click ??LOOKS LIKE THIS ALWAYS SAYS "NO"; if so, add the
		note to this sync, else add a new sync at the correct temporal location. */

	if (isGraphic)	{														/* Get noteRightL directly */
		nodeRightL = FindSymRight(doc, pt, FALSE, FALSE);
	}

	/* Temporal mode: get the lTime of the last previous item and its duration
		(its ending time), then search for a sync at that time. If it exists
		add to it, else create a new one. */

	else {																	/* Get addToSyncL or nodeRightL */
		pLPIL = FindLPI(doc, pt, (doc->lookVoice>=0? ANYONE : clickStaff), voice, TRUE);
		addToSyncL = ObjAtEndTime(doc,pLPIL,SYNCtype,voice,&lTime,EXACT_TIME,TRUE);
		if (addToSyncL) { 												/* Got a sync at the right time */	
			jipL = FindJIP(pLPIL,addToSyncL);						/* #1. */
			if (jipL) {
				rightL = GSSearch(doc, pt, ANYTYPE, ANYONE, GO_RIGHT, FALSE, FALSE, FALSE);
				if (rightL && IsAfter(rightL,addToSyncL))
					for (pL=rightL; pL!=addToSyncL; pL=RightLINK(pL))
						if (J_IPTYPE(pL))
							return TrkInsSync(doc, pL, pt, &sym, clickStaff);
			}

			/* If the sync already has a note in the voice, create a new sync before it. */

			if (SyncInVoice(addToSyncL, voice))						/* Does it already have note/rest in this voice? */
				return TrkInsSync(doc, addToSyncL, pt, &sym, clickStaff);

			if (!AddNoteCheck(doc, addToSyncL, clickStaff, sym)) {
				InvalMeasure(addToSyncL, clickStaff);
				return FALSE;
			}
			HiliteInsertNode(doc, addToSyncL, clickStaff, TRUE);
			doc->selStartL = addToSyncL;
			doc->selEndL = RightLINK(doc->selStartL);
			if (TrkInsNote(doc, pt, &sym, clickStaff))
				return TRUE;

			goto Cancelled;
		}
		nodeRightL = TimeSearchRight(doc, RightLINK(pLPIL), ANYTYPE, lTime, MIN_TIME);
	}
	return TrkInsSync(doc, nodeRightL, pt, &sym, clickStaff);
	
Cancelled:
	doc->selStartL = doc->selEndL;
	MEAdjustCaret(doc, TRUE);
	InvalMeasure(addToSyncL, clickStaff);
	return FALSE;
}


/* -----------------------------------------------------------------InsertGRNote -- */
/* Insert a grace note at a place in the object list suitable for a
mousedown at the given point. Handles feedback and allows cancelling. */

Boolean InsertGRNote(Document *doc, Point pt, Boolean isGraphic)
{
	short clickStaff, voice, sym;
	LINK  nodeRightL, pLPIL, addToSyncL, addToGRSyncL;
	long	lTime;

	/* Get the staff to insert on (and set doc->currentSystem) */

	clickStaff = FindStaff(doc, pt);
	if (clickStaff==NOONE) return FALSE;

	/* If mouseDown before System's initial (invisible) barline, force it after
		that barline. */
	pt.h = ForceInMeasure(doc, pt.h);

	/* Get the symbol and the voice and check the voice's validity on this staff. */
	sym = GetSymTableIndex(palChar);
	voice = USEVOICE(doc, clickStaff);
	if (!AddCheckVoiceTable(doc, clickStaff)) return FALSE;
	
	/* Determine if the grace note is to be added to an already existing GRSync. If so,
		check the GRSync for validity, set the selection range to equal this GRSync,
		and add it to the GRSync. */ 

	addToGRSyncL = FindGRSync(doc, pt, isGraphic, clickStaff);
	if (addToGRSyncL) {
		if (!AddGRNoteCheck(doc, addToGRSyncL, clickStaff)) {
			InvalMeasure(addToGRSyncL, clickStaff);
			return FALSE;
		}
		HiliteInsertNode(doc, addToGRSyncL, clickStaff, TRUE);
		doc->selStartL = addToGRSyncL;
		doc->selEndL = RightLINK(doc->selStartL);

		if (TrkInsGRNote(doc, pt, &sym, clickStaff))		/* Chord w/existing note/chord */
			return TRUE;

		doc->selStartL = doc->selEndL;							/* Insert cancelled. */
		MEAdjustCaret(doc, TRUE);
		InvalMeasure(addToGRSyncL, clickStaff);
		return FALSE;
	}

	/* The click wasn't close to an existing GRSync, so we have to decide where
		to insert it. In graphic mode, get first node to the right of pt.h, and
		insert a new GRSync in first valid slot in object list before this node.
		In temporal mode, determine if there is a GRSync simultaneous with the
		location of the click; if so, add the note to this GRSync, else add a new
		GRSync at the correct temporal location. */

	if (isGraphic) {
		nodeRightL = FindSymRight(doc, pt, FALSE, FALSE);
		return TrkInsGRSync(doc, nodeRightL, pt, &sym, clickStaff);
	}

	/* Temporal mode: get the lTime of the last previous item and its duration
		(its ending time), then search for a sync at that time. If it exists
		add to a GRSync to its left, else search to the right for any object
		at or after that time and add to a GRSync to its left. */

	pLPIL = FindLPI(doc, pt, (doc->lookVoice>=0? ANYONE : clickStaff), voice, TRUE);
	addToSyncL = ObjAtEndTime(doc,pLPIL,GRSYNCtype,voice,&lTime,EXACT_TIME,TRUE);
	if (addToSyncL) {

		/* If the GRSync already has a note in the voice, create a new GRSync before
			it. (We have already determined that we are not clicking close enough
			to it to make a chord.) */

		return AddNewGRSync(doc, addToSyncL, pt, sym, voice, clickStaff);
	}

	nodeRightL = TimeSearchRight(doc, RightLINK(pLPIL), ANYTYPE, lTime, MIN_TIME);
	return AddNewGRSync(doc, nodeRightL, pt, sym, voice, clickStaff);
}


/* --------------------------------------------------------------- InsertArpSign -- */
/* Insert an arpeggio sign at a place in the object list suitable for a
mousedown at the given point. The point must be on a note (and not a rest). */

Boolean InsertArpSign(Document *doc, Point pt)
{
	LINK pL, aNoteL;
	short clickStaff, voice;
	
	pL = FindGraphicObject(doc, pt, &clickStaff, &voice);			
	if (!pL) return FALSE;
	if (!SyncTYPE(pL)) return FALSE;
	aNoteL = NoteInVoice(pL, voice, FALSE);
	if (NoteREST(aNoteL)) return FALSE;
	
	doc->selEndL = doc->selStartL = pL;										/* Selection is insertion point */
	return NewArpSign(doc, pt, palChar, clickStaff, voice);
}


/* ------------------------------------------------------------------ InsertLine -- */
/* Insert a line GRAPHIC into the object list at a place in the object list
suitable for a mousedown at the given point. Handles feedback and allows cancelling. */

Boolean InsertLine(Document *doc, Point pt)
{
	LINK insertL;
	short clickStaff, voice;
	
	insertL = FindGraphicObject(doc, pt, &clickStaff, &voice);			
	if (clickStaff==NOONE) return FALSE;
	if (!insertL) return FALSE;

	doc->selEndL = doc->selStartL = insertL;

	return TrackAndAddLine(doc, pt, clickStaff, voice);				/* Add sym. to obj list */
}


/* ---------------------------------------------------------------- InsertGraphic -- */
/* Insert a GRAPHIC object in the object list, including chord symbols and
rehearsal marks. Handles feedback and allows cancelling.
#1. When user tries to insert a Graphic on the final barline of a justified
system, FindGraphicObject finds a staff, but FindStaff doesn't: it uses
pContext->staffRight to compute the object rect for clipping the region
searched, where FindGraphicObject uses the systemRect, which has been extended
1 lnSpace beyond the right end of the staff, to handle precisely this case.
#2. In OtherCode.c */

#define SAVERM_LEN 	16
#define MPATCH_LEN 	16
#define MPAN_LEN	 	16

#define CHORD_FRAME_DEFAULT 'A'
#define REHEARSAL_MARK_DEFAULT 'A'
#define PATCH_CHANGE_DEFAULT "60"
#define PAN_SETTING_DEFAULT "64"
#define CHORD_SYMBOL_DEFAULT 'C'

Boolean InsertGraphic(Document *doc, Point pt)
{
	static char lastRMStr[SAVERM_LEN];	/* So we don't waste 256 bytes of static var.space */
	static char lastMPStr[MPATCH_LEN];	/* So we don't waste 256 bytes of static var.space */
	static char lastMPanStr[MPAN_LEN];	/* So we don't waste 256 bytes of static var.space */
	static Boolean firstRMCall=TRUE;
	static Boolean firstMPCall=TRUE;
	static Boolean firstMPanCall=TRUE;
	LINK pL,measL;
	short sym,pitchLev,clickStaff,voice,newSize,newStyle,newEncl,
			hStyleChoice,uStyleChoice,staff,auxInfo;
	Str63 newFont; Str255 string;
	Boolean newRelSize, newLyric;
	CONTEXT context;
	char midiPatch[MPATCH_LEN];
	char midiPanSetting[MPAN_LEN];

	pL = FindGraphicObject(doc, pt, &clickStaff, &voice);			
	sym = GetSymTableIndex(palChar);
	if (!ChkGraphicRelObj(pL, symtable[sym].subtype)) return FALSE;

	/*
	 * At this point, if the Graphic is to be attached to a System or Page, it
	 * must be either a GRString (text or lyric) or a GRChordFrame.
	 */
	switch (ObjLType(pL)) {
		case SYSTEMtype:
		case PAGEtype:
			break;
		case MEASUREtype:
			staff = FindStaff(doc,pt);							/* #1 */
			if (staff!=NOONE) clickStaff = staff;			/* Fall through */		
		default:
			if (clickStaff!=NOONE && STAFFN_BAD(doc, clickStaff)) {
				MayErrMsg("InsertGraphic: illegal staff number %ld. pL=%ld", (long)clickStaff, (long)pL);
				return FALSE;
			}
			HiliteInsertNode(doc, pL, clickStaff, TRUE);			
	}
																		/* #2 */

	/* GRAPHICs must be after the page they are attached to, due to problems
		with functions like BeforeFirstMeas, that rely on no object in a page
		or system being located before the page or system object itself in the
		object list. */

	doc->selStartL = (PageTYPE(pL) ? SSearch(pL,SYSTEMtype,GO_RIGHT) : pL);
	doc->selEndL = doc->selStartL;

	hStyleChoice = 0;										/* Unused for most subtypes */
	auxInfo = 0;

	if (PageTYPE(pL) || SystemTYPE(pL)) {
		measL = LSSearch(pL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
		GetContext(doc, measL, 1, &context);
		*string = 0;
		if (symtable[sym].subtype==GRChordFrame) {
			/* Chord frames are compatible with possible extensions to multiple characters. */
			string[0] = 1;
			string[1] = CHORD_FRAME_DEFAULT;
			if (!ChordFrameDialog(doc, &newRelSize, &newSize, &newStyle, &newEncl, newFont,
						&string[1])) goto Cancelled;
		}
		else {
			uStyleChoice = Header2UserFontNum(doc->lastGlobalFont);
			if (!TextDialog(doc,&uStyleChoice,&newRelSize,&newSize,&newStyle,
									&newEncl,&newLyric,newFont,string,&context))
				goto Cancelled;
			hStyleChoice = User2HeaderFontNum(doc, uStyleChoice);
		}
		NewGraphic(doc, pt, palChar, NOONE, voice, 0, newRelSize, newSize, newStyle,
							newEncl, 0, newLyric, newFont, string, hStyleChoice);
		return TRUE;
	}

	if (firstRMCall) {
		lastRMStr[0] = 1;
		lastRMStr[1] = REHEARSAL_MARK_DEFAULT;
		firstRMCall = FALSE;	
	}
	if (firstMPCall) {
		lastMPStr[0] = 2;
		lastMPStr[1] = '6';
		lastMPStr[2] = '0';		
		firstMPCall = FALSE;	
	}
	if (firstMPanCall) {
		lastMPanStr[0] = 2;
		lastMPanStr[1] = '6';
		lastMPanStr[2] = '4';		
		firstMPanCall = FALSE;	
	}
	
	if (InsTrackUpDown(doc, pt, &sym, pL, clickStaff, &pitchLev)) {
		switch (symtable[sym].subtype) {
			case GRRehearsal:
				PStrCopy((StringPtr)lastRMStr, (StringPtr)string);
				if (!RehearsalMarkDialog(string)) goto Cancelled;
				if (string[0]>SAVERM_LEN-1) string[0] = SAVERM_LEN-1;
				PStrCopy((StringPtr)string, (StringPtr)lastRMStr);

				newRelSize = doc->relFSizeRM;
				newSize = doc->fontSizeRM;
				newStyle = doc->fontStyleRM;
				newEncl = doc->enclosureRM;
				PStrCopy((StringPtr)doc->fontNameRM, (StringPtr)newFont);
				break;
			case GRChordSym:
				{
					static short CSAuxInfo = -1;

					/* NOTE: This will have to be changed if we encode other options in CSAuxInfo. */
					if (CSAuxInfo==-1)	/* first time */
						CSAuxInfo = config.chordSymDrawPar? 1 : 0;
					string[0] = 1;
					string[1] = CHORD_SYMBOL_DEFAULT;
					if (!ChordSymDialog(doc, string, &CSAuxInfo)) goto Cancelled;
					newRelSize = doc->relFSizeCS;
					newSize = doc->fontSizeCS;
					newStyle = doc->fontStyleCS;
					newEncl = doc->enclosureCS;
					PStrCopy((StringPtr)doc->fontNameCS, (StringPtr)newFont);
					auxInfo = CSAuxInfo;
				}
				break;
			case GRChordFrame:
				/* Chord frames are compatible with possible extensions to multiple characters. */
				string[0] = 1;
				string[1] = CHORD_FRAME_DEFAULT;
				if (!ChordFrameDialog(doc, &newRelSize, &newSize, &newStyle, &newEncl, newFont,
						&string[1])) goto Cancelled;
				break;
			case GRString:
				GetContext(doc, pL, clickStaff, &context);
				*string = 0;
				uStyleChoice = Header2UserFontNum(doc->lastGlobalFont);
				if (!TextDialog(doc,&uStyleChoice,&newRelSize,&newSize,&newStyle,
										&newEncl,&newLyric,newFont,string,&context))
					goto Cancelled;
				hStyleChoice = User2HeaderFontNum(doc, uStyleChoice);
				break;
			case GRMIDIPatch:
				PStrCopy((StringPtr)lastMPStr, (StringPtr)string);
				if (!PatchChangeDialog(string)) goto Cancelled;
				if (string[0]>MPATCH_LEN-1) string[0] = MPATCH_LEN-1;
				PStrCopy((StringPtr)string, (StringPtr)lastMPStr);
				PStrCopy((StringPtr)string, (StringPtr)midiPatch);
				
				newRelSize = doc->relFSizeRM;
				newSize = doc->fontSizeRM;
				newStyle = doc->fontStyleRM;
				newEncl = doc->enclosureRM;
				PStrCopy((StringPtr)doc->fontNameRM, (StringPtr)newFont);
				break;
			case GRMIDIPan:
				PStrCopy((StringPtr)lastMPanStr, (StringPtr)string);
				if (!PanSettingDialog(string)) goto Cancelled;
				if (string[0]>MPAN_LEN-1) string[0] = MPAN_LEN-1;
				PStrCopy((StringPtr)string, (StringPtr)lastMPanStr);
				PStrCopy((StringPtr)string, (StringPtr)midiPanSetting);
				
				newRelSize = doc->relFSizeRM;
				newSize = doc->fontSizeRM;
				newStyle = doc->fontStyleRM;
				newEncl = doc->enclosureRM;
				PStrCopy((StringPtr)doc->fontNameRM, (StringPtr)newFont);
				break;
			case GRMIDISustainOn:
				string[0] = 3; string[1] = '1'; string[2] = '2'; string[3] = '7';
				newRelSize = doc->relFSizeRM;
				newSize = doc->fontSizeRM;
				newRelSize = TRUE;
				newSize = GRStaffHeight;
				newStyle = doc->fontStyleRM;
				newEncl = ENCL_NONE;
				PStrCopy((StringPtr)doc->fontNameRM, (StringPtr)newFont);
				break;
			case GRMIDISustainOff:
				string[0] = 1; string[1] = '0';
				newRelSize = doc->relFSizeRM;
				newSize = doc->fontSizeRM;
				newRelSize = TRUE;
				newSize = GRStaffHeight;
				newStyle = doc->fontStyleRM;
				newEncl = ENCL_NONE;
				PStrCopy((StringPtr)doc->fontNameRM, (StringPtr)newFont);
				break;
			default:
				;															/* Error */
		}
		
		NewGraphic(doc, pt, palChar, clickStaff, voice, pitchLev, newRelSize, newSize,
							newStyle, newEncl, auxInfo, newLyric, newFont, string, hStyleChoice);
		return TRUE;
	}
	
Cancelled:
	InvalMeasure(pL, clickStaff);									/* Just redraw the measure */
	return FALSE;
}


/* Given objtype and subtype, return the corresponding symbol table index. Cf.
InitNightGlobals for another way to do this: it's probably better to use this
function locally and avoid globals. */

short Type2SymTableIndex(SignedByte objtype, SignedByte subtype);
short Type2SymTableIndex(SignedByte objtype, SignedByte subtype)
{
	short j;
	
	for (j=0; j<nsyms; j++)
		if (objtype==symtable[j].objtype && subtype==symtable[j].subtype)
			return j;								/* Found it */

	return NOMATCH;								/* Not found - illegal */
}

/* -------------------------------------------------------------- InsertMusicChar -- */
/* Insert a music character as a GRAPHIC object in the object list. Handles feedback
and allows cancelling. */

Boolean InsertMusicChar(Document *doc, Point pt)
{
	LINK pL;
	short sym,pitchLev,clickStaff,voice,styleChoice,staff,stringInd;
	Str63 musicFontName;
	unsigned char string[2];
	char stringInchar;
	Boolean firstCall = TRUE;
	/* musicChar maps symtable[sym].durcode value to corresponding font character:
	                     Ped.   *   */
	char musicChar[] = { 0xA1, '*' };
	short index;

	if (firstCall) {
		stringInd = Type2SymTableIndex(GRAPHICtype, GRString);
		stringInchar = symtable[stringInd].symcode;
		firstCall = FALSE;
	}
	
	pL = FindGraphicObject(doc, pt, &clickStaff, &voice);			
	sym = GetSymTableIndex(palChar);
	if (!ChkGraphicRelObj(pL, symtable[sym].subtype)) return FALSE;

	switch (ObjLType(pL)) {
		case SYSTEMtype:
		case PAGEtype:
			break;
		case MEASUREtype:
			staff = FindStaff(doc,pt);							/* See comment in InsertGraphic */
			if (staff!=NOONE) clickStaff = staff;			/* Fall through */		
		default:
			if (clickStaff!=NOONE && STAFFN_BAD(doc, clickStaff)) {
				MayErrMsg("InsertMusicChar: illegal staff number %ld. pL=%ld", (long)clickStaff, (long)pL);
				return FALSE;
			}
			HiliteInsertNode(doc, pL, clickStaff, TRUE);			
	}

	/* GRAPHICs must be after the page they are attached to, due to problems
		with functions like BeforeFirstMeas, that rely on no object in a page
		or system being located before the page or system object itself in the
		object list. */

	doc->selStartL = (PageTYPE(pL) ? SSearch(pL,SYSTEMtype,GO_RIGHT) : pL);
	doc->selEndL = doc->selStartL;

	styleChoice = 0;
	if (InsTrackUpDown(doc, pt, &sym, pL, clickStaff, &pitchLev)) {
		string[0] = 1;
		index = symtable[sym].durcode;
		if (index<0 || index>sizeof(musicChar))
			MayErrMsg("InsertMusicChar: bad symtable[sym].durcode");
		string[1] = musicChar[index];
		
		/* NB: If doc->musFontName not available on this system, doc->musicFontNum will be Sonata. */
		GetFontName(doc->musicFontNum, musicFontName);
		if (*musicFontName==0)
			PStrCopy((StringPtr)"\pSonata", (StringPtr)musicFontName);
		NewGraphic(doc, pt, stringInchar, clickStaff, voice, pitchLev, TRUE, GRStaffHeight,
							0, ENCL_NONE, 0, FALSE, musicFontName, string, FONT_THISITEMONLY);
		return TRUE;
	}

	InvalMeasure(pL, clickStaff);									/* Just redraw the measure */
	return FALSE;
}


/* ----------------------------------------------------------------- ChkInsMODNR -- */
/* Return FALSE to indicate we won't handle addition of MODNR in this situation. 
If obj is not a note or rest, we won't handle it, but will give the user some
advice on how to get what they seem to want. */

static Boolean ChkInsMODNR(LINK insSyncL, short sym)
{
	if (MeasureTYPE(insSyncL) && symtable[sym].subtype==MOD_FERMATA) {
		GetIndCString(strBuf, INSERTERRS_STRS, 1);    /* "To attach a fermata to a barline, use..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	if (!SyncTYPE(insSyncL)) {
		GetIndCString(strBuf, INSERTERRS_STRS, 2);    /* "To attach a modifier to anything other than a note or rest..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	
	return TRUE;
}

/* ------------------------------------------------------------------InsertMODNR -- */
/* Adds a MODNR ("Modify Note/Rest") or augmentation dot to a note or rest at
the given point. Handles feedback and (other than for aug. dots) and allows
cancelling. */

Boolean InsertMODNR(Document *doc, Point pt)
{
	short				staff, sym, index, pitchLev, qPitchLev, newSlashes, status, i;
	static short	slashes=2, lastPitchLev=-3;
	LINK				insSyncL, aNoteL;

	staff=FindStaff(doc, pt);
	if (staff==NOONE) return FALSE;
	
	/* Find the symbol clicked on. */
	insSyncL = FindObject(doc, pt, &index, SMFind);
	if (!insSyncL) return FALSE;

	sym = GetSymTableIndex(palChar);
	if (!ChkInsMODNR(insSyncL,sym)) return FALSE;
		
	/* Find the note/rest to be modified. */
	aNoteL = FirstSubLINK(insSyncL);
	for (i = 0; aNoteL; i++, aNoteL = NextNOTEL(aNoteL))
		if (i==index) break;

	if (!aNoteL) {
		MayErrMsg("InsertMODNR: note %ld in sync at %ld not found", (long)index, (long)insSyncL);
		return FALSE;
	}

	staff = NoteSTAFF(aNoteL);
	doc->selEndL = doc->selStartL = insSyncL;						/* Selection is insertion point */

	if (symtable[sym].subtype==MOD_FAKE_AUGDOT) {
		AddDot(doc, insSyncL, NoteVOICE(aNoteL));
		return TRUE;
	}

	HiliteInsertNode(doc, insSyncL, staff, TRUE);
	status = InsTrackUpDown(doc, pt, &sym, doc->selStartL, staff, &pitchLev);
	if (status!=0) {
		if (symtable[sym].subtype==MOD_TREMOLO1) {
			pitchLev = lastPitchLev;								/* Ignore the new <pitchLev> */
			newSlashes = TremSlashesDialog(slashes);
			if (newSlashes<=0) {
				InvalMeasure(doc->selStartL, staff);			/* Just redraw the measure */
				return FALSE;
			}

			slashes = newSlashes;
			
			/* Tremolos really belong to a chord, not a note: to get correct position,
				substitute the voice's MainNote for the one clicked on. Since we already
				got the staff, this assumes MainNote is always on the same staff--true
				as of v.2.0. */
				
			aNoteL = FindMainNote(insSyncL, NoteVOICE(aNoteL));

			qPitchLev = halfLn2qd(pitchLev);
		}
		else if (status<0) {
			/* No vertical mouse mvmt: ask an expert for <qPitchLev> to use */
			qPitchLev = ModNRPitchLev(doc, symtable[sym].subtype, insSyncL, aNoteL);
		}
		else
			qPitchLev = halfLn2qd(pitchLev);
		
		if (symtable[sym].subtype==MOD_TREMOLO1)
			qPitchLev = 0;
		NewMODNR(doc,symtable[sym].symcode,slashes,staff,qPitchLev,insSyncL,aNoteL);
		InvalMeasure(doc->selStartL, staff);					/* Just redraw the measure */
		lastPitchLev = pitchLev;
		return TRUE;
	}

	InvalMeasure(doc->selStartL, staff);						/* Just redraw the measure */
	return FALSE;
}


/* -----------------------------------------------------------------InsertRptEnd -- */
/* Insert a RepeatEnd at a place in the object list suitable for a
mousedown at the given point. RptEnds are J_IT objects. */

Boolean InsertRptEnd(Document *doc, Point pt)
{
	short		clickStaff;													/* staff user clicked in */
	LINK		pLPIL;														/* pointer to Last Previous Item */

	clickStaff = FindStaff(doc, pt);									/* Find staff clicked on... */
	if (clickStaff==NOONE) return FALSE;
	pLPIL = FindLPI(doc, pt, clickStaff, ANYONE, FALSE);		/*   and Last Prev. Item on it */

	doc->selEndL = doc->selStartL = RightLINK(pLPIL);			/* selection is insertion point... */
	NewRptEnd(doc, pt.h, palChar, clickStaff);					/* Add sym. to object list */
	return TRUE;
}


/* ---------------------------------------------------------------- InsertEnding -- */
/* Insert an ending when the user clicks in the score with the Ending tool.
Find the relative object for the ending located at pt, if one exists, and
add the ending to the object list. Endings are J_D objects. */

Boolean InsertEnding(Document *doc, Point pt)
{
	short		clickStaff,													/* staff in which user clicked */
				staff;														/* staff & voice of obj found */
	LINK		insertL;														/* relative obj for Ending */

	clickStaff = FindStaff(doc, pt);									/* Find staff clicked on. */
	if (clickStaff==NOONE) return FALSE;
	
	insertL = FindEndingObject(doc, pt, &staff);					/* Find relative object for Ending */
	if (!insertL) return FALSE;

	doc->selEndL = doc->selStartL = insertL;

	return TrackAndAddEnding(doc, pt, staff);						/* Add sym. to object list */
}


/* -------------------------------------------------------- InsertMeasure helpers -- */

static Boolean InsMeasTupletOK(Document *);
static Boolean InsMeasUnkDurOK(Document *);

/* If there are tuplets across doc->selStartL on any staff, give an alert and
return FALSE. */

static Boolean InsMeasTupletOK(Document *doc)
{
	short s;
	char fmtStr[256];

	for (s = 1; s<=doc->nstaves; s++)
		if (HasTupleAcross(doc->selStartL, s)) {
			GetIndCString(fmtStr, INSERTERRS_STRS, 21);    /* "Can't insert a barline in middle of a tuplet..." */
			sprintf(strBuf, fmtStr, s); 
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}
		
	return TRUE;
}

/* If there are unknown-duration notes in doc->selStartL's measure, ask user whether
to proceed; return TRUE if so. Otherwise just return TRUE. */

static Boolean InsMeasUnkDurOK(Document *doc)
{
	LINK measL;

	measL = LSSearch(doc->selStartL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	if (!RhythmUnderstood(doc, measL, TRUE))
		return (InsMeasUnkDurDialog());
		
	return TRUE;
}

/* --------------------------------------------------------------- InsertMeasure -- */
/* Insert a Measure at a place in the object list suitable for a mousedown at
the given point. */

Boolean InsertMeasure(Document *doc, Point pt)
{
	short		clickStaff;
	LINK		pLPIL;														/* link to Last Previous Item */

	clickStaff = FindStaff(doc, pt);									/* Sets doc->currentSystem */
	if (clickStaff==NOONE) return FALSE;							/* Quit if no staff clicked on */

	/* If mouseDown before System's initial (invisible) barline, force it after
		that barline. */
	pt.h = ForceInMeasure(doc, pt.h);
	pLPIL = FindLPI(doc, pt, ANYONE, ANYONE, FALSE);			/* Find Last Prev. Item on ANY staff */
	
	doc->selEndL = doc->selStartL = RightLINK(pLPIL);			/* selection is insertion point... */
	if (!InsMeasTupletOK(doc)) return FALSE;
	if (!InsMeasUnkDurOK(doc)) return FALSE;

	NewMeasure(doc, pt, palChar);										/* Add sym. to object list */
	return TRUE;
}


/* ------------------------------------------------------------ InsertPseudoMeas -- */
/* Insert a pseudomeasure at a place in the object list suitable for a mousedown
at the given point. */

Boolean InsertPseudoMeas(Document *doc, Point pt)
{
	short		clickStaff;													/* staff user clicked in */
	LINK		pLPIL;														/* link to Last Previous Item */

	clickStaff = FindStaff(doc, pt);									/* Was ANY staff clicked on? */
	if (clickStaff==NOONE) return FALSE;							/* If no, forget it */

	/* If mouseDown before System's initial (invisible) barline, force it after
		that barline. */
	pt.h = ForceInMeasure(doc, pt.h);
	pLPIL = FindLPI(doc, pt, ANYONE, ANYONE, FALSE);			/* Find Last Prev. Item on ANY staff */
	
	doc->selEndL = doc->selStartL = RightLINK(pLPIL);			/* selection is insertion point */
	NewPseudoMeas(doc, pt, palChar);									/* Add sym. to object list */

	return TRUE;
}


/* ------------------------------------------------------------------ InsertClef -- */
/* Insert a clef at a place in the object list suitable for a mousedown at the
given point. */

Boolean InsertClef(Document *doc, Point pt)
{
	short		clickStaff;													/* staff user clicked in */
	LINK		pLPIL,insNodeL,aClefL;
	long		lTime;														/* logical time */
				
	clickStaff = FindStaff(doc, pt);									/* Find staff clicked on... */
	if (clickStaff==NOONE) return FALSE;
	
	pLPIL = FindLPI(doc, pt, clickStaff, ANYONE, FALSE);		/*   and Last Prev. Item on it */
	if (ClefBeforeBar(doc,pLPIL,palChar,clickStaff))
		return TRUE;

	insNodeL = ObjAtEndTime(doc,pLPIL,CLEFtype,clickStaff,&lTime,EXACT_TIME,FALSE);
	if (insNodeL) {

		aClefL = FirstSubLINK(insNodeL);
		for ( ; aClefL; aClefL = NextCLEFL(aClefL))
			if (ClefSTAFF(aClefL)==clickStaff) return TRUE;

		doc->selEndL = doc->selStartL = insNodeL;
		NewClef(doc, -99, palChar, clickStaff);				/* -99 flags adding to pre-existing clef. */
		return TRUE;
	}
	insNodeL = TimeSearchRight(doc, RightLINK(pLPIL), ANYTYPE, lTime, MIN_TIME);
	if (insNodeL) {
		doc->selEndL = doc->selStartL = LocateInsertPt(insNodeL);
		NewClef(doc, pt.h, palChar, clickStaff);				/* Add sym. to object list */
		return TRUE;
	}
	MayErrMsg("InsertClef: free TimeSearchRight returned NILINK.");
	return FALSE;
}


/* ---------------------------------------------------------------- InsertKeySig -- */
/* Insert a key signature at a place in the object list suitable for a mousedown
at the given point. */

Boolean InsertKeySig(Document *doc, Point pt)
{
	short		clickStaff,								/* staff user clicked in */
				sharps=0,flats=0,
				sharpsOrFlats=0;
	LINK		pLPIL,									/* pointer to Last Previous Item */
				insNodeL;
	long		lTime;									/* logical time */
	static Boolean onAllStaves=TRUE;				/* or "This Staff Only" */

	if (!KeySigDialog(&sharps, &flats, &onAllStaves, TRUE))
		return FALSE;
	
	if (sharps>0) 		sharpsOrFlats = sharps;
	else if (flats>0) sharpsOrFlats = -flats;

	if (onAllStaves) {
		SetCurrentSystem(doc, pt);
		clickStaff = ANYONE;
	}
	else
		clickStaff = FindStaff(doc, pt);									/* Find System & staff clicked on... */
	if (clickStaff==NOONE) return FALSE;
	
	pLPIL = FindLPI(doc, pt, clickStaff, ANYONE, FALSE);			/*   and Last Prev. Item on it */
	if (KeySigBeforeBar(doc, pLPIL, clickStaff, sharpsOrFlats))
		return TRUE;
		
	insNodeL = ObjAtEndTime(doc,pLPIL,ANYTYPE,clickStaff,&lTime,MIN_TIME,FALSE);
	if (insNodeL) {
		doc->selEndL = doc->selStartL = LocateInsertPt(insNodeL);
		NewKeySig(doc, pt.h, sharpsOrFlats, clickStaff);		/* Add sym(s). to object list */
		return TRUE;
	}

	MayErrMsg("InsertKeySig: free TimeSearchRight returned NILINK.");
	return FALSE;
}


/* --------------------------------------------------------------- InsertTimeSig -- */
/* Insert a time signature at a place in the object list suitable for a	mousedown
at the given point. */

Boolean InsertTimeSig(Document *doc, Point pt)
{
	short			clickStaff;
	LINK			pLPIL,									/* pointer to Last Previous Item */
					insNodeL;
	long			lTime;									/* logical time */
	static short type=N_OVER_D,
					 numerator=4,denominator=4;
	static Boolean onAllStaves=TRUE;					/* or "This Staff Only" */

	if (!TimeSigDialog(&type, &numerator, &denominator, &onAllStaves, TRUE))
		return FALSE;

	if (onAllStaves) {
		SetCurrentSystem(doc, pt);
		clickStaff = ANYONE;
	}
	else
		clickStaff = FindStaff(doc, pt);								/* also sets doc->currentSystem */
	if (clickStaff==NOONE) return FALSE;
	
	pLPIL = FindLPI(doc, pt, clickStaff, ANYONE, FALSE);		/* find Last Prev. Item on clickStaff */
	if (TimeSigBeforeBar(doc, pLPIL, clickStaff, type, numerator, denominator))
		return TRUE;
	
	insNodeL = ObjAtEndTime(doc,pLPIL,ANYTYPE,clickStaff,&lTime,MIN_TIME,FALSE);
	if (insNodeL) {
		doc->selEndL = doc->selStartL = LocateInsertPt(insNodeL);
		NewTimeSig(doc, pt.h, palChar, clickStaff, type, numerator, denominator);
		return TRUE;
	}

	MayErrMsg("InsertTimeSig: free TimeSearchRight returned NILINK.");
	return FALSE;
}


/*	---------------------------------------------------------------- InsertHairpin -- */
/* Handle hairpin insertion. */

static Boolean InsertHairpin(Document *doc, Point pt, LINK /*pL*/, short clickStaff)
{
	LINK insSyncL,prevMeasL; short sym,subtype;
	CONTEXT context;

	sym = GetSymTableIndex(palChar);
 	subtype = symtable[sym].subtype;

	/* Graphically search left for a sync. If found, search again on clickStaff to
		insure sync is on the proper staff. If none, we're before the first sync of
		the system; search right graphically. If none, no sync; return. Otherwise,
		see if sync found graphically is in a different measure from mouseDown pt;
		if so, get the first sync in the mouseDown pt's measure. If none, return. */

	insSyncL = FindSyncLeft(doc, pt, TRUE);
	if (insSyncL)
		insSyncL = LSISearch(insSyncL, SYNCtype, clickStaff, GO_LEFT, FALSE);
	if (!insSyncL)
		insSyncL = GSSearch(doc,pt,SYNCtype,clickStaff,GO_RIGHT,FALSE,FALSE,TRUE);
	if (!insSyncL) return FALSE;

	prevMeasL = GSSearch(doc,pt,MEASUREtype,clickStaff,GO_LEFT,FALSE,FALSE,FALSE);
	if (IsAfter(insSyncL,prevMeasL)) {
		insSyncL = LSSearch(prevMeasL,SYNCtype,clickStaff,GO_RIGHT,FALSE);
		if (!insSyncL) return FALSE;
	}

	HiliteInsertNode(doc, insSyncL, clickStaff, TRUE);
	GetContext(doc, insSyncL, clickStaff, &context);
	return TrackAndAddHairpin(doc, insSyncL, pt, clickStaff, sym, subtype, &context); 
}


/* -------------------------------------------------------------- InsertDynamic -- */
/* Insert a dynamic marking at a place in the object list suitable for a
mousedown at the given point. Handles feedback and allows cancelling. */

Boolean InsertDynamic(Document *doc, Point pt)
{
	short pitchLev,sym,clickStaff,index,staff,subtype;
	LINK pL,insSyncL;
	Point startPt; CONTEXT context;

	/* Get the staff to insert on (and set doc->currentSystem). If click was
		on a symbol, use that symbol's staff, otherwise use the closest staff. */

	clickStaff = FindStaff(doc, pt);
	if (clickStaff==NOONE) return FALSE;

	sym = GetSymTableIndex(palChar);
 	subtype = symtable[sym].subtype;

	pL = FindObject(doc, pt, &index, SMFind);

	if (pL && !SystemTYPE(pL)) {
		staff = GetSubObjStaff(pL, index);
		if (staff!=NOONE) clickStaff = staff;
	}

	/* If mouseDown before System's initial (invisible) barline, force it after
		that barline. */
	pt.h = ForceInMeasure(doc, pt.h);

	if (subtype==DIM_DYNAM || subtype==CRESC_DYNAM)
		return InsertHairpin(doc, pt, pL, clickStaff);

	/* Graphically search left for a sync. If none, we're before the first sync of the
		system; search right graphically. If none, no sync; return. Otherwise, see
		if sync found graphically in a different measure from mouseDown pt; if so,
		get the first sync in the mouseDown pt's measure. If none, return. */

	startPt = pt;
	startPt.h -= UseMagnifiedSize(3, doc->magnify);						/* Allow user some slop */
	insSyncL = GSSearch(doc, startPt, SYNCtype, clickStaff, GO_RIGHT, FALSE, FALSE, TRUE);
	if (!insSyncL) return FALSE;

	HiliteInsertNode(doc, insSyncL, clickStaff, TRUE);
	GetContext(doc, insSyncL, clickStaff, &context);
	doc->selEndL = doc->selStartL = insSyncL;
	if (InsTrackUpDown(doc, pt, &sym, doc->selStartL, clickStaff, &pitchLev)) {
		NewDynamic(doc,pt.h,0,symtable[sym].symcode,clickStaff,pitchLev,NILINK,FALSE);
		return TRUE;
	}

	InvalMeasure(insSyncL, clickStaff);										/* Just redraw the measure */
	return FALSE;
}


static short GetNoteStfVoice(LINK pL, short index, short *v);

static short GetNoteStfVoice(LINK pL, short index, short *v)
{
	LINK aNoteL; short i;

	/* Return the voice and staff of the note at index. */
	aNoteL = FirstSubLINK(pL);
	for (i = 0; aNoteL; i++, aNoteL=NextNOTEL(aNoteL))
		if (i==index) {
			*v = NoteVOICE(aNoteL);
			return NoteSTAFF(aNoteL);
		}

	*v = NOONE; return NOONE;
}

/* ------------------------------------------------------------------ InsertSlur -- */
/* Insert a slur (or set of ties) at a place in the object list suitable
for a mousedown at the given point. */

Boolean InsertSlur(Document *doc, Point pt)
{
	short		staff, index, voice;
	LINK		pL, pLPIL;
	Point		localPt;

	FindStaff(doc, pt);												/* Sets doc->currentSystem */

	/* Correct "optical illusion" (CER's term) problem with slur cursor, which
		cannot be fixed by moving down hotspot, which is moved down
		all it can be. */

	localPt = pt; localPt.v += 1;									/* Correct for "optical illusion" */
	pL = FindObject(doc, localPt, &index, SMFindNote);
	if (!pL) return FALSE;
	if (!SyncTYPE(pL)) return FALSE;

	/* Get the voice and staff of the note/rest clicked on. */
	staff = GetNoteStfVoice(pL,index,&voice);
	if (staff==NOONE) return FALSE;

	/* ??Should NOT call FindLPI; pL should be what we want. */
	pLPIL = FindLPI(doc, pt, staff, ANYONE, FALSE);
	doc->selEndL = doc->selStartL = RightLINK(pLPIL);		/* Selection is insertion point */
	NewSlur(doc, staff, voice, pt);								/* Add sym. to object list */
	return TRUE;
}


/* ------------------------------------------------------------------ InsertTempo -- */
/* Insert a tempo mark at a place in the object list suitable for a
mousedown at the given point.	Handles feedback and allows cancelling. */

Boolean InsertTempo(Document *doc, Point pt)
{
	short clickStaff,v,sym,pitchLev; static short dur,beatsPM; LINK pL;
	static Boolean hideMM,dotted;
	static Str63 tempoStr;		/* limit length to save static var. space */
	static Str63 metroStr;		/* limit length to save static var. space */
	static Boolean firstCall=TRUE;

	pL = FindTempoObject(doc, pt, &clickStaff, &v);			
	if (pL==NILINK) return FALSE;
	if (MeasureTYPE(pL)) clickStaff = FindStaff(doc, pt);
	
	if (!(SystemTYPE(pL) || PageTYPE(pL))) HiliteInsertNode(doc, pL, clickStaff, TRUE);
	
	if (InsTrackUpDown(doc, pt, &sym, pL, clickStaff, &pitchLev)) {
		if (firstCall) {
			dur = QTR_L_DUR;
			dotted = FALSE;
			beatsPM = config.defaultTempo;
			NumToString(beatsPM, metroStr);
			hideMM = FALSE;
			PStrCopy((StringPtr)"\p", (StringPtr)tempoStr);
			firstCall = FALSE;
		}

		Pstrcpy((unsigned char *)strBuf, (unsigned char *)tempoStr);
		Pstrcpy((unsigned char *)tmpStr, (unsigned char *)metroStr);
		if (TempoDialog(&hideMM, &dur, &dotted, (unsigned char *)strBuf, tmpStr)) {
			doc->selEndL = doc->selStartL = pL;
			if (strBuf[0]>63) strBuf[0] = 63;					/* Limit str. length (see above) */
			Pstrcpy((unsigned char *)tempoStr, (unsigned char *)strBuf);
			if (tmpStr[0]>63) tmpStr[0] = 63;					/* Limit str. length (see above) */
			Pstrcpy((unsigned char *)metroStr, (unsigned char *)tmpStr);
			NewTempo(doc,pt,palChar,clickStaff,pitchLev,hideMM,dur,
									dotted,tempoStr,metroStr);
			return TRUE;
		}
	}

	InvalMeasure(pL, clickStaff);									/* Just redraw the measure */
	return FALSE;
}


/* ------------------------------------------------------------------- InsertSpace -- */
/* Insert a space object at a place in the object list suitable for a
mousedown at the given point. Handles feedback and allows cancelling. */

Boolean InsertSpace(Document *doc, Point pt)
{
	short clickStaff,topStf,bottomStf; LINK pLPIL,measL; STDIST stdSpace=0;
	Point newPt; CONTEXT context;
	
	clickStaff = FindStaff(doc, pt);										/* Find staff clicked on */
	if (clickStaff==NOONE) return FALSE;

	measL = GSSearch(doc, pt, MEASUREtype, ANYONE, TRUE, FALSE, FALSE, FALSE); /* Need a LINK for GetContext */
	if (!measL) return FALSE;
	GetContext(doc, measL, clickStaff, &context);

	newPt = InsertSpaceTrackStf(doc, pt, &topStf, &bottomStf);	/* Get user feedback */
	if (newPt.h==CANCEL_INT)
		return FALSE;
	if (newPt.h-pt.h)															/* Don't divide by zero */
		stdSpace = d2std(p2d(ABS(newPt.h-pt.h)), context.staffHeight, context.staffLines);

	if (pt.h>newPt.h) pt = newPt;
	pLPIL = FindLPI(doc, pt, clickStaff, ANYONE, FALSE);			/* Find last prev item on it */
	if (pLPIL) {
		doc->selEndL = doc->selStartL = RightLINK(pLPIL);
		NewSpace(doc, pt, palChar, topStf, bottomStf, stdSpace);	/* Add sym. to object list */
		return TRUE;
	}
	return FALSE;
}


/* ------------------------------------------------------------- XLoadInsertSeg -- */
/* Null function to allow loading or unloading Insert.c's segment. */

void XLoadInsertSeg()
{
}