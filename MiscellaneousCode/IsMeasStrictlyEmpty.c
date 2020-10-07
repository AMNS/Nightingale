/* Is the given Mesaure _strictly_ empty? By "strictly empty", we mean it contains
nothing at all, so this is a stricter test than is used by FillEmptyMeasures(),
which checks for just notes and rests. */

static Boolean IsMeasStrictlyEmpty(Document *doc, LINK measL);
static Boolean IsMeasStrictlyEmpty(Document *doc, LINK measL)
{
	if (RightLINK(measL)==doc->tailL) return TRUE;
	if (SystemTYPE(RightLINK(measL))) return TRUE;
	return FALSE;
}
