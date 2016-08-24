Nightingale
===========

Development environment requirements
------------------------------------
Downloads require an Apple Developer account; they should not require a paid iOS or MacOS subscription.
 - NB: when installing Xcode/Developer Tools, be sure the MacOS 10.4 SDK installation option is selected; it may not be selected by default.

* MacOS version 10.5
 - Xcode version 2.5 with MacOS 10.4 SDK and GCC version 4.0 support added
 - should work on OS 10.4 too, but only 10.5 has been tested

* MacOS version 10.6
 - Xcode version 3.2 with MacOS 10.4 SDK, PowerPC, and GCC version 4.0 support added
 - will not work on OS >= 10.7, due to PowerPC / Rosetta dependencies

It will probably work with GCC 4.2 as well.

* The following environment _may_ work; it did for one person (Geoff C.), but another (Don B.) was unable to set it up (note the kludgy instructions!):

* MacOS version 10.6; Xcode version 4.2 with MacOS 10.4 SDK, PowerPC, and GCC version 4.0 support added 
 - Specifically, this is a hybrid of [Xcode 4.2 for MacOS 10.6] (http://adcdownload.apple.com//Developer_Tools/xcode_4.2_for_snow_leopard/xcode_4.2_for_snow_leopard.dmg)
 with [Xcode 3.2's for MacOS 10.6](http://adcdownload.apple.com//Developer_Tools/xcode_3.2.6_and_ios_sdk_4.3__final/xcode_3.2.6_and_ios_sdk_4.3.dmg) SDKs.
 Following [these instructions] (http://stackoverflow.com/questions/5333490/how-can-we-restore-ppc-ppc64-as-well-as-full-10-4-10-5-sdk-support-to-xcode-4
) restores support for the SDKs, but seems to fail in Step 4 (restoring support for GCC 4.0). At that point, run these commands:

```
curl -O https://raw.github.com/thinkyhead/Legacy-XCode-Scripts/master/restore-with-xcode3.sh
chmod 744 restore-with-xcode3.sh
./restore-with-xcode3.sh
curl -O https://raw.github.com/thinkyhead/Legacy-XCode-Scripts/master/restore-with-xcode4.sh
```

The last "curl" may fail, but (for one person) resulted in Xcode 4.2 doing what we want.


Running and Debugging
---------------------
It's probably not possible to attach a debugger on an Intel machine (due to Rosetta translation requirements), but it should be possible on a PowerPC.

Xcode 2.x and 3.x put debug build products in a directory with a path like:

`~/NightingaleDev/build/Debug`

XCode 4.x and above put debug build products in a directory with a totally unguessable path like:

`~/Library/Developer/Xcode/DerivedData/Nightingale-dghtzivoyrfkjudiupfaqdqicrev/Build/Products/Debug/`

That path can be found (in a terminal window, of course) using:

`find ~/* -name Nightingale.app`

...or -- much, much faster! -- by control-clicking on "Nightingale" at the bottom in the folder view (leftmost icon(?)) of the project, then choosing "Show in Finder" in the pop-up menu.

The built application can be run via:

`open -a ~/Library/.../Build/Products/Debug/Nightingale.app`

or simply:

`Nightingale.app/Contents/MacOS/Nightingale`

The latter is somewhat helpful, since stderr/out will be printed to the command line. But either way, logging information will be written to system.log, which can be viewed with the Console utility.

NB: The same logging information will appear in system.log even when a normal (non-debug) build of Nightingale is run in the standard way, via opening the icon in the Finder.

NB2: You can run Nightingale inside Xcode 2.x in the obvious way from the Build menu. But with Xcode 3.x, that doesn't seem to work! 
