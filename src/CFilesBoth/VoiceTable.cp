/***************************************************************************
*	FILE:	VoiceTable.c (formerly called VoiceNumbers.c)
*	PROJ:	Nightingale, revised for v. 1.4
*	DESC:	Routines for manipulating the voice-mapping table
		OffsetVoiceNums			FillVoiceTable			BuildVoiceTable
		MapVoiceNums			CompactVoiceNums
		UpdateVoiceTable		User2IntVoice			Int2UserVoice
		NewVoiceNum				CountVoices
/***************************************************************************/

/*										NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL
 * PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A
 * TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE
 * NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1991-97 by Advanced Music Notation Systems, Inc.
 * All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* About voice tables:
	A document's "voice-mapping table" or (for short) "voice table" says what internal
voice numbers are in use and, for each one that is in use: what part it belongs to;
its "voice position" or "role" (upper, lower, cross-staff, or single); and its
user voice number, by which Nightingale lets the user refer to the voice. Internal
voice numbers must be unique, but user voice numbers need not be; in fact, they start
with 1 for each part.

The voice table, doc->voiceTab, is indexed by the internal voice number. Every staff
has a default voice whose internal number is the same as its staff number, and we
reserve slots in the table for these voices. Thus, in a 5-staff score, the first 5
entries in the table always refer to the default voices. If the score consists of one
2-staff part and (below it) one 3-staff part, the beginning of the table looks like
this:
	slot (=internal voice)	1	2	3	4	5
	part					1	1	2	2	2
	user voice				1	2	1	2	3

A slot with partn==0 is empty. The table must always be compact, i.e., there can be no
slots in use after the first empty slot. */

/* Prototypes for internal functions */
static void FillVoiceTable(Document *, Boolean []);
static void CompactVoiceNums(Document *);


/* ------------------------------------------------------------- OffsetVoiceNums -- */
/* Add <nvDelta> to voice numbers of objects and subobjects that currently have voice
numbers of at least <startSt>. Intended to open up space in the voice mapping
table for newly-added staves' default voices, or to remove space formerly needed
for newly-deleted staves' default voices. */

void OffsetVoiceNums(Document *doc, short startSt, short nvDelta)
{
	LINK aNoteL, aGRNoteL;
	register LINK pL;
	
	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteVOICE(aNoteL)>startSt)
						NoteVOICE(aNoteL) += nvDelta;
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
					if (GRNoteVOICE(aGRNoteL)>startSt)
						GRNoteVOICE(aGRNoteL) += nvDelta;
				break;
			case BEAMSETtype:
				if (BeamVOICE(pL)>startSt)
					BeamVOICE(pL) += nvDelta;
				break;
			case TUPLETtype:
				if (TupletVOICE(pL)>startSt)
					TupletVOICE(pL) += nvDelta;
				break;
			case GRAPHICtype:
				if (GraphicVOICE(pL)>startSt)
					GraphicVOICE(pL) += nvDelta;
				break;
			case SLURtype:
				if (SlurVOICE(pL)>startSt)
					SlurVOICE(pL) += nvDelta;
				break;
			default:
				;
		}
	}
}


/* -------------------------------------------------------------- FillVoiceTable -- */
/* Traverse the document's main object list and fill in voice numbers as they're
actually used. Does not initialize the voice table, so initialization (with default
voices, the voice Looked at, etc.) should be done before calling this function. Also,
if the caller cares about the <unused> flags, it should set them to TRUE before
calling us. */

static void FillVoiceTable(
					register Document *doc,
					Boolean unused[])			/* On return, TRUE=nothing in voice */
{
	LINK pL, aNoteL, aGRNoteL;
	short	v, iVoice, partn, lastUsedV;
	
	/*
	 *	The objects with voice numbers other than notes and grace notes always
	 *	get their voice numbers from the notes and grace notes they're attached
	 *	to; thus, we don't need to pay any attention to the other object types
	 *	here.
	 */
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					iVoice = NoteVOICE(aNoteL);
					unused[iVoice] = FALSE;
					if (doc->voiceTab[iVoice].partn!=0) continue;
					/*
					 *	The table has no entry for this note's voice number. Fill the
					 *	relVoice field with one more than the highest number already in
					 *	use for this part; initialize the voice position to single voice
					 *	on staff; fill the part number field with this note's part number.
					 */					
					partn = Staff2Part(doc, NoteSTAFF(aNoteL));
					for (lastUsedV = 0, v = 1; v<=MAXVOICES; v++)
						if (doc->voiceTab[v].partn==partn)
							lastUsedV = n_max(lastUsedV, doc->voiceTab[v].relVoice);
					doc->voiceTab[iVoice].partn = partn;
					doc->voiceTab[iVoice].voiceRole = SINGLE_DI;
					doc->voiceTab[iVoice].relVoice = lastUsedV+1;
				}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
					iVoice = GRNoteVOICE(aGRNoteL);
					unused[iVoice] = FALSE;
					if (doc->voiceTab[iVoice].partn!=0) continue;
					/*
					 *	The table has no entry for this note's voice number. Fill the
					 *	relVoice field with one more than the highest number already in
					 *	use for this part; initialize the voice position to single voice
					 *	on staff; fill the part number field with this note's part number.
					 */					
					partn = Staff2Part(doc, GRNoteSTAFF(aGRNoteL));
					for (lastUsedV = 0, v = 1; v<=MAXVOICES; v++)
						if (doc->voiceTab[v].partn==partn)
							lastUsedV = n_max(lastUsedV, doc->voiceTab[v].relVoice);
					doc->voiceTab[iVoice].partn = partn;
					doc->voiceTab[iVoice].voiceRole = SINGLE_DI;
					doc->voiceTab[iVoice].relVoice = lastUsedV+1;
				}
				break;
			default:
				break;
		}
}


/* ------------------------------------------------------------ BuildVoiceTable -- */
/* Create, from scratch, the table that maps internal voice numbers to combinations
of part number and user voice number and specifies voices' <voiceRole> (upper, lower,
etc.). Should not be used to update an existing voice mapping table because it sets
every <voiceRole> to default and because it may reassign user voice numbers and
thereby confuse people. */

void BuildVoiceTable(Document *doc, Boolean defaults)
{
	short	v, partn, s;
	LINK	partL;
	PPARTINFO pPart;
	Boolean dummy[MAXVOICES+1];
	
	/* Clear out table, then, if <defaults>, fill in default voices with nos. the
		same as staff nos. */
	
	for (v = 1; v<=MAXVOICES; v++)
		doc->voiceTab[v].partn = 0;
	
	if (defaults) {
		partL = NextPARTINFOL(FirstSubLINK(doc->headL));
		for (partn = 1; partL; partn++, partL=NextPARTINFOL(partL)) {
			pPart = GetPPARTINFO(partL);
			
			if (pPart->firstStaff<1 || pPart->firstStaff>doc->nstaves
			||  pPart->lastStaff<1 || pPart->lastStaff>doc->nstaves) {
				MayErrMsg("BuildVoiceTable: part=%ld firstStaff=%ld or lastStaff=%ld is illegal.",
							(long)partn, (long)pPart->firstStaff, (long)pPart->lastStaff);
				return;
			}
			
			for (s = pPart->firstStaff; s<=pPart->lastStaff; s++) {
				doc->voiceTab[s].partn = partn;
				doc->voiceTab[s].voiceRole = SINGLE_DI;
				doc->voiceTab[s].relVoice = s-pPart->firstStaff+1;
			}
		}
	}
		
	FillVoiceTable(doc, dummy);

}


/* ---------------------------------------------------------------- MapVoiceNums -- */

void MapVoiceNums(Document *doc, short newVoiceTab[])
{
	LINK pL, aNoteL, aGRNoteL;
	
	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
					NoteVOICE(aNoteL) = newVoiceTab[NoteVOICE(aNoteL)];
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
					GRNoteVOICE(aGRNoteL) = newVoiceTab[GRNoteVOICE(aGRNoteL)];
				break;
			case BEAMSETtype:
				BeamVOICE(pL) = newVoiceTab[BeamVOICE(pL)];
				break;
			case TUPLETtype:
				TupletVOICE(pL) = newVoiceTab[TupletVOICE(pL)];
				break;
			case GRAPHICtype:
				if (GraphicVOICE(pL)>0)
					GraphicVOICE(pL) = newVoiceTab[GraphicVOICE(pL)];
				break;
			case SLURtype:
				SlurVOICE(pL) = newVoiceTab[SlurVOICE(pL)];
				break;
			default:
				;
		}
	}
}


/* ------------------------------------------------------------ CompactVoiceNums -- */
/* Update voice numbers of objects in the Document so there are no gaps and
create a voice-mapping table that reflects the new situation. Does not assume
the existing voice-mapping table is accurate. */
 
static void CompactVoiceNums(Document *doc)
{
	short newVoiceTab[MAXVOICES+1], v, vNew;
	
	/* Construct a special table that maps existing to desired voice numbers,
		closing up any gaps. If we close any gaps, that will make the standard
		voice-mapping table wrong, so we update it again at the end of the end
		of this function. */
		
	for (vNew = v = 1; v<=MAXVOICES; v++)
		newVoiceTab[v] = (doc->voiceTab[v].partn>0? vNew++ : 0);
			
	/* Use the special table to change voice numbers throughout the object list. */
	MapVoiceNums(doc, newVoiceTab);
	
	/* Compact the voice table and adjust Looked-at voice no. to match. */
	for (v = 1; v<=MAXVOICES; v++)
	if (newVoiceTab[v]>0 && newVoiceTab[v]!=v) {
		doc->voiceTab[newVoiceTab[v]] = doc->voiceTab[v];
		doc->voiceTab[v].partn = 0;							/* Mark the old slot as empty */
	}
	if (doc->lookVoice>=0) doc->lookVoice = newVoiceTab[doc->lookVoice];
}


/* ------------------------------------------------------------ UpdateVoiceTable -- */
/* Update the table that maps internal voice numbers to combinations of part number
and user voice number and specifies voices' <voiceRole> (upper, lower, etc.). Preserves
<voiceRole> and user voice numbers of voices in the table that are still in use. */

void UpdateVoiceTable(Document *doc, Boolean defaults)
{
	short v;
	Boolean maybeEmpty[MAXVOICES+1];
	
	/* Set all slots to "maybe empty" except (if <defaults>) those for default voices,
		and the slot for the Looked at voice, if there is one. Then fill in voices
		actually used in the object list. */
	
	for (v = 1; v<=MAXVOICES; v++)
		maybeEmpty[v] = (!defaults || v>doc->nstaves);
		
	if (doc->lookVoice>=0) maybeEmpty[doc->lookVoice] = FALSE;

	FillVoiceTable(doc, maybeEmpty);			

	/* Clear all slots that are still marked "maybe empty"--they aren't in use--and
	 * close up any gaps in the table this might have produced. */
		
	for (v = 1; v<=MAXVOICES; v++)
		if (maybeEmpty[v]) doc->voiceTab[v].partn = 0;
	CompactVoiceNums(doc);

}


/* --------------------------------------------------------------- User2IntVoice -- */
/* Return the voice-mapping table slot number (the internal voice number) for the
given user voice number and part. If the voice-mapping table doesn't have an entry
for the user-voice-number/part combination, also put it into the first empty slot,
if there is one. If it needs to add an entry but there are no empty slots, return
0.  N.B. Assumes there no entries in the table after the first blank slot! */

short User2IntVoice(Document *doc, short uVoice, LINK addPartL)
{
	short partn, v, voiceSlot;
	LINK partL;
	
	partL = NextPARTINFOL(FirstSubLINK(doc->headL));
	for (partn = 1; partL; partn++, partL=NextPARTINFOL(partL))
		if (partL==addPartL) break;

	for (voiceSlot = 0, v = 1; v<=MAXVOICES; v++) {
		if (doc->voiceTab[v].partn==partn
		&&  doc->voiceTab[v].relVoice==uVoice) {
			voiceSlot = v;
			break;
		}
		if (doc->voiceTab[v].partn<=0) {				/* Voice not in the table; add it */
			doc->voiceTab[v].partn = partn;
			doc->voiceTab[v].voiceRole = SINGLE_DI;
			doc->voiceTab[v].relVoice = uVoice;
			voiceSlot = v;
			break;
		}
	}
	return voiceSlot;
}
 

/* --------------------------------------------------------------- Int2UserVoice -- */
/* Return the user voice number and part corresponding to the given internal voice
number, and return TRUE. We get the information from the voice-mapping table; if the
table has no entry for the voice/part combination, return FALSE. */

Boolean Int2UserVoice(Document *doc, short iVoice, short *puVoice, LINK *pPartL)
{
	short partn;

	partn = doc->voiceTab[iVoice].partn;
	if (partn<=0) return FALSE;
	
	*puVoice = doc->voiceTab[iVoice].relVoice;
	*pPartL = FindPartInfo(doc, partn);
	if (*pPartL==NILINK) return FALSE;
			
	return TRUE;
}


/* ----------------------------------------------------------------- NewVoiceNum -- */
/* Find a "new" voice number for the given part: the lowest unused voice number
greater than the number of staves in that part (since we reserve voice numbers 1
thru n for default voice numbers on staves 1 thru n). */

short NewVoiceNum(Document *doc, LINK partL)
{
	PPARTINFO pPart;
	short partn, freeVoice, oldFreeVoice, v;
	
	pPart = GetPPARTINFO(partL);
	partn = Staff2Part(doc, pPart->firstStaff);
	freeVoice = pPart->lastStaff-pPart->firstStaff+2;
	
	do {
		oldFreeVoice = freeVoice;
		for (v = 1; v<=MAXVOICES; v++)
			if (doc->voiceTab[v].partn==partn)
				if (doc->voiceTab[v].relVoice==freeVoice)
					freeVoice++;
	} while (freeVoice!=oldFreeVoice);
	
	return freeVoice;
}


/* --------------------------------------------------------------- CountVoices -- */

/* Return the number of voices in use in the given document. */
short CountVoices(Document *doc) 
{
	short nv = 0;
	
	/* If the voice table is legal, we could stop counting when we find the first
		empty slot; but continuing to count all the way to the end might be more
		robust. Anyway, it shouldn't hurt.
	 */
	for (int i = 0; i <= MAXVOICES; i++) {
		if (doc->voiceTab[i].partn != 0) nv++;
	}
	
	return nv;
}


