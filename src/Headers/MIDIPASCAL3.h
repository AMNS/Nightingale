/* MP2.2.h header file for MidiPascal 3.0 MIDI Driver, used with LSC4.0
	(c) 1990 James Chandler Jr. and Altech Systems.
	This version dated 7/30/90
*/

pascal void InitMidi(unsigned int InSize, unsigned int OutSize);
/*
	InSize and OutSize passed by value
	InSize and OutSize are the number of timestamped midi values allowed
		in the buffers, so InitMidi actually allocates buffers
		four times the size of InSize and OutSize.
	Call initmidi() early in your program to avoid memory fragmentation.
*/

pascal void MidiIn(int * MidiByte, long * TimeStamp);
/*
	MidiByte and TimeStamp passed by reference
	If input buffer is empty, MidiByte has a value of -1 and TimeStamp
		contains the current time.
*/		

pascal void MidiOut(int Midibyte, long TimeStamp, int * Result);
/*
	MidiByte and TimeStamp are passed by value.
	Result is passed by reference.
	If Result equals 1 (true), MidiOut was successful.
	If Result equals 0 (false), the output buffer is full.
	If TimeStamp is zero, or less than the current Tick time,
		Midibytes are transmitted immediately, except when a byte with a 
		high TimeStamp preceeds bytes with lower TimeStamps.  In this case,
		the high TimeStamp byte will hold off all following bytes until it's
		transmission time.
	It is therefore best to midiout bytes in a time-ordered sequence for 
		accurate playback.
	MidiPascal does not sort non-sequential MIDI messages the way Midi
		Manager does, but I am sure you know that there is no such thing as
		a truly fast sort routine.  MidiPascal should be more efficient,
		especially on Mac Pluses and SEs.
*/

pascal void MidiNow(int Statusbyte, int Databyte1, int Databyte2, int * Result);
/*
	Statusbyte, Databyte1, and Databyte2 passed by value.
	Result is passed by reference.
	Result is 1 if MidiNow is successful.
	MidiNow uses a 256 byte buffer.
	MidiNow will return false if the midinow buffer is full.
	Unused arguments should be -1 or 255 (-1 for a char).
	The intended purpose of midinow is to make it easy to write applications
		which remap incoming MIDI data in real time while playing a sequence.
		Midiout the time-stamped sequence data and midinow the immediate
		data.
*/

pascal void GetMidi(unsigned char * MidiString, long RequestCount, long * ReturnCount);
/*
	MidiString and ReturnCount passed by reference.
	RequestCount passed by value.
	MidiString is not a pascal or c string.  MidiString is merely 
		a mac memory address, obtained by using the NewPtr toolbox
		call, or the NewHandle call followed by HLock, or merely dimensioning
		a C unsigned character array (if the dump is small).  Global C 
		variables cannot exceed 32K total, so you will usually have to 
		request memory from the system instead of dimensioning arrays.
	RequestCount is a long so we can request dumps bigger than an int.
	ReturnCount is the number of bytes received before timeout (about 1.5 
		seconds on a Mac Plus).
	Getmidi() is primarily intended to facilitate reception of bulk sysex.
	Bulk sysex will be guaranteed to be completely "clean" if you filter
		all status bytes except $F0 and $F7.
*/	

pascal void SendMidi(unsigned char *MidiString, long SendCount, int *Result);
/*	
	Midistring and Result passed by reference.
	SendCount passed by value.
	MidiString is a pointer to a mac memory block.
	SendCount tells SendMidi how many bytes to transmit starting at 
		MidiString address.
	Result = 1 if SendMidi is successful.
	Result = 0 if the computer is still sending (or waiting to send)
		a previous SendMidi call.  User code should wait and try
		again until Result = 1.
	Sendmidi immediately returns control to your program after cueing the
		midistring pointer, so your program can be doing other things while
		a long bulk dump is transmitting.
	If timestamped midi is being played from the transmit buffer, or midithru
		or midinow action is taking place, the midistring will wait
		until the current midi message is complete before transmitting, so
		you do not have to take unusual precautions against hung notes.
	You can easily write a patch editor which plays a sequence while you
		edit on-screen or play a keyboard thru the computer.  Everytime you
		SendMidi, the sysex stream will merge seamlessly with the other
		midi streams.
*/

pascal void InCount(unsigned int *Count);
/*	
	Returns a count of timestamped bytes (four bytes per each count) which
		are currently in the input buffer.
	One usually has to periodically poll GetNextEvent while reading midi 
		input, to make sure the program does not hang in never-never land
		if a received midi message does not have the expected syntax. 
		Excessive polling of GetNextEvent can significantly slow your 
		program's throughput, especially under multifinder.
	I deal with this by polling incount and GetNextEvent until one or the
		other becomes true.  Then I deal with midi input until incount
		is zero without polling GetNextEvent, maximizing midi thruput
		without risking an infinite loop.
*/

pascal void OutCount(unsigned int *Count);
/*	
	Returns the count of timestamped bytes in the timestamped output buffer
		(four bytes per each count).
*/	

pascal void MidiThru(int);
/*
	Pass a zero to turn thru off, or a 1 to turn thru on.
	Midithru uses a 256 byte buffer, and merges incoming data with midiout,
		midinow, and sendmidi calls.
	Merging works best if all messages you output do not use running
		status.  Merging also works fine (no hung notes) if you do midiout 
		running status, but timing will be less than optimal and under 
		worst case the midithru buffer could over-run and cause stuck notes.
	Midithru only passes channelized bytes.  No message with a status
		greater than $EF will ever be thrued.
*/

pascal void ReChannel(int);
/*
	Pass 1 thru 16 to select an input rechannel, any other int to turn off.
	If midithru is on, the rechanneled bytes get thrued.
	When writing sequencing applications, it is commonplace to leave the
		master controller on its channel, and rechannelize the controller
		when recording tracks for other synths.  The rechannel command 
		makes this trivially easy to do, adding minimal overhead to your 
		program.
*/

pascal void Midi(int Arg);
/*
	Midi(0) completely resets the serial port, then reinstalls it, clearing
		input, output, midinow and midithru buffers in the process.  If you
		call midi(0) before each major use of the midi ports, you can be
		reasonably sure no other application or DA has stolen the ports
		from you.
	Midi(5) clears the input buffer and Midi(6) clears the output buffer.
*/	

pascal void MidiPort(int Arg);
/*
	midiport(0) - Select 0.5 MHz interface speed
	midiport(1) - Select 1.0 MHz interface speed
	midiport(2) - Select 2.0 MHz interface speed
	midiport(3) - Select Communications port
	midiport(4) - Select Printer port
	midiport(5) - Experimental - set up FAST mode for Midi Time Piece Iface
*/	

pascal void MidiFilter(int EnaDisa, int Lower, int Upper);
/*
	EnaDisa tells whether to install or remove a filter.
	Lower and Upper must be status bytes (greater than $7F).
	MidiFilter of bytes greater than 247 does not affect running status.
	If a running status between a filter's Lower and Upper is received,
		all subsequent data bytes are filtered until running status
		changes to a non-filtered value.  This is the way it should
		be to filter a MIDI stream.
	To filter all sysex, one would do two calls:
		MidiFilter(1, 240, 240);
		MidiFilter(1, 247, 247);
	There are 16 filter locations, and filters are dynamically allocated/
		de-allocated so we don't have to refer to them by number.
	Your program should keep up with how many filters it uses if it could
		conceivably install more than 16 filters.  Installing more than 
		16 filters will not crash anything, midifilter will simply ignore
		them.
	When midifilter removes a filter, it matches for the lower value only.
	For best results your program should only remove the same filter pairs
		previously installed. For instance, don't do a midifilter(1,144,147)
		followed later with a midifilter(1,144,149). If you install the same 
		filter twice, you will have to remove it twice to get rid of it.
	The Receive interrupt handler only checks the number of filters active,
		 so if no filters are active, the filter routine is entirely 
		 bypassed, slightly enhancing efficiency.
*/		

pascal void MidiTime(unsigned int ticklength);
/*	ticklength = 0 		turns off timestamping
	ticklength = 782	1 millisecond interrupt
	ticklength = 1564	2 millisecond interrupt
	To calculate ticklength from beats per minute, use this formula:
	
				(782000 ticklength per sec) * (60 secs per minute)
ticklength = 	--------------------------------------------------
					(ticks per beat) * (beats per minute)
	
	So for 192 ticks per beat, ticklength = 244375/BPM
	
	192 or 240 ticks per beat seem to be good figures for a Mac sequencing
		application, because this allows tempos down below 10 BPM before
		overflowing an unsigned int, and allows tempos up to 300 BPM or so
		before interrupts occur more often than 1 millisec.
	If timing interrupts occur more often than 1000 or so per second, on a 
		mac plus there is very little time left over for your application
		to run in, especially under multifinder.
	Miditime(0) completely disconnects the timer from MidiPascal, and all 
		midiins or gettimes return a timestamp of zero (except when synching
		to external midi source).
	If the timer is off, a non-zero miditime() call sets tickcount to zero and
		"steals" the timer for MidiPascal's use. Subsequent non-zero 
		miditime() calls only change tempo, and do not reset time to zero,
		so your program can follow a tempo map by keeping up with the time 
		and making miditime calls when appropriate.
	Beeps other than simple beep steal the timer, and are to be avoided.
*/

pascal void MidiStopTime(void);
/*
	Leaves the timer running, but does not update the tickcount.
	In addition, when in syncout mode (see midisync command) stoptime
		sends a 252 (STOP) command.
*/

pascal void MidiStartTime(void);
/*
	Reverses the effect of stoptime(), enabling the tickcount to advance.
	If in syncout mode, starttime() will send a 250 (START) command if
		tickcount equals zero, and a 251 (CONTINUE) command if tickcount
		is not zero.  This conforms with the IMA's definition of how 
		these commands should be used with syncout.
*/

pascal void MidiSetTime(long timestamp);
/*
	Set the tickcounter.  Useful for cuing into a sequence, or relocating
		time during a chase.  If a sequence is stopped in the middle of 
		playback, one method of shipping out all relevant note-offs which
		are already in the output buffer is to settime to a high value,
		which causes the buffer to be transmitted posthaste.
*/
pascal void MidiGetTime(long * timestamp);
/*
	Return the current tickcount.
*/

pascal void MidiSync(int mode, int syncticks);
/*
	Mode = 0  --- No sync in or out.
	Mode = 1  --- Sync in.
	Mode = 2  --- Sync out.
	Sync in mode causes the tickcounter to increment based on incoming $F8
		(MIDI CLOCK) bytes.  Syncticks is a multiplier which aligns the
		application's internal resolution to the standard 24 ticks per beat
		of MIDI clock.  For 192 ticks per beat internal resolution,
		syncticks = 8.  For 240 ticks per beat, syncticks = 10.
	While in sync in mode, a received START command sets the tickcounter
		to zero, and enables tickcount to respond to MIDI CLOCK.  A 
		CONTINUE enables tickcount, but does not reset the tickcounter.
		A received STOP command disables tickcount response to MIDI CLOCK.
		This conforms to the IMA's definition of how syncin should work.
	Sync out mode sends MIDI CLOCK bytes at a rate of the internal resolution
		divided by syncticks, so to send 24 ticks per beat from a 192
		tick per beat internal resolution, use 8, just as with syncin.
	While in sync out mode, stoptime() sends a STOP byte, and starttime()
		sends either a START or CONTINUE, depending on whether tickcount is
		zero.
	Song position pointer is complex enough that your application will have
		to take most responsibility for dealing with this, if you decide
		to implement it.
	Midi Time Code is currently not supported, although it will be soon.  
		However, LSC 4.0 can call midiin() more than ten thousand times per
		second on a Mac plus, so your application should be able to pretty
		closely chase SMPTE time by polling Midi input.
*/

pascal void QuitMidi(void);
/*
	Removes all interrupt handlers and discards memory allocated by
		MidiPascal.  If you do not call quitmidi before the program 
		terminates, the Mac will almost certainly crash.
*/