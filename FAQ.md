# Introduction #

You should find here answers to most often questions :

# Q/A #

Q:<i><b>Could you add gesture XXXX ?</b></i><br />
A: Short answer: maybe. Long Answer : For now we are limitated on PS2 dev to voodooPS2 basics, in order to implement all possible gestures, we need to change from IOHIDPointer to a new class (IOHIDFamily ?), but that represents a LOT of work since using a "Upper" class also means get rawer implementation to graphical hooks ... in other words, when someone will find a way to implement a trackpad (whichever material) using a Upper class, others will then follow. it does not means i will not work on that, i will, at least try to, but the workload being quite huge, don't expect it soon (on the next months) from my part at least. If you have in mind a feature, make an issue, and i would tell you if it's possible or not.

Q:<i><b>I have a bug/freeze/hang/whatsoever using your kext, what should i do ?</b></i><br />
A: Firstly, you need to be sure that you have followed the per-requisites on the installation note on main page, by removing all other mouse related kexts (AppleACPIPS2Nub.kext/ApplePS2Controller.kext), but also other "plugin" kexts like smoothmouse. if you are sure on that, then check your permissions. Finally, if everything is checked, use the following debug kext downloadable <a href='https://mega.co.nz/#!psBlBL4Z!e9ZEyxZch55sedpn9Zp2NltRd4QjhAnFRAYNh9kQkLQ'>here</a>(will keep link updated)<br />
Try to reproduce the bug with this kext loaded (you can also build one with the latest sources and use the debug version). Once the bug occurs, you have 2 things to do :<br />
<ul>
<li>write down date(s)/time(s) it occurs</li>
<li>extract logs by using the following commands and sent it to me: sudo cat /var/log/system.log | grep -i cypress > ~/cypress_dump.txt</li>
</ul>
You'll have a cypress\_dump.txt on your home directory containing the kext logs.
Don't hesitate contacting me by the forum (PM or on the cypress topic), by the google issues or even directly by mail. you need to try to explain the bug as much as possible :<br />
<ul>
<li>a link to download your cypress_dump.txt (or put it in attachment)</li>
<li>mention the time(s)/date(s) it occured</li>
<li>its type (hang, jerkyness, freeze or KP) ?</li>
<li>is it at boot ?</li>
<li>is it using a specific feature ?</li>
</ul>

Generally, the few issues comes from a untest yet firmware with some feature, since i have a XPS 13 (cypress firmware v11) i cannot see/detect some bugs without the help of the community.<br />
I do all this just because i like to code, and i like OsX, so I'll try to do the best i can to get it working on most different computers. Again, if you have a problem, don't hesitate, contact me and send me logs.