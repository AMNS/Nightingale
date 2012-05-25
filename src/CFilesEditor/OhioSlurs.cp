
/*
 *	This file contains routines implementing the algorithms set forth in
 *	the Ohio State University Technical Research Report, "Computer Design
 *	of Musical Slurs, Ties, and Phrase Marks", by Francisco J. Sola.
 *
 *	Douglas M. McKenna, April 5, 1988
 */

#ifdef NOTYET

//#include "types.h"
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "OhioSlurs.h"
//#include "CurrentEvents.h"

/* Prototypes of private library routines known only in this file */

void	SL_GetLocation(void);
void	SL_GetSingleLocation(void);
void	SL_GetMultiLocation(void);
void	SL_SetStemUp(Boolean, Boolean, Boolean, Boolean, Boolean);
void	SL_SetStemDown(Boolean, Boolean, Boolean, Boolean, Boolean);
void	SL_SetAccentUp(Boolean, Boolean, Boolean, Boolean, Boolean);
void	SL_SetAccentDown(Boolean, Boolean, Boolean, Boolean, Boolean);
void	SL_SetBetweenLines(Boolean, Boolean, Boolean, Boolean, Boolean);
void	SL_SetBeamed(Boolean, Boolean, Boolean, Boolean, Boolean);
void	SL_SetStaffDegree(DDIST, DDIST, DDIST, DDIST, DDIST);
void	SL_SetStemLength(DDIST, DDIST, DDIST, DDIST, DDIST);
void	SL_GetControlPoints(void);
void	SL_GetHighest(void);
double	sqrt(double);

/*
 *	Various state variables for drawing the current slur, from Section 3.1.
 */

static Boolean	multipleVoices,	/* TRUE when several voices occupy slur's staff */
				upperVoice,		/* Does slur relate to upper voice */
				middleVoice,	/* Does slur relate to middle voice */
				lowerVoice,		/* Does slur relate to lower voice */
				intermediate;	/* Does slur pass intermediate notes */

static DDIST	dyStaff;		/* Distance between staff lines */
static DDIST	slurLength;		/* X Distance from starting note to ending note */

typedef struct {
				unsigned first : 1;		/* Note starts a slur */
				unsigned last  : 1; 	/* Note ends a slur */
				unsigned ends  : 1;		/* Both first and last */
				unsigned inter : 1; 	/* Note is in-between end notes of slur */
				unsigned all   : 1; 	/* All of the above */
			
			} NoteInfo;

static NoteInfo	stemUp,			/* Which notes have stems pointing up */
				stemDown,		/* And which down */
				accentUp,		/* Which notes have accents above note head */
				accentDown,		/* And which down */
				beamed,			/* Which notes are beamed or flagged */
				betweenLines;	/* Which notes are positioned between staff lines */

typedef struct {
				DDIST first;
				DDIST last;
				DDIST ends;
				DDIST inter;
				DDIST all;
			
			} NoteDist;

static NoteDist	staffDegree,	/* How high in staff are starting and ending notes */
				stemLength,		/* How long are the stems */
				tipValue,		/* Distance from stem tip end to slur endpoint */
				location,		/* Vert dist from center of notehead to place slur points to */
				phyVerLoc,		/* Vert location of slur end w/r/t staff */
				phyHorLoc;		/* Horiz dist from notehead center to slur start */
			
static Boolean	bowUp;			/* TRUE when slur should bow up; FALSE for down */

static DDIST	xDist,yDist;	/* Offset from first slur endpoint to first control pt */
static DDIST	xCtl0,yCtl0;	/* Absolute first control point */
static DDIST	xCtl1,yCtl1;	/* Absolute last control point */
static DDIST	SDTop;			/* Top of slur */

/*
 *	This should be called first every time we want to build a new slur travelling
 *	between two notes.
 *	It sets the current inter-staff-line distance, units of which are used in
 *	all the rest of these algorithms, and generally initializes for a new slur.
 *	The last parameter declares the voice environment the slur is to be found in.
 */

void SL_InitSlur(DDIST dy, int voice)
	{
		dyStaff = dy;				/* Local copy of inter-staffline distance */
#if 0
		DebugPrintf("SL_InitSlur: dyStaff = %d\n",dyStaff);
#endif
		
		/* Init the various state flags prior to looping through spanned notes */
		
		intermediate = FALSE;
		
		stemUp.first = stemDown.first = accentUp.first = accentDown.first = FALSE;
		beamed.first = betweenLines.first = FALSE;
		
		stemUp.last = stemDown.last = accentUp.last = accentDown.last = FALSE;
		beamed.last = betweenLines.last = FALSE;
		
		stemUp.all = stemDown.all = accentUp.all = accentDown.all = TRUE;
		beamed.all = betweenLines.all = TRUE;
		
		stemUp.inter = stemDown.inter = accentUp.inter = accentDown.inter = TRUE;
		beamed.inter = betweenLines.inter = TRUE;
		
		stemUp.ends = stemDown.ends = accentUp.ends = accentDown.ends = TRUE;
		beamed.ends = betweenLines.ends = TRUE;
		
		stemLength.inter = 0;
		
		multipleVoices = upperVoice = middleVoice = lowerVoice = FALSE;
		
		switch(voice) {
			case V_Upper:
					multipleVoices = upperVoice = TRUE;
					break;
			case V_Middle:
					multipleVoices = middleVoice = TRUE;
					break;
			case V_Lower:
					multipleVoices = lowerVoice = TRUE;
					break;
			case V_Single:
					break;
			}
	}

/*
 *	After startup, every note that the slur passes over should be declared to
 *	the library so that it can make the correct decisions on slur placement and
 *	shape accordingly.  There should be at least two calls to this routine,
 *	with type set to N_First the first time, and N_Last for the last time; if
 *	the slur is to pass over intermediate notes, then they should be declared
 *	as type N_Intermediate.  The first note is assumed to the left of the last.
 *	The values for each declared note should be:
 *
 *		stem			1 if stem up, 0 if no stem, -1 if stem down
 *		acc				1 if accent up, 0 if no accent, -1 if accent down
 *		beam			1 if beamed or flagged, 0 if not
 *		x				the X position of the notehead
 *		y				the Y position of the notehead w/r/t bottom staff line
 *		stlen			the stem length of the note (0 if none; always positive)
 *
 *	This lets us make a quick analysis of the local area that the slur is to
 *	be drawn in, so that we can bow it up or down, avoid staff lines and notes, etc.
 *	All coordinate or length values are in DDISTS (as opposed to staff-spaces as
 *	they are in the original report).
 */

void SL_DeclareNote(int type, int stem, int acc, int beam, DDIST x, DDIST y, DDIST stlen)
	{
		int deg;
		
		deg = y / (dyStaff>>1);
#if 0
		DebugPrintf("DeclareNote: type(%d) stem(%d) accent(%d) beamed(%d) x(%d) y(%d) degree(%d) length(%d)\n",
						type,stem,acc,beam,x,y,deg,stlen);
#endif
		switch(type) {
			case N_First:
					slurLength = x;			/* Prepare for calculation in N_Last */
					staffDegree.first = y;
					stemLength.first = stlen;
					phyHorLoc.first = x;
					SL_SetStemUp(stem>0,FALSE,stem>0,FALSE,stem>0);
					SL_SetStemDown(stem<0,FALSE,stem<0,FALSE,stem<0);
					SL_SetAccentUp(acc>0,FALSE,acc>0,FALSE,acc>0);
					SL_SetAccentDown(acc<0,FALSE,acc<0,FALSE,acc<0);
					SL_SetBeamed(beam!=0,FALSE,beam!=0,FALSE,beam!=0);
					SL_SetBetweenLines(deg&1,FALSE,deg&1,FALSE,deg&1);
					break;
			case N_Intermediate:
					SL_SetStemUp(FALSE,FALSE,FALSE,stem>0,stem>0);
					SL_SetStemDown(FALSE,FALSE,FALSE,stem<0,stem<0);
					SL_SetAccentUp(FALSE,FALSE,FALSE,acc>0,acc>0);
					SL_SetAccentDown(FALSE,FALSE,FALSE,acc<0,acc<0);
					SL_SetBeamed(FALSE,FALSE,FALSE,beam!=0,beam!=0);
					SL_SetBetweenLines(FALSE,FALSE,FALSE,deg&1,deg&1);
					break;
			case N_Last:
					slurLength = x - slurLength;
					staffDegree.last = y;
					stemLength.last = stlen;
					phyHorLoc.last = x;
					SL_SetStemUp(FALSE,stem>0,stem>0,FALSE,stem>0);
					SL_SetStemDown(FALSE,stem<0,stem<0,FALSE,stem<0);
					SL_SetAccentUp(FALSE,acc>0,acc>0,FALSE,acc>0);
					SL_SetAccentDown(FALSE,acc<0,acc<0,FALSE,acc<0);
					SL_SetBeamed(FALSE,beam!=0,beam!=0,FALSE,beam!=0);
					SL_SetBetweenLines(FALSE,deg&1,deg&1,FALSE,deg&1);
					break;
			}
	}

/*
 *	Once all the notes have been scanned by SL_DeclareNote() above, we call
 *	this routine to do all the actual calculations, and to deliver the
 *	coordinates of a slur that will fit according to the various criteria.
 */

void SL_GetSlur(DPoint *startPt, DPoint *c1, DPoint *c2, DPoint *endPt)
	{
		SL_GetLocation();
		SL_GetControlPoints();
		SL_GetHighest();
		
		startPt->h = phyHorLoc.first;
		startPt->v = location.first + phyVerLoc.first;
		c1->h = xCtl0;
		c1->v = yCtl0;
		c2->h = xCtl1;
		c2->v = yCtl1;
		endPt->h = phyHorLoc.last;
		endPt->v = location.last + phyVerLoc.last;
	}

/*
 *	These routines continue to set the environment for this slur to be built.
 *	SetStemUp(FALSE,TRUE,FALSE,FALSE,FALSE) says the first note under the
 *	slur does not have an upward stem; the last note does have an upward stem,
 *	that the ending notes don't have stems up, that intermediate notes don't
 *	have stems up, and that all notes don't have stems up.  This is more general
 *	than need be, since only certain fields are applicable or even make sense.
 */

static void SL_SetStemUp(Boolean f, Boolean l, Boolean e, Boolean i, Boolean a)
	{
		stemUp.first |= f;
		stemUp.last  |= l;
		stemUp.ends  &= e;
		stemUp.inter &= i; if (i) intermediate = TRUE;
		stemUp.all	 &= a;
	}

static void SL_SetStemDown(Boolean f, Boolean l, Boolean e, Boolean i, Boolean a)
	{
		stemDown.first = f;
		stemDown.last  = l;
		stemDown.ends  = e;
		stemDown.inter = i; if (i) intermediate = TRUE;
		stemDown.all   = a;
	}

static void SL_SetAccentUp(Boolean f, Boolean l, Boolean e, Boolean i, Boolean a)
	{
		accentUp.first = f;
		accentUp.last  = l;
		accentUp.ends  = e;
		accentUp.inter = i; if (i) intermediate = TRUE;
		accentUp.all   = a;
	}

static void SL_SetAccentDown(Boolean f, Boolean l, Boolean e, Boolean i, Boolean a)
	{
		accentDown.first = f;
		accentDown.last  = l;
		accentDown.ends  = e;
		accentDown.inter = i; if (i) intermediate = TRUE;
		accentDown.all   = a;
	}

static void SL_SetBeamed(Boolean f, Boolean l, Boolean e, Boolean i, Boolean a)
	{
		beamed.first = f;
		beamed.last  = l;
		beamed.ends  = e;
		beamed.inter = i; if (i) intermediate = TRUE;
		beamed.all   = a;
	}

static void SL_SetBetweenLines(Boolean f, Boolean l, Boolean e, Boolean i, Boolean a)
	{
		betweenLines.first = f;
		betweenLines.last  = l;
		betweenLines.ends  = e;
		betweenLines.inter = i; if (i) intermediate = TRUE;
		betweenLines.all   = a;
	}

static void SL_SetStaffDegree(DDIST f, DDIST l, DDIST e, DDIST i, DDIST a)
	{
		staffDegree.first = f;
		staffDegree.last  = l;
		staffDegree.ends  = e;
		staffDegree.inter = i;
		staffDegree.all   = a;
	}
	
static void SL_SetStemLength(DDIST f, DDIST l, DDIST e, DDIST i, DDIST a)
	{
		stemLength.first = f;
		stemLength.last  = l;
		stemLength.ends  = e;
		stemLength.inter = i;
		stemLength.all   = a;
	}

/*
 *	Figure the physical vertical location for the starting and ending points
 *	of slur from the current environment (Section 3.5).
 */

static void SL_GetLocation()
	{
		DDIST dy,dy2;
		
		/* Figure out bowing direction (Section 3.2) */
		
		if (multipleVoices && upperVoice)		bowUp = TRUE;
		 else if (multipleVoices && lowerVoice) bowUp = FALSE;
		 else if (stemUp.all)					bowUp = FALSE;
		 else									bowUp = TRUE;
		
		/* Get the tip values (Section 3.3) */
		
		if ((accentUp.first & stemUp.first) || (accentDown.first & stemDown.first))
			tipValue.first = dyStaff / 2;
		 else if (beamed.first)
		 	tipValue.first = 0;
		  else
		 	tipValue.first = (-5*dyStaff) / 4;
		if ((accentUp.last & stemUp.last) || (accentDown.last & stemDown.last))
			tipValue.last = dyStaff / 2;
		 else if (beamed.last)
		 	tipValue.last = 0;
		  else
		 	tipValue.last = (-5*dyStaff) / 4;
		
		/* Get location offsets (Section 3.4) */
		
		if (multipleVoices) SL_GetMultiLocation();
		 else				SL_GetSingleLocation();
		
		/* Get absolute vertical and horizontal locations (Section 3.5) */

		dy = bowUp ? dyStaff : -dyStaff;
		dy2 = dy / 2;
		
		/* Get vertical location */
		
		phyVerLoc.first = staffDegree.first;
		if (betweenLines.first)
			phyVerLoc.first += (location.first==0) ? (3*dy)/4 : tipValue.first + dy/2;
		 else
			phyVerLoc.first += (location.first==0) ? (dy/4) : tipValue.first + dy/2;
			
		phyVerLoc.last = staffDegree.last;
		if (betweenLines.last)
			phyVerLoc.last += (location.last==0) ? (3*dy)/4 : tipValue.last + dy/2;
		 else
			phyVerLoc.last += (location.last==0) ? (dy/4) : tipValue.last + dy/2;
		
		/* Now get horizontal distance from notehead center to slur endpoints */
		
		phyHorLoc.first += slurLength / (100*dyStaff);
		if (location.first==0 || stemUp.first) phyHorLoc.first += dyStaff;
		
		phyHorLoc.last -= slurLength / (100*dyStaff);
		if (location.last==0 || stemUp.last) phyHorLoc.last -= dyStaff;
		
		/* Now offset both first and last horizontal positions by 1/2 a notehead */
		
		phyHorLoc.first += .6*dyStaff;
		phyHorLoc.last  += .6*dyStaff;
#if 1
		DebugPrintf("GetLocation: phyVerLoc.first = %d  phyVerLoc.last = %d\n",
					phyVerLoc.first, phyVerLoc.last);
		DebugPrintf("             phyHorLoc.first = %d  phyHorLoc.last = %d\n",
					phyHorLoc.first, phyHorLoc.last);
#endif
	}

/*
 *	Figure out the end displacement for slur in a single voice (from Section 3.4)
 *	location is the vertical offset from the center of the notehead to the place
 *	the slur should point to.
 */

static void SL_GetSingleLocation()
	{
		location.first = location.last = 0;
		
		if (intermediate)
			if (stemUp.ends && ~stemUp.inter) {
				location.first = stemLength.first + tipValue.first;
				location.last  = stemLength.last  + tipValue.last;
				return;
				}
		
		if (stemUp.first && stemDown.last) {
		 	if (beamed.first || accentUp.first) {
		 		if (staffDegree.last >= (staffDegree.first+stemLength.first+tipValue.first))
		 			location.first = stemLength.first + tipValue.first;
		 		}
		 	 else	/* No beams or accents */
		 		if (staffDegree.last>staffDegree.first &&
		 			staffDegree.last <= (staffDegree.first+stemLength.first+tipValue.first))
		 			location.first = staffDegree.last - staffDegree.first;
		 		 else
		 			location.first = stemLength.first + tipValue.first;
		 	return;
		 	}
		 			
		if (stemDown.first && stemUp.last) {
		 	if (beamed.last || accentUp.last) {
		 		if (staffDegree.first >= (staffDegree.last+stemLength.last+tipValue.last))
		 			location.last = stemLength.last + tipValue.last;
		 		}
		 	 else	/* No beams or accents */
		 		if (staffDegree.first > (staffDegree.last+(dyStaff<<1)) &&
		 			staffDegree.first <= (staffDegree.last+stemLength.last+tipValue.last))
		 			location.last = staffDegree.first - staffDegree.last;
		 		 else
		 			location.last = stemLength.last + tipValue.last;
		 	}
	}

/*
 *	Get the location of the slur's endpoints in the multivoice environment
 */

static void SL_GetMultiLocation()
	{
		location.first = location.last = 0;
		if (upperVoice) {
			location.first = stemLength.first + tipValue.first;
			location.last  = stemLength.last  + tipValue.last;
			}
		 else if (lowerVoice) {
		 	location.first = -stemLength.first - tipValue.first;
		 	location.last  = -stemLength.last  - tipValue.last;
		 	}
		 else
			SL_GetSingleLocation();
	}

/*
 *	Get the horizontal and vertical offset of the slur's first spline control
 *	point from the slur's first (left) endpoint (section 4.5).  Leave the answer
 *	in (xCtl0,yCtl0) and (xCtl1,yCtl1).
 *	This offset (xDist,yDist) is first calculated assuming that the heights
 *	of both the starting and ending points of the slur are the same.  The
 *	other control point is then chosen symmetrically, and the pair of control
 *	points is then rotated appropriately.
 */

static void SL_GetControlPoints()
	{
		FASTFLOAT x,y,dx,dy,angle,cs,sn,r;
		DDIST xDist2,yDist2;
		
		if (slurLength < (5*dyStaff))
			yDist = .2 * slurLength;
		 else if (slurLength < (25*dyStaff))
			yDist = (slurLength + 5*dyStaff) / 10;
		 else
			yDist = 3*dyStaff;
			
		if (slurLength < (4*dyStaff))
			xDist = (slurLength * 0.3) - (0.1 * dyStaff);
		 else if (slurLength < (25*dyStaff))
			xDist = (slurLength*3 + (15*dyStaff)) / 20;
		 else
			xDist = 2.16 * dyStaff;
		
		/* Now flip vertical (before rotating) if the slur should bow down */
		
		if (!bowUp) yDist = -yDist;
		
		/* And get symmetric second control point (again before rotating) */
		
		xDist2 = -xDist;
		yDist2 = yDist;
		
		/* Now rotate (if ends are not the same height) */
		
		dx = slurLength;
		dy = phyVerLoc.last - phyVerLoc.first;
		
		if (dy != 0) {

			/* Get cos and sin of angle to rotate */
			
			r = sqrt(dx*dx + dy*dy);
			cs = dx / r;
			sn = dy / r;
			
			/* Rotate (xDist,yDist) (a relative vector) by that angle */
			
			x = cs * xDist - sn * yDist;
			y = sn * xDist + cs * yDist;
			
			/* And get other symmetric control point under same rotation */
			
			xDist2 = -cs * xDist - sn * yDist;
			yDist2 = -sn * xDist + cs * yDist;
			
			xDist = x;
			yDist = y;
			}
		
		/* Get absolute control points */
		
		xCtl0 = phyHorLoc.first + xDist;
		yCtl0 = phyVerLoc.first + yDist;
		
		xCtl1 = phyHorLoc.last + xDist2;
		yCtl1 = phyVerLoc.last + yDist2;
	}

/*
 *	Adjust the current slur so that its highest (lowest) point stays away from
 *	a staff line.  This entails taking the derivative of the Bezier
 *	equation in Y, and finding the parametric solution for when it equals 0.
 *	From Section 4.4.
 */

#define TOL .00000001

static void SL_GetHighest()
	{
		DDIST x1,y1,x2,y2,x3,y3,x4,y4,yHighest;
		FASTFLOAT t,r,numer,denom,zero;
		
		/* Get Bezier points translated to origin */
		
		x1 = y1 = 0;
		x2 = xCtl0 - phyHorLoc.first; y2 = yCtl0 - phyVerLoc.first;
		x3 = xCtl1 - phyHorLoc.first; y3 = yCtl1 - phyVerLoc.first;
		x4 = phyHorLoc.last - phyHorLoc.first; y4 = phyVerLoc.last - phyVerLoc.first;
		
		/* Get (+,-) square root term in quadratic formula (derivative of Bezier eq.) */
		
		r = 2.0*y2 - y3; r = r*r;
		r -= y2 * (3.0*y2 - 3.0*y3 +y4);
		r = sqrt(r);
		
		/*
		 *	Get parametric highest point, using whichever (+,-) square root works.
		 *	Note that a symmetric curve with both endpoints 0 means that both
		 *	numerator and denominator here will be 0 (and t should be 1/2).
		 */
		
		numer = 2.0*y2 - y3 - r;
		denom = 3.0*y2 - 3.0*y3 + y4;
		if (numer>-TOL && numer<TOL && denom>-TOL && denom<TOL) t = 0.5;
		 else if (denom>-TOL && denom<TOL) t = 0.0;
		 else {
			t = numer / denom;
			if (t < 0.0 || t>1.0) t = (numer+2.0*r) / denom;
			}
		
		/* But clip it to endpoints if outside range of spline (no maximum) */
		
		if (t < 0.0) t = 0.0; else if (t > 1.0) t = 1.0;
		
		say("Highest or lowest point on slur is %f2.0%% along it\n",100*t);
		
		/* Convert parameter to actual Y value by plugging it back into spline eq. */
		
		yHighest = t * ( t * (t*(3.0*y2-3.0*y3+y4) + (3.0*y3-6.0*y2)) + 3.0*y2);
		
		/* And top of slur in absolute coordinates */
		
		SDTop = phyVerLoc.first + yHighest;
		
	}


#endif /* NOTYET */
