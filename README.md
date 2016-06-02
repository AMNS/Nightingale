Nightingale
===========

Development environment requirements
------------------------------------
Downloads require an Apple Developer account; they should not require a paid iOS or MacOS subscription.
 - NB: when installing Xcode/Developer Tools, be sure the MacOS 10.4 SDK installation option is selected; it may not be selected by default.

* MacOS version 10.5
 - Xcode version 2.5 with MacOS 10.4 SDK and GCC version 4.0 support added
 - should work on version 10.4 too, but only version 10.6 has been tested

* MacOS version 10.6
 - will not work on versions >= 10.7, due to PPC / Rosetta dependencies
 - Xcode version 3.2 with MacOS 10.4 SDK, PPC, and GCC version 4.0 support added
* The following environment _may_ work; it did for one person, but failed for another:

* MacOS version 10.6; Xcode version 4.2 with MacOS 10.4 SDK, PPC, and GCC version 4.0 support added 
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


Debugging
---------
It's not currently possible to attach a debugger on an Intel machine (due to Rosetta translation requirements).  It should be possible to debug on a PPC machine.

XCode puts debug build products in a directory like:

`~/Library/Developer/Xcode/DerivedData/Nightingale-dghtzivoyrfkjudiupfaqdqicrev/Build/Products/Debug/`

This can be found using:

`find ~/* -name Nightingale.app`

And run like:

`open -a ~/Library/.../Build/Products/Debug/Nightingale.app`

or simply:

`Nightingale.app/Contents/MacOS/Nightingale`

The latter is helpful, since stderr/out will be printed to the command line.
