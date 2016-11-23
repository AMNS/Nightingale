/***************************************************************************
*	FILE:	SpaceTime.c
*	PROJ:	Nightingale
*	DESC:	Low- and medium-level space and time routines (welcome to the
* 			Twilight Zone...). No user-interface assumptions.
*
	FillRelStaffSizes
	SymWidthLeft			SymWidthRight				SymLikelyWidthRight
	SymDWidthLeft			SymDWidthRight				ConnectDWidth
	GetClefSpace			GetTSWidth
	GetKSWidth				KSDWidth					GetKeySigWidth
	FillSpaceMap
	FIdealSpace				IdealSpace					CalcSpaceNeeded
	MeasSpaceProp			SetMeasSpacePercent			GetMSpaceRange
	LDur2Code				
	Code2LDur				SimpleLDur					SimpleGRLDur
	GetMinDur				TupletTotDir				GetDurUnit
	GetMaxDurUnit
	TimeSigDur				CalcNoteLDur				SyncMaxDur
	SyncNeighborTime		GetLTime					GetSpaceInfo
	FixStaffTime			FixVoiceTimes				GetSpTimeInfo
	FixMeasTimeStamps		FixTimeStamps				GetLDur
	GetVLDur				GetMeasDur					GetTimeSigMeasDur
	GetStaffMeasDur			WholeMeasRestIsBreve

/***************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Note: "Logical duration" herein refers to Nightingale's internal representation
of a note or rest's symbolic duration, given in the same units as play duration:
"PDUR ticks".  Cf. the definition of PDURUNIT. Thus, a triplet note of the shortest
possible duration would have a logical duration of 2/3 of PDURUNIT.


/* Prototypes for functions local to this file */

static void GetSpaceInfo(Document *, LINK, LINK, short, SPACETIMEINFO []);


/* ------------------------------------------------------------ Fill various tables -- */

/* Fill in doc's table of staff sizes. If any staff is not in its "standard" size,
return TRUE, else return FALSE. */
Boolean FillRelStaffSizes(Document *doc)
{
	LINK staffL, aStaffL; short stf; DDIST lnSpace; Boolean nonstandard=FALSE;
	
	staffL = SSearch(doc->headL, STAFFtype, GO_RIGHT);				/* Should never fail */
	for (aStaffL = FirstSubLINK(staffL); aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		lnSpace = StaffHEIGHT(aStaffL) / (StaffSTAFFLINES(aStaffL)-1);
		stf = StaffSTAFF(aStaffL);
		doc->staffSize[stf] = lnSpace*(STFLINES-1);
		if (doc->staffSize[stf]!=drSize[doc->srastral]) nonstandard = TRUE;
	}
	
	return nonstandard;
}		


/* -------------------------------------------------- SymWidthLeft, SymWidthRight -- */
/* Return the horizontal space <pL> really occupies, i.e., the minimum required
to avoid overwriting, for the given staff only or for all staves. SymWidthLeft
returns width to left of symbol origin, SymWidthRight to right of symbol origin.

We accomodate different size staves by using doc->srastral as the "reference size"
and scaling distances on other-size staves as we go; so we always return a value
in STDIST units for the reference size, even if <staff> is a different size. */

#define STF_SCALE(val, s)	(doc->staffSize[s]*(long)(val)/(long)drSize[doc->srastral])

STDIST SymWidthLeft(
			Document *doc,
			LINK pL,
			short staff,		/* Number of staff to consider, or ANYONE for all staves */
			short measNode)		/* Index into _ignoreChord_, or -1 = have no context info (unused) */ 
{
	STDIST		totWidth;
	PAMEASURE	aMeas;
	LINK 		aNoteL, aGRNoteL, aMeasL;
	short		xmoveAcc, maxxmoveAcc, s;
	Boolean		anyStaff, noteToLeft;
	
	anyStaff = (staff==ANYONE);
	if (!anyStaff && STAFFN_BAD(doc, staff)) return 0;						/* Sanity check */

	switch (ObjLType(pL)) {
		case SYNCtype:
			if (measNode>=0) LogPrintf(LOG_NOTICE, "(a) ignoreChord[1]=%d [2]=%d [3]=%d\n",
					ignoreChord[measNode][1], ignoreChord[measNode][2], ignoreChord[measNode][3]);

			noteToLeft = FALSE;
			/* ??PROBLEM: We pass the staff no. to ChordNoteToLeft(), but it expects
				voice no.! I tried to fix this, but the result waas a crash _every_
				time Respacing was done. And giving it staff no. instead of voice no.
				seems to work rather well! I dunno. */
			if (anyStaff) {
				for (s = 1; s<=doc->nstaves; s++)
					if (!ChordNoteToLeft(pL, s)) { noteToLeft = TRUE; break; }
			}
			else
				if (ChordNoteToLeft(pL, staff)) noteToLeft = TRUE;

			maxxmoveAcc = -1;
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				if ((anyStaff || NoteSTAFF(aNoteL)==staff) && NoteACC(aNoteL)!=0) {
					xmoveAcc = (NoteACC(aNoteL)==AC_DBLFLAT?
											NoteXMOVEACC(aNoteL)+2 : NoteXMOVEACC(aNoteL));
					if (doc->nonstdStfSizes) xmoveAcc = STF_SCALE(xmoveAcc, NoteSTAFF(aNoteL));
					if (NoteCOURTESYACC(aNoteL))
						xmoveAcc += config.courtesyAccLXD;
					maxxmoveAcc = n_max(maxxmoveAcc, xmoveAcc);
				}
			}
	  		if (maxxmoveAcc>=0) {
				/*
				 *	Ordinarily, the accidental position is relative to a note on the "normal"
				 *	side of the stem. But if note is in a chord that's downstemmed and has
				 *	notes to the left of the stem, its accidental is moved to the left. (The
				 *	following adjustments should really take into account STF_SCALE, but we
				 *	can't just do it for the staff <xmoveAcc> is on because another staff
				 *	might be the one with <noteToLeft>. Someday. )
				 */
				if (noteToLeft) maxxmoveAcc += 4;

	  			totWidth = STD_ACCWIDTH;
	  			
	  			/* Add a little for roundoff error. */
	  			totWidth += STD_LINEHT/8;
	  			
	  			totWidth += (STD_ACCWIDTH*(maxxmoveAcc-DFLT_XMOVEACC))/4;
	  			return totWidth;
	  		}
		  	else
		  		return (noteToLeft? STD_LINEHT : 0);
		
		case GRSYNCtype:
			maxxmoveAcc = -1;
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
				if ((anyStaff || GRNoteSTAFF(aGRNoteL)==staff) && GRNoteACC(aGRNoteL)!=0) {
					xmoveAcc = (GRNoteACC(aGRNoteL)==AC_DBLFLAT?
										GRNoteXMOVEACC(aGRNoteL)+1 : GRNoteXMOVEACC(aGRNoteL));
					if (doc->nonstdStfSizes) xmoveAcc = STF_SCALE(xmoveAcc, GRNoteSTAFF(aGRNoteL));
					if (GRNoteCOURTESYACC(aGRNoteL))
						xmoveAcc += config.courtesyAccLXD;


					maxxmoveAcc = n_max(maxxmoveAcc, xmoveAcc);
				}
			}
	  		if (maxxmoveAcc>=0) {
#ifdef NOTYET
				/*
				 *	Ordinarily, the accidental position is relative to a note on the "normal"
				 *	side of the stem. But if grace note is in a chord that's downstemmed and
				 *	has notes to the left of the stem, its accidental is moved to the left.
				 * FIXME: ChordNoteToLeft DOESN'T KNOW ABOUT GRACE NOTES. ALSO, noteToLeft
				 * SHOULD BE CONSIDERED REGARDESS OF ACCS.--CF. case SYNCtype ABOVE.
				 */
				if (ChordNoteToLeft(pL, aGRNote->staffn)) maxxmoveAcc += 4;
#endif

	  			totWidth = STD_ACCWIDTH;
	  			totWidth += (STD_ACCWIDTH*(maxxmoveAcc-DFLT_XMOVEACC))/4;
	  			return GRACESIZE(totWidth);
	  		}
		  	else
		  		return 0;
		
		case MEASUREtype:
			/*
			 *	Though Measures (barlines) must have subobjects on all staves, they can be
			 * be hidden on some, so perhaps we should look at all staves and take the
			 * staff sizes of the visible ones into account, but I doubt it's worth it--
			 *	anyway, consistency among Measures is desirable. The same thing applies
			 *	to Pseudomeasures.
			 */ 
			aMeasL = FirstSubLINK(pL);
			aMeas = GetPAMEASURE(aMeasL);
			if (aMeas->subType==BAR_RPT_R || aMeas->subType==BAR_RPT_LR)
				return (3*STD_LINEHT)/4;
			else
				return STD_LINEHT/8;

		case PSMEAStype:
			return STD_LINEHT/8;

		case RPTENDtype:							/* To allow for dots, give it more than a barline gets. */
			return STD_LINEHT/2;

		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case SPACERtype:
		case ENDINGtype:
		default:
			return 0;
	}
}


STDIST SymWidthRight(
			Document *doc,
			LINK	pL,
			short	staff,		/* Number of staff to consider, or ANYONE for all staves */
			Boolean	toHead 		/* For notes/grace notes, ignore stuff to right of head? */
			)
{
	STDIST		totWidth, width, nwidth, normWidth;
	PANOTE		aNote;
	PAGRNOTE	aGRNote;
	PACLEF		aClef;
	PAKEYSIG 	aKeySig;
	PATIMESIG	aTimeSig;
	PAMEASURE	aMeasure;
	PAPSMEAS	aPSMeas;
	PSPACER		pSpace;
	LINK		aNoteL, aClefL, aKeySigL, aTimeSigL, aMeasureL, aPSMeasL, aGRNoteL;
	short		nChars, s;
	Boolean		anyStaff, wideChar, noteToRight;
	CONTEXT		context;
	DRect		dRestBar;
	
	anyStaff = (staff==ANYONE);
	if (!anyStaff && STAFFN_BAD(doc, staff)) return 0;						/* Sanity check */

	switch (ObjLType(pL)) {
	 case SYNCtype:
		totWidth = 0;
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
			if (anyStaff || NoteSTAFF(aNoteL)==staff) {
				if (NoteType(aNoteL)<WHOLEMR_L_DUR) {
					GetContext(doc, pL, (anyStaff? 1 : staff), &context);
					GetMBRestBar(-NoteType(aNoteL), &context, &dRestBar);
					nwidth = d2std(dRestBar.right-dRestBar.left, context.staffHeight,
										context.staffLines);
					if (doc->nonstdStfSizes) nwidth = STF_SCALE(nwidth, (anyStaff? 1 : staff));
				}
				/*
				 * If the note/rest has any dots and we're considering stuff after the
				 * notehead, the dots determine its width; otherwise, if it's stem
				 * up and has flags, the flags determine its width. (Grace notes are
				 * the same except that they can't have dots.)
				 */
			  	else if (NoteNDOTS(aNoteL)==0 || toHead) {
					aNote = GetPANOTE(aNoteL);
					if (doNoteheadGraphs)
			  			nwidth = NOTEHEAD_GRAPH_WIDTH*(STD_LINEHT*4)/3;
					else
			  			nwidth = (STD_LINEHT*4)/3;
					if (!aNote->rest && !aNote->beamed			/* Is it a note, unbeamed, of */						
					&&  NFLAGS(aNote->subType)>0					/*   flag-needing duration, */  
					&&  aNote->yd>aNote->ystem						/*   stem up, */
					&&  !toHead)									/* and we're considering flags? */
					/* In line below, STD_LINEHT is too little, STD_LINEHT*4/3 too much. */
						nwidth += (STD_LINEHT*7)/6; 				/* Yes, allow space for flags */
					if (doc->nonstdStfSizes) nwidth = STF_SCALE(nwidth, aNote->staffn);
				}
			  	else {												/* Include aug. dots. */
					aNote = GetPANOTE(aNoteL);
					if (doNoteheadGraphs)
			  			nwidth = NOTEHEAD_GRAPH_WIDTH*(STD_LINEHT*2)+2;	/* For 1st dot default pos. */
					else
			  		nwidth = (STD_LINEHT*2)+2;						/* For 1st dot default pos. */

			  		nwidth += (STD_LINEHT*(aNote->xmovedots-3))/4;	/* Fix for 1st dot actl.pos. */
			  		if (aNote->ndots>1)
			  			nwidth += STD_LINEHT*(aNote->ndots-1);		/* Fix for additional dots */
					if (doc->nonstdStfSizes) nwidth = STF_SCALE(nwidth, aNote->staffn);
			  	}
			  	totWidth = n_max(totWidth, nwidth);
	  		}
	  	}

		/*
		 *	If chord is upstemmed and has notes to the right of the stem, it extends
		 *	further to the right than it otherwise would. (The following adjustments
		 * should really take into account STF_SCALE. Someday.)
		 */
		noteToRight = FALSE;
		if (anyStaff) {
			for (s = 1; s<=doc->nstaves; s++)
				if (ChordNoteToRight(pL,s)) { noteToRight = TRUE; break; }
		}
		else
			if (ChordNoteToRight(pL,staff)) noteToRight = TRUE;
		if (noteToRight) totWidth += STD_LINEHT;

	  	return totWidth;

	 case GRSYNCtype:
		totWidth = 0;
		aGRNoteL = FirstSubLINK(pL);
		for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
			aGRNote = GetPAGRNOTE(aGRNoteL);
			if (anyStaff || aGRNote->staffn==staff) {
		  		nwidth = (STD_LINEHT*4)/3;
				if (!aGRNote->beamed							/* Is it unbeamed, of */						
				&&  NFLAGS(aGRNote->subType)>0					/*   flag-needing duration, */  
				&&  aGRNote->yd>aGRNote->ystem					/*   stem up, */
				&&  !toHead)									/* and we're considering flags? */
					nwidth += (STD_LINEHT*2)/3; 				/* Yes, allow space for flags */
				if (doc->nonstdStfSizes) nwidth = STF_SCALE(nwidth, aGRNote->staffn);
			  	totWidth = n_max(totWidth, nwidth);
	  		}
	  	}
#ifdef NOTYET
		/*
		 *	If chord is upstemmed and has grace notes to the right of the stem, it extends
		 *	further to the right than it otherwise would. FIXME: ChordNoteToRight DOESN'T
		 *	KNOW ABOUT GRACE NOTES.
		 */
		noteToRight = FALSE;
		if (anyStaff) {
			for (s = 1; s<=doc->nstaves; s++)
				if (ChordNoteToRight(pL,s)) { noteToRight = TRUE; break; }
		}
		else
			if (ChordNoteToRight(pL,staff)) noteToRight = TRUE;
		if (noteToRight) totWidth += STD_LINEHT;
#endif

	  	return GRACESIZE(totWidth)+STD_LINEHT/3;

	 case MEASUREtype:
		/*
		 *	Though Measures (barlines) must have subobjects on all staves, they can be
		 * be hidden on some, so perhaps we should look at all staves and take the
		 * staff sizes of the visible ones into account, but I doubt it's worth it--
		 *	anyway, consistency among Measures is desirable. The same thing applies
		 *	to Pseudomeasures.
		 */ 
		aMeasureL = FirstSubLINK(pL);
		aMeasure = GetPAMEASURE(aMeasureL);
		switch (aMeasure->subType) {
			case BAR_SINGLE:
				return 0;
			case BAR_DOUBLE:
				return STD_LINEHT/2;
			case BAR_FINALDBL:
				return (2*STD_LINEHT)/2;
			case BAR_RPT_L:
				return (11*STD_LINEHT)/8;
			case BAR_RPT_R:
				return (8*STD_LINEHT)/8;
			case BAR_RPT_LR:
				return (11*STD_LINEHT)/8;
			default:
				return 0;
		}

	 case PSMEAStype:
		aPSMeasL = FirstSubLINK(pL);
		aPSMeas = GetPAPSMEAS(aPSMeasL);
		switch (aPSMeas->subType) {
			case PSM_DOTTED:
			case PSM_DOUBLE:
				return STD_LINEHT/2;
			case PSM_FINALDBL:
				return (2*STD_LINEHT)/2;
			default:
				return 0;
		}

	 case RPTENDtype:
		return (5*STD_LINEHT)/2;

	 case CLEFtype:
		normWidth = .85*STD_LINEHT*4;
		totWidth = 0;
		aClefL = FirstSubLINK(pL);
		for ( ; aClefL; aClefL=NextCLEFL(aClefL)) {
			aClef = GetPACLEF(aClefL);
			if (anyStaff || aClef->staffn==staff) {
				aClef = GetPACLEF(aClefL);
				width = (aClef->small? SMALLSIZE(normWidth) : normWidth);
#ifdef NOTYET_FIXDRAWBUG
				if (doc->nonstdStfSizes) width = STF_SCALE(width, aClef->staffn);
#endif
  				totWidth = n_max(totWidth, width);
  			}
  		}
		return totWidth;

	 case KEYSIGtype:
		{
			short nAcc, stdWidth;
			
			aKeySigL = FirstSubLINK(pL);
			for ( ; aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL)) {
				aKeySig = GetPAKEYSIG(aKeySigL);
				if (anyStaff || aKeySig->staffn==staff) {
					aKeySig = GetPAKEYSIG(aKeySigL);
					nAcc = (aKeySig->nKSItems>0? aKeySig->nKSItems : aKeySig->subType);
					stdWidth = STD_ACCWIDTH;
					if (nAcc>1) stdWidth += (nAcc-1)*STD_ACCWIDTH;
					stdWidth += STD_LINEHT/2;
					if (doc->nonstdStfSizes) stdWidth = STF_SCALE(stdWidth,aKeySig->staffn);
					return stdWidth;
				}
			}
		}
		return 0;															/* Nothing for this staff */

	 case TIMESIGtype:
	 	nChars = 1;
	 	wideChar = FALSE;
		aTimeSigL = FirstSubLINK(pL);
		for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
			aTimeSig = GetPATIMESIG(aTimeSigL);
			if (anyStaff || aTimeSig->staffn==staff) {
			 	if (aTimeSig->subType==N_ONLY || aTimeSig->subType==N_OVER_D) {
			 		if (aTimeSig->numerator>=10) nChars = 2;
					if (aTimeSig->subType==N_OVER_D)
			 			if (aTimeSig->denominator>=10) nChars = 2;
				}
				else if (aTimeSig->subType==C_TIME || aTimeSig->subType==CUT_TIME)
					wideChar = TRUE;
			}
		}
		if (nChars==1 && wideChar) nwidth = 2*STD_LINEHT;
		else								nwidth = (nChars*3*STD_LINEHT)/2;
		if (doc->nonstdStfSizes) nwidth = STF_SCALE(nwidth, aTimeSig->staffn);
		return nwidth;

	 case SPACERtype:
	 	pSpace = GetPSPACER(pL);
	 	return pSpace->spWidth;

	 default:
		return 0;
	}
}


/* ---------------------------------------------------------- SymLikelyWidthRight -- */
/* SymLikelyWidthRight delivers a guess as to the space to right of symbol origin the
given object needs. For most types of objects, it simply uses the graphic width; for
Syncs, it uses the maximum of the Sync's ideal space (based on duration) and its graphic
width; if it's a Measure, it uses the maximum of the standard spacing after barlines
and the graphic width. N.B. This is only a guess, though usually a good one: the
only way to determine the correct spacing is the way Respace1Bar does. */

STDIST SymLikelyWidthRight(
				Document *doc,
				LINK pL,
				long spaceProp 			/* Use spaceProp/(RESFACTOR*100) of normal spacing */
				)
{
	STDIST idealSp, width;
	
	pL = FirstValidxd(pL, GO_LEFT);
	width = SymWidthRight(doc, pL, ANYONE, TRUE);
	
	switch (ObjLType(pL)) {
		case SYNCtype:
			idealSp = IdealSpace(doc, SyncMaxDur(doc, pL), spaceProp);
			width = n_max(idealSp, width);
			break;
		case MEASUREtype:
			width = n_max(config.spAfterBar, width);
			break;
		default:
			;
	}
	
	return width;
}


/* ----------------------------------------------- SymDWidthLeft, SymDWidthRight -- */
/* Returns DDIST symWidthRight, symWidthLeft. */

DDIST SymDWidthLeft(Document *doc, LINK pL, short staff, CONTEXT context)
{
	return std2d(SymWidthLeft(doc, pL, staff, -1), context.staffHeight,
						context.staffLines);
}


DDIST SymDWidthRight(Document *doc, LINK pL, short staff, Boolean toHead, CONTEXT context)
{
	return std2d(SymWidthRight(doc, pL, staff, toHead),
						context.staffHeight, context.staffLines);
}


/* --------------------------------------------------------------- ConnectDWidth -- */

DDIST ConnectDWidth(short srastral, char connectType)
{
	DDIST dLineSp, width;
	
	dLineSp = drSize[srastral]/(STFLINES-1);					/* Space between staff lines */

	switch (connectType) {
		case CONNECTLINE:
			width = 0;
			break;
		case CONNECTCURLY:
			width = (7*dLineSp)/4;
			break;
		case CONNECTBRACKET:
			width = (4*dLineSp)/4;
			break;
	}
 	return width;
}


/* -------------------------------------------------------- GetClefSpace,GetTSWidth -- */
/* Special-purpose functions used by routines to insert and delete before the 1st bar.
FIXME: Accuracy of both is dependent on screen resolution; GetClefSpace's accuracy
further depends on staff size--yeech! But it should be easy to fix these problems. */

/* Returns INITKS_SP plus the width of the charRect of the treble clef. INITKS_SP
is the difference between width of clef and space between the initial clef and
timesig. When called by BeforeFirst clef insertion routines, uses the exact opposite
of this number. */

#define INITKS_SP		p2d(7)	/* FIXME: WRONG! MUST DEPEND ON STAFF HEIGHT */

DDIST GetClefSpace(LINK /*clefL*/)		/* argument is unused */
{
	DDIST clefWidth,change; Rect r;

	r = CharRect(MCH_trebleclef);
	clefWidth = r.right-r.left;
	change = p2d(clefWidth)+INITKS_SP;

	return change;
}


/* Return width of the given time sig. object: get the maximum numeral displayed by any
of timeSigL's subobjects, and return its width in DDISTs. */

DDIST GetTSWidth(LINK timeSigL)
{
	LINK aTimeSigL; PATIMESIG aTimeSig;
	short num,tsNum,tsDenom,prevNum=0,timeSigWidth;
	Str31 nStr;

 	/* Get maximum numeral of either timeSig numerator, denominator,
 		or numeral displayed by other timeSig type of all timeSig
 		subObjs in the timeSig object. */
 
 	aTimeSigL = FirstSubLINK(timeSigL);
 	for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
		aTimeSig = GetPATIMESIG(aTimeSigL);
 		switch (aTimeSig->subType) {
			case N_OVER_D:
				tsNum = aTimeSig->numerator;
				tsDenom = aTimeSig->denominator;
				num = n_max(tsNum,tsDenom);
				break;
			case C_TIME:
			case CUT_TIME:
				num = 2;
				break;
			case N_ONLY:
				tsNum = aTimeSig->numerator;
				break;
			case ZERO_TIME:
			case N_OVER_QUARTER:
			case N_OVER_EIGHTH:
			case N_OVER_HALF:
			case N_OVER_DOTTEDQUARTER:
			case N_OVER_DOTTEDEIGHTH:
				num = 2;
				break;
 		}
 		num = n_max(num,prevNum);
 		prevNum = num;
 	}
 	
 	/* Return string width of this maximum numeral in DDISTs */
 
	NumToString(num, nStr);
	timeSigWidth = StringWidth(nStr);	/* ??MUCH better: NPtStringWidth, then pt2d */

	return (p2d(timeSigWidth));
}


/* ------------------------------------------------------ Get Keysig Width Functions -- */
/* ??Cf. KSDWidth and GetKeySigWidth to GetKSWidth and SymWidthRight: these functions
should too. But much better, there should be only one way to get keysig width for a
subobj and only one for an object, and it should use STD_ACCWIDTH and STD_ACCSPACE. */

#define STD_ACCSPACE STD_ACCWIDTH	/* Space between accs. in key sig. is the same as their width */

/* Get the width of the given key signature subobject, in pixels. */

short GetKSWidth(LINK aKeySigL, DDIST staffHeight, short staffLines)
{
	PAKEYSIG aKeySig; short nAcc; short stdWidth; DDIST dWidth;

	aKeySig = GetPAKEYSIG(aKeySigL);
	nAcc = (aKeySig->nKSItems>0? aKeySig->nKSItems : aKeySig->subType);
	
	stdWidth = STD_ACCWIDTH;
	if (nAcc>1) stdWidth += (nAcc-1)*STD_ACCSPACE;
	dWidth = std2d(stdWidth, staffHeight, staffLines);

	return d2p(dWidth);
}


/* Get the width of the given key signature subobject, in DDISTs. ??Should use
STD_ACCWIDTH. */

DDIST KSDWidth(LINK aKeySigL, CONTEXT context)
{
	PAKEYSIG aKeySig; STDIST width;

	aKeySig = GetPAKEYSIG(aKeySigL);
	width = .4*aKeySig->nKSItems*STD_LINEHT*4;
	return (std2d(width,context.staffHeight,context.staffLines));
}

/* Get the width of the given key signature object, in DDISTs. ??Treatment of the staff
no. argument is a mess: it's ignored unless it's ANYONE, but if it is, this may not give
the correct results! Surely, if staffn==ANYONE, should return max. width, else width on
the given staff, but in that case, why not just use SymDWidthRight? */

DDIST GetKeySigWidth(
			Document *doc,
			LINK keySigL,
			short staffn)			/* ??ignored unless it's ANYONE: see above */
{
	LINK aKeySigL; CONTEXT context; DDIST KSWidth=-9999;

	if (staffn==ANYONE) {
		/* ??This assumes all staves have same width keysig: this may not be correct. */
		aKeySigL = FirstSubLINK(keySigL);
		GetContext(doc, keySigL, KeySigSTAFF(aKeySigL), &context);
		return (KSDWidth(aKeySigL,context));
	}

	/* Get the max. width of any subobject, *not* of the one on <staffn>! */
	
	aKeySigL = FirstSubLINK(keySigL);
	for ( ; aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL)) {
		GetContext(doc, keySigL, KeySigSTAFF(aKeySigL), &context);
		KSWidth = n_max(KSWidth,KSDWidth(aKeySigL,context));
	}
	return KSWidth;
}


/* ------------------------------------------------------ FillSpaceMap and helper -- */
/* This routine installs the whichTable'th spacing table. If whichTable is positive,
we use it as the resource ID of a 'SPTB' resource containing the spacings coded
as longs each containing 100 times the desired value.  If If whichTable is 0 or we
can't get the resource, then we use the precompiled table here. No checking is done to
ensure that the resource is the proper size or that its contents are reasonable. */

static void SpaceMapErr(void);
static void SpaceMapErr()
{
	GetIndCString(strBuf, MISCERRS_STRS, 3);	/* "Can't get the requested space table" */
	CParamText(strBuf, "", "", "");
	NoteInform(GENERIC_ALRT);
}

/* Ideal spacing of basic (undotted, non-tuplet) durations:
	 128th 64th  32th  16th   8th    qtr    half   whole  breve */
FASTFLOAT dfltSpaceMap[MAX_L_DUR] =
	{.625, 1.0, 1.625, 2.50,  3.75,  5.50,  8.00,  11.5,  16.25 };	/* new Fib/sqrt(2) */

void FillSpaceMap(Document *doc, short	whichTable)
{
	short i, saveResFile; Boolean useDefault=TRUE; Handle rsrc;
	
	saveResFile = CurResFile();
	UseResFile(setupFileRefNum);
	{
		doc->spaceTable = whichTable;
		if (whichTable > 0) {
			rsrc = GetResource('SPTB', whichTable);
			if (!GoodResource(rsrc))	SpaceMapErr();
			else						useDefault = FALSE;
		}
	}

	for (i=0; i<MAX_L_DUR; i++) 
		doc->spaceMap[i] = STD_LINEHT * (useDefault ? dfltSpaceMap[i] :
												((long *)(*rsrc))[i] / 100.0);
		UseResFile(saveResFile);
}


/* ------------------------------------------------------ FIdealSpace,IdealSpace -- */

/* Return fine "ideal" horizontal space for the given duration and spacing. */

short FIdealSpace(
			Document *doc,
			long dur,				/* dur is physical in (128*PDURUNIT)ths */
			long spaceProp 		/* Use spaceProp/(RESFACTOR*100) of normal spacing */
			)
{
	long space, tsp;
	short i, two2i, lastTwo2i;
	FASTFLOAT hScale;

	hScale = spaceProp;
	hScale /= RESFACTOR*100L;
		
	dur /= PDURUNIT;													/* p_dur code for shortest note */

	if (dur<=0) return 0;
	
	for (lastTwo2i = 0, two2i=1, i = 0; i<MAX_L_DUR; i++) {
		if (dur<two2i) {												/* Dur. less than this entry? */
			tsp = InterpY(lastTwo2i, doc->spaceMap[i-1], two2i, doc->spaceMap[i], dur);
			space = FIDEAL_RESOLVE*hScale*tsp;
			return space;
		}
		else if (dur==two2i)	{										/* Dur. correct for this entry? */
			space = FIDEAL_RESOLVE*hScale*doc->spaceMap[i];
			return space;
		}
		else {
			lastTwo2i = two2i;
			two2i *= 2;
		}
	}
	
	space = FIDEAL_RESOLVE*hScale*doc->spaceMap[MAX_L_DUR-1];	/* Dur. longer than longest table entry */
	return space;
}

/* Return "ideal" horizontal space for the given duration and spacing. */

STDIST IdealSpace(
				Document *doc,
				long dur,				/* dur is physical in (128*PDURUNIT)ths */
				long spaceProp			/* Use spaceProp/(RESFACTOR*100) of normal spacing */
				)
{
	return FIdealSpace(doc, dur, spaceProp)/10;
}


/* ------------------------------------------------------------- CalcSpaceNeeded -- */
/* Calculate the amount of space needed by the last object on the page before pL.
??Should probably call SymLikelyWidthRight instead of doing most of the work
itself. */

DDIST CalcSpaceNeeded(Document *doc, LINK pL)
{
	LINK		beforeL;
	LINK		pSubL;
	long		maxLen,tempLen;
	short		noteStaff;
	CONTEXT	context;
	STDIST	symWidth,space;
	
	if (!(PageTYPE(pL) || SystemTYPE(pL) || TailTYPE(pL)))
		MayErrMsg("CalcSpaceNeeded: link %ld is of the wrong type %ld",
					(long)pL, (long)ObjLType(pL));

	beforeL = FirstValidxd(LeftLINK(pL),TRUE);
	switch(ObjLType(beforeL)) {
		case SYNCtype:
				maxLen = space = 0L;
				pSubL = FirstSubLINK(beforeL);
				for ( ; pSubL; pSubL = NextNOTEL(pSubL)) {
					tempLen = CalcNoteLDur(doc, pSubL, beforeL);
					if (tempLen > maxLen) {
						maxLen = tempLen;
						noteStaff = NoteSTAFF(pSubL);
					}
				}
				space = IdealSpace(doc, maxLen, RESFACTOR*doc->spacePercent);
				symWidth = SymWidthRight(doc, beforeL, noteStaff, FALSE);
				GetContext(doc, beforeL, noteStaff, &context);
				return (LinkXD(beforeL) + ((space >= symWidth) ? 
										std2d(space,context.staffHeight,context.staffLines) :
				 						std2d(symWidth,context.staffHeight,context.staffLines)));
		case RPTENDtype:
		case MEASUREtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case GRSYNCtype:
		case SPACERtype:
		case PSMEAStype:
			return (SymWidthRight(doc, beforeL, ANYONE, FALSE));
		default:
			if (TYPE_BAD(beforeL))
				MayErrMsg("CalcSpaceNeeded: object at %ld has illegal type %ld",
							(long)beforeL, (long)ObjLType(beforeL));
			return 0;
	}
}


/* ------------------------------------------ MeasSpaceProp, SetMeasSpacePercent -- */

short MeasSpaceProp(LINK pL)
{
	LINK measL;
	PMEASURE pMeas;
	
	measL = LSSearch(pL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	pMeas = GetPMEASURE(measL);
	return RESFACTOR*pMeas->spacePercent;
}


void SetMeasSpacePercent(LINK measL, long spaceProp)
{
	PMEASURE pMeas;
	
	pMeas = GetPMEASURE(measL);
	pMeas->spacePercent = spaceProp/RESFACTOR;
}


/* -------------------------------------------------------------- GetMSpaceRange -- */
/* Return the range of Measure spacePercents for Measures in the given range
of the given score. If an error is found, including no Measures in the range,
return FALSE, else return TRUE. */

Boolean GetMSpaceRange(Document *doc, LINK startL, LINK endL, short *pSpMin,
								short *pSpMax)
{
	LINK startBarL, endBarL, lastBarL, measL;	PMEASURE pMeas;
	
 /*
  *	Get the range of spacePercents in the Measures to be respaced. Start at the
  *	beginning of the Measure startL is in; if there is no such Measure, start
  *	at the first Measure of the score. If end is not after start Measure, there's
  *	nothing we can do, so arbitrarily say 100%.
  */
 		startBarL = LSSearch(LeftLINK(startL), MEASUREtype, ANYONE, GO_LEFT,
 								FALSE);
	if (!startBarL) {
		startBarL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
		if (endL==startBarL || IsAfter(endL, startBarL))
			{ *pSpMin = *pSpMax = 100; return FALSE; }
	}
	endBarL = EndMeasSearch(doc, LeftLINK(endL));
	lastBarL = LSSearch(LeftLINK(endBarL), MEASUREtype, ANYONE, GO_LEFT, FALSE);

	*pSpMin = 9999;
	*pSpMax = 1;
	for (measL = startBarL; measL!=LinkRMEAS(lastBarL); measL = LinkRMEAS(measL)) {
		pMeas = GetPMEASURE(measL);
		*pSpMin = n_min(*pSpMin, pMeas->spacePercent);
		*pSpMax = n_max(*pSpMax, pMeas->spacePercent);
	}
	
	return TRUE;
}


/* -------------------------------------------------------------------- LDur2Code -- */
/* Convert logical duration <lDur> in PDUR ticks to a more-or-less equivalent note
l_dur code and number of augmentation dots, with an error of no more than <errMax>
playDur units. The error is always positive, i.e., we may return a duration that's
shorter than desired but never longer. If we can do this, return TRUE. If there is
no such equivalent, return FALSE. */

Boolean LDur2Code(short lDur, short errMax, short maxDots, char *pNewDur, char *pNewDots)
{
	register short i, j, remainder;
	
	for (i = MAX_L_DUR; i>UNKNOWN_L_DUR; i--) {
		if (l2p_durs[i]==lDur) {
			/*
			 *	We have an exact match; no dots are necessary. 
			 */
			*pNewDur = i;
			*pNewDots = 0;
			return TRUE;
		}
		else if (l2p_durs[i]>lDur) {
			/*
			 *	l2p_durs[i+1] is the longest undotted duration shorter than <lDur>.
			 *	See if adding dots can match <lDur> closely enough.
			 */
			*pNewDur = i+1;
			if (i==MAX_L_DUR) return FALSE;				/* No, dur. shorter than shortest possible */
			remainder = lDur-l2p_durs[i+1];
			
			*pNewDots = 0;
			for (j = i+2; j<=MAX_L_DUR; j++) {
				if (remainder>=l2p_durs[j] && *pNewDots<maxDots) {
					(*pNewDots)++;
					remainder -= l2p_durs[j];
				}
				else
					return FALSE;								/* Can't match closely enough */
				if (remainder<=errMax) return TRUE;
			}
			return FALSE;										/* Can't match closely enough */
		}
	}
	
	/* The duration is longer than the longest possible single note/rest. */
	return FALSE;
}


/* -------------------------------------------------------------------- Code2LDur -- */
/* Convert note l_dur code and number of augmentation dots to logical duration in
PDUR ticks. */

long Code2LDur(char durCode, char nDots)
{
	register short	j;
	register long	noteDur;

	noteDur = (long)l2p_durs[durCode]; 				/* Get basic duration */
	for (j = 1; j<=nDots; j++)
		noteDur += (long)l2p_durs[durCode+j];		/* Add in aug. dots */
	return noteDur;
}


/* ------------------------------------------------------------------- SimpleLDur -- */
/* Compute the "simple" logical duration of a note/rest in PDUR ticks, ignoring
tuplet membership and whole-measure rests. */

long SimpleLDur(LINK aNoteL)
{
	register PANOTE aNote;

	aNote = GetPANOTE(aNoteL);
	if (aNote->subType==UNKNOWN_L_DUR || aNote->subType<=WHOLEMR_L_DUR) {
		MayErrMsg("SimpleLDur: can't handle unknown duration note or whole-measure rest, subobj %ld (ivoice %ld).",
					(long)aNoteL, (long)NoteVOICE(aNoteL));
		return 0L;
	}

	return Code2LDur(aNote->subType, aNote->ndots);
}
		

/* ---------------------------------------------------------------- SimpleGRLDur -- */
/* Compute the "simple" logical duration of a grace note. */

long SimpleGRLDur(LINK aGRNoteL)
{
	register PAGRNOTE	aGRNote;

	aGRNote = GetPAGRNOTE(aGRNoteL);
	if (aGRNote->subType==UNKNOWN_L_DUR) {
		MayErrMsg("SimpleGRLDur:can't handle unknown duration grace note at %ld.",
					(long)aGRNoteL);
		return 0L;
	}
	
	return Code2LDur(aGRNote->subType, aGRNote->ndots);
}
		

/* ------------------------------------------------------------------- GetMinDur -- */
/* Get the minimum SimpleLDur of any note/chord in the given range in the given
voice. Intended to get a value to initialize the Fancy Tuplet dialog with. */

short GetMinDur(short voice, LINK voiceStartL, LINK voiceEndL)
{
	LINK pL, aNoteL;
	short minDur;
	
	minDur = SHRT_MAX;
	for (pL = voiceStartL; pL!= voiceEndL; pL = RightLINK(pL))
		if (ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteVOICE(aNoteL)==voice) {
					minDur = n_min(minDur, SimpleLDur(aNoteL));
					break;						/* Assumes all notes in chord have same duration. */
				}
			}
		}
	return minDur;
}


/* ---------------------------------------------------------------- TupletTotDir -- */
/* Get the total SimpleLDur of the given tuplet. */

short TupletTotDir(LINK tupL)
{
	short totalDur, voice; LINK pL, aNoteL; PTUPLET pTuplet;
	
	pTuplet = GetPTUPLET(tupL);
	voice = pTuplet->voice;
	for (totalDur = 0, pL = FirstInTuplet(tupL); pL; pL = RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			aNoteL = NoteInVoice(pL, voice, FALSE);
			if (aNoteL!=NILINK) totalDur += SimpleLDur(aNoteL);
		}
		if (pL==LastInTuplet(tupL)) break;
	}
	return totalDur;
}


/* ----------------------------------------------------- GetDurUnit,GetMaxDurUnit -- */
/* Functions to infer the duration units of tuplets. It would be better simply to
store tuplets' duration units, making it unnecessary to infer the unit. However,
these procedures should be reliable and pretty fast.

To differentiate between the two functions, other than by their arguments, consider
a tuplet of four 16ths in the time of three 16ths: it would normally be labelled
"4:3" (16ths), but it could also be labelled "8:6" (32nds), "16:12" (64ths), etc.
In the normal case, both functions will give the same answer. */

/* Function 1: Infer the nominal duration unit for a tuplet from its numerator and
its total notated duration. This function would identify the unit in the example
tuplet as 16ths if it's written "4:3", 32nds if "8:6", etc. If no duration unit
makes sense--e.g., if the numerator is 3 and the tuplet includes 5 eighths for a
total of 1200 ticks--return 0. */

short GetDurUnit(short totalDur, short accNum, short /*accDenom*/)		/* <accDenom> is ignored */
{
	short temp, lDur, multiple;
	
	if (totalDur<PDURUNIT) return 0;
	
	temp = totalDur/PDURUNIT;
	lDur = PDURUNIT;
	while (!odd(temp))
		{ temp /= 2; lDur *= 2; }
	
	multiple = totalDur/lDur;
	while (accNum>multiple)
		{ multiple *= 2; lDur /= 2; }
	if (accNum!=multiple) return 0;

	return lDur;
}

/* Function 2: Infer the longest possible duration unit for the given tuplet from its
notes. Normally, this will also be the nominal duration unit. This function would
identify the unit in the example tuplet as 16ths regardless of how it's written. */

short GetMaxDurUnit(LINK tupletL)
{
	LINK aNoteTupleL, aNoteL; PANOTETUPLE aNoteTuple; Boolean firstNote;
	short durUnit;
	
	firstNote = TRUE;
	
	/* Get GCD of simple logical durations of notes */
	aNoteTupleL = FirstSubLINK(tupletL);
	for ( ; aNoteTupleL; aNoteTupleL = NextNOTETUPLEL(aNoteTupleL)) {
		aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
		aNoteL = FindMainNote(aNoteTuple->tpSync, TupletVOICE(tupletL));
		if (NoteType(aNoteL)<=UNKNOWN_L_DUR) return 0;
		if (firstNote) {
			durUnit = SimpleLDur(aNoteL);
			firstNote = FALSE;
		}
		else
			durUnit = GCD(durUnit, (short)SimpleLDur(aNoteL));
	}
		
	return durUnit;
}


/* ------------------------------------------------------------------- TimeSigDur -- */
/* Return the nominal duration of a measure with the given time signature. */

long TimeSigDur(short /*timeSigType*/,		/* ignored */
						short numerator,
						short denominator)
{
	return ((long)numerator*l2p_durs[WHOLE_L_DUR]/(long)denominator);
}


/* ----------------------------------------------------------------- CalcNoteLDur -- */
/* Compute the logical duration in PDUR ticks of a note/rest. For whole-measure rests,
return one measure's duration according to the time signature; for multibar rests,
multiply by the number of measures. If the note/rest's tuplet flag is set but the
tuplet can't be found, return -1. */

long CalcNoteLDur(Document *doc, LINK aNoteL, LINK syncL)
{
	PANOTE	aNote;
	CONTEXT	context;
	PTUPLET	pTuplet;
	LINK	tupletL;
	short	measCount;
	long	tempDur, noteDur;

	aNote = GetPANOTE(aNoteL);
	if (aNote->subType==UNKNOWN_L_DUR)							/* Logical dur. unknown? */
		return (long)(aNote->playDur);							/* Yes, use performance dur. */
	else {														/* No, use logical dur. */
		if (aNote->rest && aNote->subType<=WHOLEMR_L_DUR) {		/* Whole-meas. or multibar rest? */
			GetContext(doc, syncL, aNote->staffn, &context);	/* Yes, duration=measure dur. */
			measCount = -aNote->subType;
			noteDur = TimeSigDur(context.timeSigType,			
										context.numerator,
										context.denominator);
//if (measCount>1)
//	LogPrintf(LOG_DEBUG, "CalcNoteLDur: measCount=%d 1st noteDur=%ld\n", measCount, noteDur);
			noteDur *= measCount;
			aNote = GetPANOTE(aNoteL);
		}
		else													/* Normal duration */
			noteDur = SimpleLDur(aNoteL);
		
		if (NoteINTUPLET(aNoteL)) {
			tupletL = LVSearch(syncL, TUPLETtype, NoteVOICE(aNoteL), TRUE, FALSE);
			if (tupletL==NILINK) return -1;						/* Error, missing tuplet */
			pTuplet = GetPTUPLET(tupletL);
			tempDur = noteDur*pTuplet->accDenom;
			noteDur = tempDur/pTuplet->accNum;
		}
		return noteDur;
	}
}


/* -------------------------------------------------------------------SyncMaxDur -- */
/*	Compute the maximum logical duration in PDUR ticks of all notes/rests in the
given Sync. */

long SyncMaxDur(Document *doc, LINK syncL)
{
	long dur, newDur;	LINK aNoteL;
	
	dur = 0L; 
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		newDur = CalcNoteLDur(doc, aNoteL, syncL);
		dur = n_max(dur, newDur);
	}
	return dur;
}


/* ------------------------------------------------------------ SyncNeighborTime -- */
/* Look for specified object's closest Sync in the given direction within the same
Measure and get that Sync's logical time since previous Measure.  If there is no
Sync in the given direction within this Measure, return -1. */

long SyncNeighborTime(Document */*doc*/, LINK target, Boolean goLeft)
{
	LINK		pL;

	if (ObjLType(target)==MEASUREtype)
		return -1L;

	pL = (goLeft ? LeftLINK(target) : RightLINK(target));
	for ( ; pL!=NILINK; pL = (goLeft ? LeftLINK(pL) : RightLINK(pL)))
		switch (ObjLType(pL)) {
			case SYNCtype:
				return SyncTIME(pL);
			case MEASUREtype:
				return -1L;
			default:
				;
		}
		return -1L;
}


/* -------------------------------------------------------------------- GetLTime -- */
/*	Get "logical time" in PDUR ticks since previous Measure:
	If there's no previous Measure, give an error and return -1.
	If the argument is itself a Measure, return 0.
	If the argument is a J_IT or J_IP symbol, return the start time, in PDUR
		ticks, of that symbol.
	Otherwise, return the logical time (NOT end time!) of the last previous Sync
		in the Measure; if there is none, return 0. */

long GetLTime(Document *doc, LINK target)
{
	LINK		startL;
	long		startTime;
	short		last;
	SPACETIMEINFO	*spTimeInfo;

	spTimeInfo = (SPACETIMEINFO *)NewPtr((Size)MAX_MEASNODES *
											sizeof(SPACETIMEINFO));
	if (!GoodNewPtr((Ptr)spTimeInfo)) {
		OutOfMemory((long)MAX_MEASNODES * sizeof(SPACETIMEINFO));
		return -1L;
	}

	if (ObjLType(target)==MEASUREtype)
		goto normalReturn;

	startL = LSSearch(target, MEASUREtype, ANYONE, GO_LEFT, FALSE); /* Find previous barline */
	if (!startL) {
		MayErrMsg("GetLTime: no Measure before %ld", (long)target);
		goto errorReturn;
	}

	last = GetSpTimeInfo(doc, RightLINK(startL), target, spTimeInfo, FALSE);
	if (last>=0) {
		startTime = spTimeInfo[last].startTime;
		DisposePtr((Ptr)spTimeInfo);
		return startTime;
	}

normalReturn:
	DisposePtr((Ptr)spTimeInfo);
	return 0L;
	
errorReturn:
	DisposePtr((Ptr)spTimeInfo);
	return -1L;
	
}


/* ---------------------------------------------------------------- GetSpaceInfo -- */
/*	Get information on Syncs needed for Gourlay's spacing algorithm: the	
controlling duration and fraction of that duration that affects spacing.
Also fill in suitable values for non-Syncs in the spine. */

static void GetSpaceInfo(
				Document *doc,
				LINK	barFirst,				/* First obj within Measure, i.e., after barline */
				LINK	/*barLast*/,			/* Last obj to consider (usually the next Measure obj.) */				
				short	count,
				SPACETIMEINFO spaceTimeInfo[] 	/* Assumes startTime,link,isSync already filled in */
				)
{
	long		timeHere, earliestEnd,
				earliestDur,
				nextStartTime,
				vLTimes[MAXVOICES+1],				/* Logical times for voices */
				vLDur[MAXVOICES+1];					/* Logical note durations for voices */
	register short k;
	short		v, earlyVoice;
	LINK		syncL, aNoteL;
	
	for (v = 0; v<=MAXVOICES; v++)
		vLTimes[v] = -1L;

	/*
	 * We need to set the <dur> (controlling duration) and <frac> (fraction of that
	 * duration that affects spacing here) for each object in the spine.
	 *
	 *	For each Sync, find the earliest ending of all notes starting or continuing
	 *	at its start time and (in case a voice drops out in the middle of the measure)
	 *	continuing to the next Sync. If there's more than one ending at the earliest
	 *	time, take the one, or one of the ones, of shortest duration. Also find the
	 *	fraction of that note that will elapse before the next Sync. (This is the
	 *	first stage of Gourlay's line spacing algorithm with two modifications: ignoring
	 *	notes that don't continue til the next Sync, and taking the shortest of notes
	 *	that end at the same time. Without the first modification, a score with two
	 *	half notes in one voice and just a 16th in another voice in the first half's
	 *	Sync would be spaced abominably. The second modification is just for fine-tuning.)
	 *
	 * For objects other than Syncs (Spacers, pseudo-Measures, etc.), just set <dur>
	 * and <frac> to appropriate constants.
	 */
 	for (k = 0; k<count; k++)
		if (spaceTimeInfo[k].isSync) {
			syncL = spaceTimeInfo[k].link;
	
			timeHere = spaceTimeInfo[k].startTime;

			aNoteL = FirstSubLINK(syncL);					/* Synchronize all participating voices */
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				vLDur[NoteVOICE(aNoteL)] = CalcNoteLDur(doc, aNoteL, syncL);
				vLTimes[NoteVOICE(aNoteL)] = timeHere+vLDur[NoteVOICE(aNoteL)];
			}
			
			nextStartTime = spaceTimeInfo[k+1].startTime;
			earliestEnd = 9999999L;
			for (v = 0; v<=MAXVOICES; v++) {
				if (vLTimes[v]>timeHere						/* Continue into current sync? */
				&& vLTimes[v]>=nextStartTime				/* Continues until next sync or measure end? */
				&& (   vLTimes[v]<earliestEnd				/* Ends earlier than previous earliest, or... */
					|| (vLTimes[v]==earliestEnd				/* ...ends no later than it and... */
						&& vLDur[v]<earliestDur				/* ...has shorter duration? */					
						)
					)
				) {
					earliestEnd = vLTimes[v];
					earliestDur = vLDur[v];
					earlyVoice = v;
				}
			}
			if (earliestEnd>=9999999L)
				MayErrMsg("GetSpaceInfo: can't find a note to set spacing in bar at %ld. k=%ld,timeHere=%ld",
							(long)LeftLINK(barFirst), (long)k, timeHere);
			spaceTimeInfo[k].dur = vLDur[earlyVoice];
			spaceTimeInfo[k].frac = (FASTFLOAT)(vLTimes[earlyVoice]-timeHere)/spaceTimeInfo[k].dur;
		}
		else {
			spaceTimeInfo[k].dur = 0L;
			spaceTimeInfo[k].frac = 1.0;
		}
}


/* ---------------------------------------------------------------- FixStaffTime -- */
/* Set times for the given staff based on voices that most recently appeared on
that staff. Intended to be called for objects (clefs, key signatures, time
signatures) that are themselves attached to a staff but are affected by timings
of all voices on that staff. */

static void FixStaffTime(long [], short, short [], long []);
static void FixStaffTime(
				long	stLTimes[MAXSTAVES+1],		/* Logical times for staves */
				short	staff,
				short	vStaves[MAXVOICES+1],
				long	vLTimes[MAXVOICES+1] 		/* Logical times for voices */
				)
{
	register short i;
	
	for (i = 0; i<=MAXVOICES; i++)
		if (vStaves[i]==staff) stLTimes[i] = n_max(stLTimes[i], vLTimes[i]);
}


/* --------------------------------------------------------------- FixVoiceTimes -- */
/* Set times for all voices that most recently appeared on the given staff to
at least <timeHere>. Intended to be called for objects (clefs, key signatures,
time signatures) that are themselves attached to a staff but affect timings of
all voices on that staff. */

static void FixVoiceTimes(long, short, short [], long []);
static void FixVoiceTimes(
				long	timeHere,
				short	staff,
				short	vStaves[MAXVOICES+1],
				long	vLTimes[MAXVOICES+1] 		/* Logical times for voices */
				)
{
	register short i;
	
	for (i = 0; i<=MAXVOICES; i++)
		if (vStaves[i]==staff) vLTimes[i] = n_max(vLTimes[i], timeHere);
}


#ifdef NOTYET
			/* Synchronize for round-off error within the tuplet. Note that this
				technique will cause GetSpTimeInfo to ignore any tweaking done to
				the durations of notes in tuplets. To avoid that, instead of this,
				when creating the tuplet, could add in the total round-off error
				to the duration of the last note in the tuplet, if that duration
				were stored in the data structure. But it is gotten from
				CalcNoteLDur, which blindly adjusts multiplying by denom/num. 
				So CalcNoteLDur could also check for LastInTuplet, and add diff
				to compensate for round-off, if that were stored anywhere. To
				get totalTime of tuplet, just need to store minDur: total time
				= minDur * denom. totalTime is guaranteed to be an integer,
				and so this algorithm is guaranteed to eliminate roundoff,
				and synchronize all notes in the voice following the tuplet
				properly. Problem is: what if one of the notes in the tuplet is
				moved along by some other voice? */

			if (NoteINTUPLET(aNoteL) {
				tupletL = LVSearch(pL,TUPLETtype,NoteVOICE(aNoteL),FALSE,FALSE);
				if (pL==LastInTuplet(tupletL)) {
					syncForTuplet[NoteVOICE(aNoteL)] = TRUE;
				}
			}
			else {
				if (syncForTuplet[NoteVOICE(aNoteL)]) {
					tupletL = LVSearch(pL,TUPLETtype,NoteVOICE(aNoteL),FALSE,FALSE);
					firstL = FirstInTuplet(tupletL);
					
					tEndTime = ??? time of firstL + total time of tuplet ???;

					if (vLTimes[NoteVOICE(aNoteL)) < tEndTime
						vLTimes[NoteVOICE(aNote) = tEndTime;
					
					syncForTuplet[NoteVOICE(aNoteL)] = FALSE;
				}
			}
#endif


/* ----------------------------------------------------------------- GetSpTimeInfo -- */
/* Creates the rhythmic spine, i.e., the set of MEvent logical times integrated
across all voices for symbols of either independent justification type, for one
measure or less. It then uses the spine to fill in various information in
<spaceTimeInfo>: always, in startTime, logical times; in justType, justification
types; in link, object pointers; and in isSync, whether the object is a Sync.
If <spacing>, the dur and frac fields are also filled in. The function value
returned is the index of the last entry in the list, NOT the number of entries
in the list! For example, if it returns 4, startTime[0--4] and justType[0--4] are
meaningful. If the spine is empty, it returns -1.

One reason this is tricky is some symbols (notes, grace notes) are in voices, others
(clefs, key sigs., etc.) are on staves, but their interactions affect timing. This
is true even though notes and rests are the only symbols that occupy time. */

short GetSpTimeInfo(
			Document		*doc,
			LINK			barFirst,			/* First obj within measure, i.e., after barline */
			LINK			barLast,			/* Last obj to consider (usually the next MEASURE obj.) */				
			SPACETIMEINFO	spaceTimeInfo[],
			Boolean			getSpacing 			/* TRUE=return spacing info as well as time info */
			)
{
	register long	timeHere;
	long		stLTimes[MAXSTAVES+1],			/* Logical times for staves */
				vLTimes[MAXVOICES+1];			/* Logical times for voices */
	short		vStaves[MAXVOICES+1];			/* Staff each voice is currently on */
	short		last, i;
	PANOTE		aNote;
	PAGRNOTE	aGRNote;
	register LINK	pL, aNoteL, aGRNoteL;
	LINK		aClefL, aKeySigL, aTimeSigL;
	char		jType;
	Boolean		voiceInSync[MAXVOICES+1];
	long		noteDur, minDur;
	
	for (i = 0; i<=doc->nstaves; i++)
		stLTimes[i] = 0L;
	for (i = 0; i<=MAXVOICES; i++) {
		vLTimes[i] = 0L;
		vStaves[i] = 0;
	}

	last = -1;

	for (pL = barFirst; pL!=RightLINK(barLast); pL = RightLINK(pL)) {
		timeHere = 0L;
		jType = objTable[ObjLType(pL)].justType;

		switch (ObjLType(pL))
		{
			case SYNCtype:
				for (i = 0; i<=MAXVOICES; i++)
					voiceInSync[i] = FALSE;
					
				/*
				 * Set the start time for this Sync to the current time for the voice or
				 * staff that's furthest along of all participating. Then synchronize all
				 * participating voices by setting their <vLTimes> to the ending times
				 * of their notes in this Sync.
				 */
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					timeHere = n_max(timeHere, vLTimes[aNote->voice]);
					timeHere = n_max(timeHere, stLTimes[aNote->staffn]);
				}
				minDur = 99999L;
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					noteDur = CalcNoteLDur(doc, aNoteL, pL);
					aNote = GetPANOTE(aNoteL);
					vLTimes[aNote->voice] = timeHere+noteDur;
					voiceInSync[aNote->voice] = TRUE;
					vStaves[aNote->voice] = aNote->staffn;
					if (noteDur<minDur) minDur = noteDur;
				}
				
				/* FIXME: Code to sync after tuplets, avoiding roundoff errror, belongs here. */
				
				/*
				 * The following code insures that every voice that doesn't participate
				 * in this Sync has a time at least as far along as some voice that
				 * does participate. This is a drastic step, intended to handle situations
				 * where we have no way to tell when a Sync should happen because we can't
				 * trace its voices back to the beginning of the Measure. Unfortunately,
				 * it can give bad results in cases where consecutive Syncs have no voices
				 * in common but we could still trace both back to the beginning of the
				 * Measure. We should fix this someday.
				 */
				for (i = 0; i<=MAXVOICES; i++)							/* Make non-participating */
					if (!voiceInSync[i])								/*   voices catch up */
						if (vLTimes[i]<=timeHere)
							vLTimes[i] = timeHere+minDur;
				break;
				
			case GRSYNCtype:
				/*
				 * Handle grace Syncs just like regular ones except that ending times
				 * of participating voices are all equal to the Sync's start time.
				 */
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
					aGRNote = GetPAGRNOTE(aGRNoteL);
					timeHere = n_max(timeHere, vLTimes[aGRNote->voice]);
					timeHere = n_max(timeHere, stLTimes[aGRNote->staffn]);
				}
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
					aGRNote = GetPAGRNOTE(aGRNoteL);
					vLTimes[aGRNote->voice] = timeHere;
					vStaves[aGRNote->voice] = aGRNote->staffn;
				}
				break;
				
			case TAILtype:
			case SYSTEMtype:
			case PAGEtype:
			case MEASUREtype:
			case PSMEAStype:
			case RPTENDtype:
				/* Synchronize everything to high-water mark of either staves or voices. */
				for (i = 1; i<=doc->nstaves; i++)
					timeHere = n_max(timeHere, stLTimes[i]);
				for (i = 0; i<=MAXVOICES; i++)
					timeHere = n_max(timeHere, vLTimes[i]);
				break;
				
			case CLEFtype:
				aClefL = FirstSubLINK(pL);
				for ( ; aClefL; aClefL = NextCLEFL(aClefL)) {
					FixStaffTime(stLTimes, ClefSTAFF(aClefL), vStaves, vLTimes);
					timeHere = n_max(timeHere, stLTimes[ClefSTAFF(aClefL)]);
				}
				aClefL = FirstSubLINK(pL);
				for ( ; aClefL; aClefL = NextCLEFL(aClefL)) {
					stLTimes[ClefSTAFF(aClefL)] = timeHere;
					FixVoiceTimes(timeHere, ClefSTAFF(aClefL), vStaves, vLTimes);
				}
				break;
				
			case KEYSIGtype:
				aKeySigL = FirstSubLINK(pL);
				for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) {
					FixStaffTime(stLTimes, KeySigSTAFF(aKeySigL), vStaves, vLTimes);
					timeHere = n_max(timeHere, stLTimes[KeySigSTAFF(aKeySigL)]);
				}
				aKeySigL = FirstSubLINK(pL);
				for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) {
					stLTimes[KeySigSTAFF(aKeySigL)] = timeHere;
					FixVoiceTimes(timeHere, KeySigSTAFF(aKeySigL), vStaves, vLTimes);
				}
				break;
				
			case TIMESIGtype:
				aTimeSigL = FirstSubLINK(pL);
				for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
					FixStaffTime(stLTimes, TimeSigSTAFF(aTimeSigL), vStaves, vLTimes);
					timeHere = n_max(timeHere, stLTimes[TimeSigSTAFF(aTimeSigL)]);
				}
				aTimeSigL = FirstSubLINK(pL);
				for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
					stLTimes[TimeSigSTAFF(aTimeSigL)] = timeHere;
					FixVoiceTimes(timeHere, TimeSigSTAFF(aTimeSigL), vStaves, vLTimes);
				}
				break;
				
			case STAFFtype:
			case CONNECTtype:
			case BEAMSETtype:
			case TUPLETtype:
			case OTTAVAtype:
			case DYNAMtype:
			case SLURtype:
			case GRAPHICtype:
			case TEMPOtype:
			case SPACERtype:
			case ENDINGtype:
				break;
				
			default:
				MayErrMsg("GetSpTimeInfo: can't handle object type %ld (at %ld)",
							(long)ObjLType(pL), (long)pL);
				break;
		}

		if (jType!=J_D) {
			last++;
			if (last>=MAX_MEASNODES)	{										/* Is there room to add it? */
				MayErrMsg("GetSpTimeInfo: can't handle over %ld nodes in a measure",
							(long)MAX_MEASNODES);
				return TOO_MANY_MEASNODES;
			}
			else {																	/* Yes, add it to list */
				spaceTimeInfo[last].startTime = timeHere;
				spaceTimeInfo[last].justType = jType;
				spaceTimeInfo[last].link = pL;
				spaceTimeInfo[last].isSync = SyncTYPE(pL);
			}
		}
	}
	
	if (getSpacing) GetSpaceInfo(doc, barFirst, barLast, last, spaceTimeInfo);

	return last;
}


/* ------------------------------------------------------------- RhythmUnderstood -- */
/* Decide if Nightingale "understands" rhythm in the given measure.

If <strict>, return TRUE iff there are no unknown-duration notes in the measure.
(This is really not very strict: for example, we might also insist:
	- All time signatures denote the same total time
	- No time signature appears after the first Sync
	- The total duration of each voice that has any notes agrees with the time signature
	- Every Sync after the 1st has a common voice with the preceding Sync
Cf. DCheckMeasDur.)

If <!strict>, return TRUE if there are no unknown-duration notes OR there's at least
one known-duration note in the measure. */

Boolean RhythmUnderstood(Document *doc, LINK measL, Boolean strict)
{
	LINK pL, aNoteL, endMeasL;
	short nKnown=0, nUnknown=0;
	
	endMeasL = EndMeasSearch(doc, measL);
	for (pL = RightLINK(measL); pL!=endMeasL; pL = RightLINK(pL))
		if (ObjLType(pL)==SYNCtype) {
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
			if (NoteType(aNoteL)==UNKNOWN_L_DUR) {
				if (strict) return FALSE;
				else			nUnknown++;
			}
			else
				nKnown++;
		}

	if (strict) return (nUnknown==0);
	else			return (nUnknown==0 || nKnown>0);
}


/* ----------------------------------------------------------- FixMeasTimeStamps -- */
/* Recompute timeStamps for Syncs in the given Measure. Does not adjust the
lTimeStamps of following Measures, but returns the Measure's change in duration
to facilitate its caller doing so.

NB: If Nightingale doesn't understand rhythm in the measure--e.g., because of
unknown duration notes--this is a dangerous thing to do. For now, if we think we
don't understand the rhythm, do nothing at all. */

static long FixMeasTimeStamps(Document *, LINK, SPACETIMEINFO *);
static long FixMeasTimeStamps(
					Document *doc,
					LINK measL,
					SPACETIMEINFO	*spTimeInfo 	/* Scratch space (neither input nor output!) */
					)
{
	LINK endMeasL, pL, nextMeasL, aMeasureL;
	PAMEASURE aMeasure;
	short i, last;
	long endMeasTime, oldMeasDur, timeChange;
	Boolean measTooLong=FALSE;
	char fmtStr[256];
	
#if 1
	/* ??Experimental partial solution */
	if (!CapsLockKeyDown() && !RhythmUnderstood(doc, measL, FALSE)) return 0L;
#endif

	endMeasL = EndMeasSearch(doc, measL);
	last = GetSpTimeInfo(doc, RightLINK(measL), endMeasL, spTimeInfo, FALSE);
	if (last>=0) {
		for (i = 0; i<last; i++)
			if (spTimeInfo[i].isSync) {
				if (spTimeInfo[i].startTime>MAX_SAFE_MEASDUR) {
					spTimeInfo[i].startTime = 65535L;
					measTooLong = TRUE;
				}
				pL = spTimeInfo[i].link;
				SyncTIME(pL) = spTimeInfo[i].startTime;
			}
		endMeasTime = spTimeInfo[last].startTime;
	}
	else
		endMeasTime = 0L;

	if (measTooLong) {
		aMeasureL = FirstSubLINK(measL);
		aMeasure = GetPAMEASURE(aMeasureL);
		GetIndCString(fmtStr, MISCERRS_STRS, 4);		/* "Meas. has note(s) > 65500 ticks" */
		sprintf(strBuf, fmtStr, aMeasure->measureNum+doc->firstMNNumber);
		CParamText(strBuf, "", "", "");
		LogPrintf(LOG_NOTICE, strBuf);
		NoteInform(GENERIC_ALRT);
	}

	oldMeasDur = 0L;
	nextMeasL = LinkRMEAS(measL);
	if (nextMeasL) oldMeasDur = MeasureTIME(nextMeasL)-MeasureTIME(measL);
	timeChange = endMeasTime-oldMeasDur;
	return timeChange;
}


/* --------------------------------------------------------------- FixTimeStamps -- */
/* Recompute timeStamps for Syncs in the range of Measures [startL,endL] and
adjust lTimeStamps of all following Measures accordingly. If startL is before the
first Measure of the score, first set its lTimeStamp to 0. Note that if startL is
a Measure, its timestamp will NOT be recomputed. */

Boolean FixTimeStamps(
		Document *doc,
		LINK startL,				/* The 1st Measure to fix or any link within it, or doc->headL */
		LINK endL 					/* Any link within the last Measure to fix, or NILINK */
		)
{
	SPACETIMEINFO *spTimeInfo;
	LINK measL, endMeasL=NILINK;
	long timeChange; Boolean okay=TRUE;
	
	spTimeInfo = AllocSpTimeInfo();
	if (!spTimeInfo) return FALSE;
	
	/*
	 *	Start at the beginning of the Measure startL is in; if there is no such Measure,
	 *	start at the first Measure of the score. If end is not after start Measure, there's
	 *	nothing to do.
	 */
	measL = SSearch(startL, MEASUREtype, GO_LEFT);
	if (measL==NILINK) {
		measL = SSearch(startL, MEASUREtype, GO_RIGHT);
		MeasureTIME(measL) = 0L;
	}

	if (endL) endMeasL = SSearch(RightLINK(endL), MEASUREtype, GO_RIGHT);
	if (endMeasL && !IsAfter(measL, endMeasL))
		{ okay = FALSE; goto done; }

	for ( ; measL && measL!=endMeasL; measL=LinkRMEAS(measL)) {
		timeChange = FixMeasTimeStamps(doc, measL, spTimeInfo);
		MoveTimeMeasures(RightLINK(measL), doc->tailL, timeChange);
	}
	
done:
	DisposePtr((Ptr)spTimeInfo);
	return okay;
}


/* --------------------------------------------------------------------- GetLDur -- */
/* Get logical duration of an object for the given staff or for all staves. */
				
long GetLDur(
			Document *doc,
			LINK pL,
			short staff 			/* Staff to consider, or 0=all staves ??want ANYONE! */
			)
{
	long num, denom, noteDur;
	LINK	 aNoteL;

	if (ObjLType(pL)==SYNCtype)	{
		num = 0L;
		denom = 128L;
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
			if (NoteSTAFF(aNoteL)==staff || staff==0) {
				noteDur = CalcNoteLDur(doc, aNoteL, pL);
	  			num = n_max(num, noteDur);
	  		}
	  	return num;
	}
	
	return 0L;												/* Not a Sync */
}


/* -------------------------------------------------------------------- GetVLDur -- */
/* Get logical duration of an object for the given voice or for all voices. */
				
long GetVLDur(
			Document *doc,
			LINK pL,
			short voice 			/* Voice to consider, or 0=all voices ??want ANYONE! */
			)
{
	long num, denom, noteDur;
	LINK aNoteL;

	if (ObjLType(pL)==SYNCtype) {		
		num = 0L;
		denom = 128L;
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==voice || voice==0)		/* This note in desired voice? */			
			{
				noteDur = CalcNoteLDur(doc, aNoteL, pL); 	/* Yes, get its dur. */
  				num = n_max(num, noteDur);
  			}
	  	return num;
	}
	
	return 0L;												/* Not a Sync */
}


/* ------------------------------------------------------------------- GetMeasDur -- */
/* Get the actual -- based on notes and rests, not time signature -- logical duration
of the Measure preceding the given Measure object, either for a single staff or across
all staves. If something goes wrong, return -1L. */
				
long GetMeasDur(Document *doc,
				LINK endMeasL,		/* Object ending a Measure */
				short staffn)
{
	LINK			startL, syncL, aNoteL;
	long			startTime, maxDur;
	short			last;
	SPACETIMEINFO	*spTimeInfo=NULL;

	startL = LSSearch(LeftLINK(endMeasL), MEASUREtype, ANYONE, GO_LEFT, FALSE); /* Find previous barline */
	if (!startL) {
		MayErrMsg("GetMeasDur: no Measure before %ld", (long)endMeasL);
		return -1L;
	}
	
	if (staffn!=ANYONE) {
		/* Consider one staff only. */
		syncL = LSSearch(LeftLINK(endMeasL), SYNCtype, staffn, GO_LEFT, FALSE);
		if (!syncL) return 0L;
		maxDur = 0L;
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			if (NoteSTAFFN(aNoteL)==staffn)
			maxDur = max(maxDur, CalcNoteLDur(doc, aNoteL, syncL));
		}
		LogPrintf(LOG_DEBUG, "GetMeasDur: endMeasL=%d SyncTIME(syncL)=%d maxDur=%ld\n", endMeasL, SyncTIME(syncL), maxDur);
		return SyncTIME(syncL)+maxDur;
	}
	
	/* From this point on, consider all staves. */
	if (!RhythmUnderstood(doc, startL, TRUE)) {
		/* It's doubtful this function can do a good job in this case, but we'll try. */
		syncL = LSSearch(LeftLINK(endMeasL), SYNCtype, ANYONE, GO_LEFT, FALSE);
		if (!syncL) return 0L;
		return SyncTIME(syncL)+NotePLAYDUR(FirstSubLINK(syncL));
	}

	spTimeInfo = (SPACETIMEINFO *)NewPtr((Size)MAX_MEASNODES *
												sizeof(SPACETIMEINFO));
	if (!GoodNewPtr((Ptr)spTimeInfo)) {
		OutOfMemory((long)MAX_MEASNODES * sizeof(SPACETIMEINFO));
		return -1L;
	}

	last = GetSpTimeInfo(doc, RightLINK(startL), endMeasL, spTimeInfo, FALSE);
	if (last>=0) {
		startTime = spTimeInfo[last].startTime;
		DisposePtr((Ptr)spTimeInfo);
		return startTime;
	}

	if (spTimeInfo) DisposePtr((Ptr)spTimeInfo);
	return -1L;
}


/* -------------------------------------------------------------- GetTimeSigMeasDur -- */
/* Get the notated (i.e., according to the time signature) duration of the Measure
ending at the given link. Looks at all staves; if they don't all have the same time
signature, returns -1. NB: If any staff changes time signature in the middle of the
Measure, this will misbehave. Cf. GetMeasDur, which gets the Measure's actual
duration. */

long GetTimeSigMeasDur(Document */*doc*/,
							LINK endMeasL)		/* Object ending a Measure */
{
	LINK measL, aMeasL;
	PAMEASURE aMeasure;
	long thisMeasDur, measDur=-1L;
	
		/*
		 *	If <endMeasL> is not itself a Measure, look for the following Measure;
		 * if no following Measure, use the preceding Measure (which must exist).
		 */	
		if (MeasureTYPE(endMeasL))
			measL = endMeasL;
		else {
			measL = SSearch(endMeasL, MEASUREtype, GO_RIGHT);
			if (!measL) measL = SSearch(endMeasL, MEASUREtype, GO_LEFT);
		}
		
		for (aMeasL = FirstSubLINK(measL); aMeasL; aMeasL = NextMEASUREL(aMeasL)) {
			aMeasure = GetPAMEASURE(aMeasL);
			thisMeasDur = TimeSigDur(aMeasure->timeSigType,
											aMeasure->numerator,
											aMeasure->denominator);
			if (measDur<0L) measDur = thisMeasDur;
			else if (measDur!=thisMeasDur) return -1L;
		}
		
		return measDur;
}

								
#ifdef NOTYET

long GetStaffMeasDur(Document *doc, LINK endMeasL, short staffn);

/* ------------------------------------------------------------------ GetStaffMeasDur -- */
/* Get the actual -- based on notes and rests, not time signature -- logical duration
of notes/rests on the given staff in the Measure preceding the given Measure object. If
something goes wrong, return -1L. */

long GetStaffMeasDur(Document *doc,
					 LINK endMeasL,		/* Object ending a Measure */
					 short staffn)
{
	LINK	startL, pL, aNoteL;
	long	maxTime, endTime;

	startL = LSSearch(LeftLINK(endMeasL), MEASUREtype, ANYONE, GO_LEFT, FALSE); /* Find previous barline */
	if (!startL) {
		MayErrMsg("GetStaffMeasDur: no Measure before %ld", (long)endMeasL);
		return -1L;
	}

	maxTime = 0L;
	pL = startL;
	for ( ; pL = RightLINK(pL); pL!=endMeasL) {
		if (!SyncTYPE(pL)) continue;
			aNoteL = FirstSubLINK(pL);
			for (; aNoteL; aNoteL=NextNOTEL(aNoteL))
			if (NoteSTAFF(aNoteL)==staffn) {
				endTime = SyncTIME(+SimpleLDur??(aNoteL);
				if (endTime>maxTime) maxTime = endTime;
			}
		}
	
	return maxTime;
}

#endif

/* --------------------------------------------------------- WholeMeasRestIsBreve -- */
/* If, in a time signature with the given numerator and denominator, a whole-measure
rest should look like a breve rest, return TRUE; otherwise, a whole-measure rest
should look like a whole rest, and return FALSE.

There's no question that in 4/2, breve rests should be used for whole measures; in
other long-duration meters, what authorities say is inconsistent and seems not always
well-thought-out. But Gould's _Behind Bars_ says (p. 160) that breve rests should be
used in 4/2 and 8/4 and all greater durations. Surely they should also be used in 2/1,
16/8, etc.! */

Boolean WholeMeasRestIsBreve(char numerator, char denominator)
{
	Boolean breveWholeMeasure;
	
	breveWholeMeasure = (config.wholeMeasRestBreve!=0 && numerator>=2*denominator);
	return breveWholeMeasure;
}
