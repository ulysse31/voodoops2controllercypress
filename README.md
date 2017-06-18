<h1>VoodooPS2Controller for Cypress PS2 touchpad</h1>
This project was based on a fork of VoodooPS2Controller from rehabman.<br />
The goal was to implement support for cypress PS2 on it : Goal is now completed, since we can now detect up to 5 fingers, even on cypress firmwares that officially only supports up to 3 fingers. <br />
Rehabman sources were modified to allow/add new messages to VoodooPS2Keyboard, allowing new type of gestures/shortcuts, feeting a bit more with 4 and 5 fingers detection.<br />
And have also created our own class/object instance (VoodooPS2CypressTouchPad) to detect and manage cypress ps2 touchpads.<br />
<b>I do all this for my personal fun, but if some good souls are happy with it and wants to make a donation<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=ulysse31%40gmail%2ecom&lc=FR&item_name=ulysse31&no_note=0&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHostedGuest"> click here</a></b>
<br />

<b>News:</b> v0.7 is out ! New features, bug corrections and a working prefpane !<br />
<b>01-12-14: New version available: v0.7 => Everyone should use this release !</b> this last version is stable and has a working prefpane allowing to set/customized advanced settings<br />
<b>Installation Note:</b> Backup all your actual PS2 related kext ! since this version is forked from rehabman's one: you do not need AppleACPIPS2Nub.kext/ApplePS2Controller.kext and <b>must</b> remove it if you have it or <b>your system will crash</b>.<br />

For now, you can still test multiple versions : 
<ul>
<li>v0.1 <a href="https://mega.co.nz/#!h5wxyD6b!S15aBHOJCsZLNTadG997fgT6e1wCzWD0RqzogCL53lc">here </a><b>(Deprecated)</b></li>
<li>v0.2 <a href="https://mega.co.nz/#!41BSXJ4C!b09J-yD1aEV5JeHRUDebkjtEfWtLbGXxfkBnhwZkhKg">here </a><b>(Deprecated)</b></li>
<li>v0.3<a href="https://mega.co.nz/#!MhQw0LBY!bLorvyqsfO5lOjA8AOwWthPuPtxPOYsOcrBIQSg8h1c"> here </a><b>(Deprecated)</b></li>
<li>v0.4<a href="https://mega.co.nz/#!B4ITXC6T!FbCkQ0Wo3LFvG_dFi6gpe18eZFlRduoUecVdvPpeugs"> here </a><b>(Deprecated)</b></li>
<li>v0.5<a href="https://mega.co.nz/#!l4BxSRoZ!JpIZkVpxx7TJzf99avhTfyyosU4w8Js7zxVpgTPfpck"> here </a><b>(Deprecated)</b></li>
<li>v0.6<a href="https://mega.co.nz/#!o8YCmYRC!ehz1Qn_AJrngTX3zLZRzMl-cpVcKQ2cq6XUzbn4KWIA"> here </a><b>(Deprecated)</b></li>
<li>v0.7<a href="https://mega.co.nz/#!w94AXTra!CHDT0UMuC7jJ0s5C9eaQRHnZIolqKej27i1rldfThAA"> here (mega)</a> and <a href="http://www.4shared.com/zip/H8Qwo-hJce/VoodooPS2Controllerkext.html"> here (4shared)</a><b> New Release !</b></li>
<li>v0.7 prefPane<a href="https://mega.co.nz/#!kg41BaSb!Ji6ipV_eAbtcuY6BsdIJClghg-R0tfn3JxkVh9DlzeE"> here (mega)</a> and <a href="http://www.4shared.com/zip/qr1KXG7lba/VoodooPS2cypressPaneprefPane.html"> here (4shared)</a><b> New Release !</b></li>
</ul>

<b>Basic settings can be now changed on the System Preferences => Trackpad</b><br />
<b>Advanced settings can be now changed with the cypress prefpane, you can know easily which parameters to set in the VoodooPS2Trackpad Info.plist</b><br />

Actual Features:
<ul>
<li>tap to click</li>
<li>dragging <b>New !</b></li>
<li>one finger triple tap draglock (one tap to release) <b>New !</b></li>
<li>two fingers scrolling (no smooth momentum)</li>
<li>two fingers tap for right click</li>
<li>3 fingers tap/window move/select (aka 3 fingers mouse drag)</li>
<li>4 fingers swipe: need to have set on System Preferences => keyboard input shortcut to control+command+[up/down/left/right]</li>
<li>5 fingers lock screen : keep your 5 fingers for arround a second and leave the pad </li>
<li>5 fingers sleep mode : keep your 5 fingers on pad for at least 3 secs</li>
<li>a working prefpane to change in live advanced values on the kext and easily know what to set on the VoodooPS2Trackpad Info.plist <b>New !</b></li>
</ul>

If you have any questions/remarks, feel free to add an issue or you can visit the official topic at InsanelyMac Forum <a href="http://www.insanelymac.com/forum/topic/294608-cypress-ps2-trackpad-kext/">here</a><br />




<h2>CHANGELOG</h2>

<ul>

<li>v0.7</li>

<li>corrected bugs and added optimizations</li>
<li>cleaned a bit the code</li>
<li>added kalman filtering (smooth) on multifinger gestures and pressure calc</li>
<li>added viable communication between kext and prefpane</li>
<li>added a prefpane that allows changing advanced settings in live and quick Info.plist edition</li>
<li> all values can now be set with the VoodooPS2Trackpad Info.plist</li>
<br />


<li>v0.6</li>

<li>changed all taps to use real timers (better finger tapping for 1 to 5 fingers)</li>
<li>cleaned a bit the code</li>
<li>enhanced 4 fingers swipes</li>
<li>scrolling speed can be now scalled on the trackpad preferences</li>
<li>5 fingers lock screen : keep your 5 fingers for arround a second and leave the pad : it launches the OSX shortcut keys for screen locking (Control+Shift+Eject)</li>
<li>5 fingers sleep mode : keep your fingers for at least 3 secs : it launches the OSX sleep shortcut keys (Command+Option(Alt)+Eject)</li>
<li>2 fingers tap right click : 2 fingers tap gives a right click</li>
<br />


<li>v0.5:</li>

<li>Handles Buggy firmwares (v34, v35) that do not follow cypress own documentation (?!) sending 0x04 header packet instead of 0x20 for 4 fingers packets.</li>
<li>Now basic settings can be modified on System Preferences => Trackpad !</li>
<br />

<li>v0.4:</li>

<li>Added tap to click for XPS 15z ( framerate superior on firmware v34 v35 to firmware v11)</li>
<li>corrected bug for firmwares superior to v11 that does not handle rate and resolution to 200dpi 8count/mm</li>
<br />

<li>v0.3 :</li>

<li>Added 3 fingers tap/window move</li>
<li>Added 4 fingers swipe</li>
<br />

<li>v0.2:</li>

<li>smoothed mouse moves</li>
<br />

<li>v0.1:</li>

<li>first release : basic features => tap to click, 2 fingers scrolling</li>

 </ul>
