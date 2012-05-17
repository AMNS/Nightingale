/* NightMacHeaders.h - source to MacHeaders, configured for Nightingale. */

#ifdef __MWERKS__

/*
 *	MacHeaders.c
 *
 *	Script to generate the 'MacHeaders<xxx>' precompiled header for Metrowerks C/C++.
 *  Copyright © 1993 metrowerks inc.  All rights reserved.
 */

/*
 *	Required for c-style toolbox glue function: c2pstr and p2cstr
 *	the inverse operation (pointers_in_A0) is performed at the end ...
 */

#if !(powerc || __CFM68K__)
 #pragma d0_pointers on
#endif

/*
 *	Metrowerks-specific definitions
 *
 *	These definitions are commonly used but not in Apple's headers. We define
 *	them in our precompiled header so we can use the Apple headers without modification.
 */

#define PtoCstr		p2cstr
#define CtoPstr		c2pstr
#define PtoCString	p2cstr
#define CtoPString	c2pstr

#define topLeft(r)	(((Point *) &(r))[0])
#define botRight(r)	(((Point *) &(r))[1])

#define TRUE		true
#define FALSE		false

#ifndef powerc
 #include <MixedMode.h>
 long GetCurrentA5(void)
  ONEWORDINLINE(0x200D);
#endif

/*
 *	Apple #include files
 *
 *	Uncomment any additional #includes you want to add to MacHeaders.
 */

//	#include <ADSP.h>
	#include <AEObjects.h>
	#include <AEPackObject.h>
	#include <AERegistry.h>
	#include <AEUserTermTypes.h>
//	#include <AIFF.h>
	#include <Aliases.h>
//	#include <AppleEvents.h>
//	#include <AppleGuide.h>
	#include <AppleScript.h>
//	#include <AppleTalk.h>
//	#include <ASDebugging.h>
//	#include <ASRegistry.h>
//	#include <Balloons.h>
//	#include <CMApplication.h>
//	#include <CMComponent.h>
//	#include <CodeFragments.h>
	#include <ColorPicker.h>
//	#include <CommResources.h>
//	#include <Components.h>
	#include <ConditionalMacros.h>
//	#include <Connections.h>
//	#include <ConnectionTools.h>
	#include <Controls.h>
	#include <ControlDefinitions.h>
//	#include <ControlStrip.h>
//	#include <CRMSerialDevices.h>
//	#include <CTBUtilities.h>
//	#include <CursorCtl.h>
//	#include <CursorDevices.h>
//	#include <DatabaseAccess.h>
//	#include <DeskBus.h>
	#include <Devices.h>
	#include <Dialogs.h>
//	#include <Dictionary.h>
//	#include <DisAsmLookup.h>
//	#include <Disassembler.h>
	#include <DiskInit.h>
//	#include <Disks.h>
//	#include <Displays.h>
//	#include <Drag.h>
//	#include <Editions.h>
//	#include <ENET.h>
//	#include <EPPC.h>
//	#include <ErrMgr.h>
	#include <Errors.h>
	#include <Events.h>
//	#include <fenv.h>
	#include <Files.h>
//	#include <FileTransfers.h>
//	#include <FileTransferTools.h>
	#include <FileTypesAndCreators.h>
//	#include <Finder.h>
	#include <FixMath.h>
	#include <Folders.h>
	#include <Fonts.h>
//	#include <fp.h>
//	#include <FragLoad.h>
//	#include <FSM.h>
	#include <Gestalt.h>
//	#include <HyperXCmd.h>
//	#include <Icons.h>
//	#include <ImageCodec.h>
//	#include <ImageCompression.h>
//	#include <IntlResources.h>
//	#include <Language.h>
	#include <Lists.h>
	#include <LowMem.h>
//	#include <MachineExceptions.h>
//	#include <MacTCP.h>
//	#include <MediaHandlers.h>
	#include <Memory.h>
	#include <Menus.h>
//	#include <MIDI.h>
	#include <MixedMode.h>
//	#include <Movies.h>
//	#include <MoviesFormat.h>
//	#include <Notification.h>
//	#include <OSA.h>
//	#include <OSAComp.h>
//	#include <OSAGeneric.h>
	#include <OSUtils.h>
	#include <Packages.h>
//	#include <Palettes.h>
	#include <Patches.h>			/* Not in the original Nightingale version; added 7/98 */
//	#include <Picker.h>
//	#include <PictUtil.h>
//	#include <PictUtils.h>
	#include <PLStringFuncs.h>
//	#include <Power.h>
//	#include <PPCToolbox.h>
	#include <Printing.h>
	#include <Processes.h>
//	#include <QDOffscreen.h>
	#include <Quickdraw.h>
//	#include <QuickdrawText.h>
//	#include <QuickTimeComponents.h>
	#include <Resources.h>
//	#include <Retrace.h>
//	#include <ROMDefs.h>
#ifndef powerc
//	#include <SANE.h>
#endif
	#include <Scrap.h>
//	#include <Script.h>
//	#include <SCSI.h>
	#include <SegLoad.h>
//	#include <Serial.h>
//	#include <ShutDown.h>
//	#include <Slots.h>
	#include <Sound.h>
//	#include <SoundComponents.h>
//	#include <SoundInput.h>
//	#include <Speech.h>
	#include <StandardFile.h>
//	#include <Start.h>
	#include <Strings.h>
//	#include <Terminals.h>
//	#include <TerminalTools.h>
	#include <TextEdit.h>
//	#include <TextServices.h>
	#include <TextUtils.h>
//	#include <Threads.h>
//	#include <Timer.h>
	#include <ToolUtils.h>
//	#include <Translation.h>
//	#include <TranslationExtensions.h>
	#include <Traps.h>
//	#include <TSMTE.h>
	#include <Types.h>
//	#include <Unmangler.h>
//	#include <Video.h>
	#include <Windows.h>
//	#include <WorldScript.h>


/*
 *	required for c-style toolbox glue function: c2pstr and p2cstr
 *	(match the inverse operation at the start of the file ...
 */

#if !(powerc || __CFM68K__)
 #pragma d0_pointers reset
#endif

#else

/* Assume THINK C. This is the THINK C 7 version, simplifed from Symantec's original. */

/*
 *  Following are the original Symantec comments; CAVEAT: not all apply
 *  to the Nightingale version.
 *
 *  There are some conflicts and order dependencies among the various
 *  headers:
 *
 *	•	<LoMem.h> and <SysEqu.h> cannot both be included.
 *
 *	•	<asm.h> and <Traps.h>, if both are included, must appear
 *		in that order.  If <Traps.h> is included, traps used in
 *		inline assembly must appear without leading underscores.
 *
 *  If the "Check Pointer Types" option is disabled during the
 *  precompilation process, trap definitions will be stored in
 *  such a way that when a trap is called (in a source file that
 *  #includes the precompiled header), pointer arguments to the
 *  trap will not be matched against the types of corresponding
 *  formal parameters.
 *
 *  This is accomplished by storing "simplified" prototypes for
 *  traps, in which any argument of pointer type is stored as
 *  "void *".  The result closely resembles the treatment of traps
 *  in pre-5.0 versions of THINK C.
 *
 *  (Note that this file is written in such a way that it is immune
 *  to the actual compiler setting of "Check Pointer Types".  Use
 *  the "SIMPLIFY_PROTOTYPES" macro, below, to control whether full
 *  prototypes are retained.)
 *
 *	Note that none of the foregoing applies to Symantec C++, nor to 
 *	Symantec C/C++ for MPW. This file is #included by the source for
 *	MacHeaders++, the precompiled header for Symantec C++. If a compiler
 *	other than THINK C precompiles this file, asm.h and LoMem.h are
 *	#if'd out. If an MPW-hosted compiler precompiles this file, BDC.h
 *	and pascal.h are #if'd out.
 *
 */

#ifndef _H_MacHeaders
#define _H_MacHeaders


#ifndef __SC__

// set this to 0 (zero) to retain full prototypes
// set this to 1 (one) for "simplified" prototypes
#define SIMPLIFY_PROTOTYPES		1


// prototype checking level
// MAS : CodeWarrior-esque macros?
//#if SIMPLIFY_PROTOTYPES
//	#if !__option(check_ptrs)
//		#undef SIMPLIFY_PROTOTYPES
//	#endif
//	#pragma options(!check_ptrs)
//#else
//	#if __option(check_ptrs)
//		#undef SIMPLIFY_PROTOTYPES
//	#endif
//	#pragma options(check_ptrs)
//#endif

#endif

//		#ifdef __EPPC__
//			#include <AppleEvents.h>
//		#else
//			#define __EPPC__				// suppress unnecessary #include <EPPC.h>
//			#include <AppleEvents.h>
//			#undef __EPPC__
//		#endif

//#include <Controls.h>
//#include <Desk.h>
//#include <Devices.h>
//#include <Dialogs.h>
//#include <DiskInit.h>
//#include <EPPC.h>
//#include <Errors.h>
//#include <Events.h>
//#include <Files.h>
//#include <Finder.h>
//#include <FixMath.h>
//#include <Fonts.h>
//#include <GestaltEqu.h>
//#include <Lists.h>
//#include <Memory.h>
//#include <Menus.h>
//#include <Notification.h>
//#include <OSEvents.h>
//#include <OSUtils.h>
//#include <Packages.h>
//#include <Palettes.h>
//#include <Picker.h>
//#include <Printing.h>
//#include <Processes.h>
//#include <Quickdraw.h>
//#include <Resources.h>
//#include <Scrap.h>
//#include <Script.h>
//#include <SegLoad.h>
//#include <StandardFile.h>
//#include <TextEdit.h>
//#include <Timer.h>
//#include <ToolUtils.h>
//#include <Types.h>
//#include <Windows.h>

//#include <pascal.h>

#ifdef THINK_C
	#include <asm.h>
#else
#endif

//#if defined(__CONDITIONALMACROS__)
//	#include <LowMem.h>
//#elif !defined(__cplusplus) && !defined(__SYSEQU__)
//	#include <LoMem.h>
//#endif

/*  replaces the old stuff below
#if 1 && !defined(__cplusplus)
	#include <LoMem.h>
#endif
*/

#ifndef __SC__

// restore "Check Pointer Types" to previous setting
#if SIMPLIFY_PROTOTYPES
	#pragma options(check_ptrs)
#elif defined(SIMPLIFY_PROTOTYPES)
	#pragma options(!check_ptrs)
#endif
#undef SIMPLIFY_PROTOTYPES

#endif
#endif	/* ndef _H_MacHeaders */

#endif	/* def __MWERKS__ */
