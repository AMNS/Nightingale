/*	EditScore.c for Nightingale */

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


/* Handle mouse clicks in a currently active score. <pt> must be in paper-relative
coordinates with respect to the current sheet, and the window local coordinates must
have already been set to reflect the current sheet as well. Always returns TRUE.
This should be called by DoDocContent. */

Boolean DoEditScore(Document *doc, Point pt, short modifiers, short dblClick)
	{
		Boolean shiftFlag,optionFlag,cmdFlag,capsLockFlag;

		shiftFlag = (modifiers & shiftKey)!=0;
		optionFlag = (modifiers & optionKey)!=0;
		cmdFlag = (modifiers & cmdKey)!=0;
		capsLockFlag = (modifiers & alphaLock)!=0;

		MEHideCaret(doc);
		
		switch (clickMode) {
			case ClickErase:										/* for debugging */
				DoClickErase(doc, pt);
				break;
			
			case ClickSelect:
				if (optionFlag || currentCursor==genlDragCursor) {
					if (dblClick) { DoOpenSymbol(doc, pt); break; }

					DeselAll(doc);
					
					if (DoSymbolDrag(doc, pt)) break;
					
					DoAccModNRClick(doc, pt); break;
				}
				
				if (currentCursor==arrowCursor) {
					if (dblClick) {
						LINK pL = DoOpenSymbol(doc, pt);
						if (pL==NILINK)						/* maybe user dbl-clicked on modifier */
							DoOpenModNR(doc, pt);
					}
					else		  DoSelect(doc, pt, shiftFlag, cmdFlag, capsLockFlag, SMSelect);
					break;
				}
				
				if (currentCursor==threadCursor)
					DoSelect(doc, pt, shiftFlag, cmdFlag, capsLockFlag, SMThread);
				else
					DoClickInsert(doc, pt);
				break;
			
			case ClickFrame:										/* for debugging */
				DoClickFrame(doc, pt);
				break;
		}
		DrawMessageBox(doc, TRUE);

	return TRUE;
	}


/* Erase and Inval the measure clicked in. */

void DoClickErase(Document *doc, Point pt)
	{
		LINK		pL;
		PMEASURE	pMeasure;
		Rect		tempR;
		
		pL = doc->headL;											/* find measure clicked in */
		while (pL!=doc->tailL) {
			if (MeasureTYPE(pL)) {
				pMeasure = GetPMEASURE(pL);
				if (LinkVALID(pL))
					if (PtInRect(pt, &pMeasure->measureBBox)) {
						tempR=pMeasure->measureBBox;
						OffsetRect(&tempR, doc->currentPaper.left, doc->currentPaper.top);
						EraseAndInval(&tempR);			/* force a screen update on it */
					}
				pL = LinkRMEAS(pL);
				if (!pL) pL = doc->tailL;
			}
			else pL = RightLINK(pL);
		}
	}

	
/* Frame objRects of objects in measure clicked in. */

void DoClickFrame(Document *doc, Point pt)
	{
		short 	clickStaff;
		LINK	prevMeasL, pL, pLPIL;
		
		clickStaff = FindStaff(doc, pt);								/* Find staff clicked on... */
		pLPIL = FindLPI(doc, pt, clickStaff, ANYONE, TRUE);				/*   and Last Prev. Item on it */
		prevMeasL = LSSearch(pLPIL, MEASUREtype, clickStaff, TRUE, FALSE);
		for (pL=RightLINK(prevMeasL); pL && !MeasureTYPE(pL); pL=RightLINK(pL)) {
			FrameRect(&LinkOBJRECT(pL));
			SleepTicks(30L);
			PenPat(NGetQDGlobalsWhite());											/* Erase objRect */
			FrameRect(&LinkOBJRECT(pL));
			PenPat(NGetQDGlobalsBlack());
			}
	}

void DoClickInsert(Document *doc, Point pt)
	{
		short index, subtype;
		DDIST xd; CONTEXT context;
		
		DeselAll(doc);

		doc->showWaitCurs = FALSE;		/* Avoid distracting cursor changes when inserting */
		
		index = GetSymTableIndex(palChar);
		switch (symtable[index].objtype) {
			case SYNCtype:
				InsertNote(doc, pt, doc->insertMode);
				break;
			case GRSYNCtype:
				InsertGRNote(doc, pt, doc->insertMode);
				break;
			case MEASUREtype:
				subtype = symtable[index].subtype;

				if ((subtype==BAR_RPT_L || subtype==BAR_RPT_R || subtype==BAR_RPT_LR)
											 && !config.dblBarsBarlines) {
					if (SymbolIsBarline())
						InsertMeasure(doc, pt);
					else
						InsertRptEnd(doc, pt);
				}
				else if (subtype==BAR_DOUBLE && !config.dblBarsBarlines) {
					if (SymbolIsBarline())
						InsertMeasure(doc, pt);
					else
						InsertPseudoMeas(doc, pt);
				}
				else								/* No ambiguity: single or final double, etc. */
					InsertMeasure(doc, pt);
				break;
			case PSMEAStype:
				InsertPseudoMeas(doc, pt);
				break;
			case CLEFtype:
				InsertClef(doc, pt);
				break;
			case KEYSIGtype:
				InsertKeySig(doc, pt);
				break;
			case TIMESIGtype:
				InsertTimeSig(doc, pt);
				break;
			case DYNAMtype:
				InsertDynamic(doc, pt);
				break;
			case MODNRtype:
				InsertMODNR(doc, pt);
				break;
			case RPTENDtype:
				InsertRptEnd(doc, pt);
				break;
			case ENDINGtype:
				InsertEnding(doc, pt);
				break;
			case GRAPHICtype:
				switch (symtable[index].subtype) {
					case GRArpeggio:
						InsertArpSign(doc, pt);
						break;
					case GRDraw:
						InsertLine(doc, pt);
						break;
					case GRChar:
						InsertMusicChar(doc, pt);
						break;
					default:
						InsertGraphic(doc, pt);
				}
				break;
			case SLURtype:
				InsertSlur(doc, pt);
				break;
			case TEMPOtype:
				InsertTempo(doc, pt);
				break;
			case SPACERtype:
				InsertSpace(doc, pt);
				break;
			default:
				break;
			}

		/*
		 * The thing just inserted may end up far away from where the user clicked,
		 * and if they now zoom in, it's very likely they want to look at the new
		 * symbol, not the spot where they clicked. Reset the zoom origin accordingly.
		 */			
		GetContext(doc, doc->selStartL, 1, &context);
		xd = PageRelxd(doc->selStartL, &context);
		doc->scaleCenter.h = context.paper.left+d2p(xd);
	}

/* Null routine to allow loading or unloading EditScore.c's segment. */

void XLoadEditScoreSeg()
	{
	}
