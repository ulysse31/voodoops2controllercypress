//#import "VoodooPS2Pref.h"
#import "VoodooPS2cypressPane.h"
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOKitKeys.h>




static io_service_t io_service=0;
static CFMutableDictionaryRef dict=0; 



IOReturn sendNumber (const char * key, unsigned int number, io_service_t service)
{
	IOReturn retvalue = kIOReturnError;
	CFStringRef cf_key = CFStringCreateWithCString(kCFAllocatorDefault, key, CFStringGetSystemEncoding());
	CFNumberRef cf_number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &number);
	if (cf_number) 
	{ 
		retvalue = IORegistryEntrySetCFProperty(service, cf_key, cf_number);
		
        CFDictionarySetValue (dict, cf_key, cf_number);
//		CFRelease(cf_number); 
	}
//	if (cf_key) CFRelease(cf_key);
	return retvalue;
}

int getNumber (const char * key, io_service_t io_service)
{
	CFNumberRef num; 
	int tt=0;
	CFStringRef cf_key = CFStringCreateWithCString(kCFAllocatorDefault, key, CFStringGetSystemEncoding());
	if ((num=(CFNumberRef)IORegistryEntryCreateCFProperty(io_service, cf_key, kCFAllocatorDefault, 0)))
	{
		CFDictionarySetValue (dict, cf_key, num);
		CFNumberGetValue (num,kCFNumberIntType,&tt);
//		CFRelease(num);	
	}
//	if (cf_key) CFRelease(cf_key);
	return tt;
}

IOReturn sendLongNumber (const char * key, unsigned long long number, io_service_t service)
{
	IOReturn retvalue = kIOReturnError;
	CFStringRef cf_key = CFStringCreateWithCString(kCFAllocatorDefault, key, CFStringGetSystemEncoding());
	CFNumberRef cf_number = CFNumberCreate(kCFAllocatorDefault, kCFNumberLongLongType, &number);
	if (cf_number) 
	{ 
		CFDictionarySetValue (dict, cf_key, cf_number);
		retvalue = IORegistryEntrySetCFProperty(service, cf_key, cf_number);
//
        
        
        CFRelease(cf_number);
	}
//	if (cf_key) CFRelease(cf_key);
	return retvalue;
}

long long getLongNumber (const char * key, io_service_t io_service)
{
	CFNumberRef num; 
	long long tt=0;
	CFStringRef cf_key = CFStringCreateWithCString(kCFAllocatorDefault, key, CFStringGetSystemEncoding());
	if ((num=(CFNumberRef)IORegistryEntryCreateCFProperty(io_service, cf_key, kCFAllocatorDefault, 0)))
	{
		CFDictionarySetValue (dict, cf_key, num);
		CFNumberGetValue (num,kCFNumberLongLongType,&tt);
//		CFRelease(num);	
	}
//	if (cf_key) CFRelease(cf_key);
	return tt;
}

IOReturn sendBoolean (const char * key, long bl, io_service_t service)
{
	IOReturn retvalue = kIOReturnError;
	CFStringRef cf_key = CFStringCreateWithCString(kCFAllocatorDefault, key, CFStringGetSystemEncoding());
	CFDictionarySetValue (dict, cf_key, bl?kCFBooleanTrue:kCFBooleanFalse);
	retvalue = IORegistryEntrySetCFProperty(service, cf_key, bl?kCFBooleanTrue:kCFBooleanFalse);
   
//	if (cf_key) CFRelease(cf_key);
	return retvalue;
}

int getBoolean (const char * key, io_service_t io_service)
{
	CFBooleanRef num; 
	int tt=0;
	CFStringRef cf_key = CFStringCreateWithCString(kCFAllocatorDefault, key, CFStringGetSystemEncoding());
	if ((num=(CFBooleanRef)IORegistryEntryCreateCFProperty(io_service, cf_key, kCFAllocatorDefault, 0)))
	{
		CFDictionarySetValue (dict, cf_key, num);
		tt=CFBooleanGetValue (num);
//		CFRelease(num);	
	}
//	if (cf_key) CFRelease(cf_key);
	return tt;
}

@implementation VoodooPS2Pref

- (void) GenerateInfoPlistParams
{
    NSString *s = [NSString stringWithFormat: @"<key>CypressTwoFingerRightClick</key>\n" ];
    s = [ s stringByAppendingString:(EnabletwoFingersRightTap.state == NSOnState ? @"<true/>\n" : @"<false/>\n")];
    s = [ s stringByAppendingString:@"<key>CypressThreeFingerDrag</key>\n" ];
    s = [ s stringByAppendingString:(EnableThreeFingerDrag.state == NSOnState ? @"<true/>\n" : @"<false/>\n")];
    s = [ s stringByAppendingString:@"<key>CypressFourFingerHorizSwipeGesture</key>\n" ];
    s = [ s stringByAppendingString:(EnableFourFingerHorizSwipe.state == NSOnState ? @"<true/>\n" : @"<false/>\n")];
    s = [ s stringByAppendingString:@"<key>CypressFourFingerVertSwipeGesture</key>\n" ];
    s = [ s stringByAppendingString:(EnableFourFingerVertSwipe.state == NSOnState ? @"<true/>\n" : @"<false/>\n")];
    s = [ s stringByAppendingString:@"<key>CypressFiveFingerScreenLock</key>\n" ];
    s = [ s stringByAppendingString:(EnableFiveFingersScreenLock.state == NSOnState ? @"<true/>\n" : @"<false/>\n")];
    s = [ s stringByAppendingString:@"<key>CypressFiveFingerSleep</key>\n" ];
    s = [ s stringByAppendingString:(EnableFiveFingersSleep.state == NSOnState ? @"<true/>\n" : @"<false/>\n")];
    s = [ s stringByAppendingString:@"<key>Cypress1FingerMaxTapTime</key>\n" ];
    s = [ s stringByAppendingString:[ NSString stringWithFormat:@"<integer>%d</integer>\n" , oneFingerRightTapTime_slide.intValue ] ];
    s = [ s stringByAppendingString:@"<key>Cypress2FingerMaxTapTime</key>\n" ];
    s = [ s stringByAppendingString:[ NSString stringWithFormat:@"<integer>%d</integer>\n" , twoFingersRightTapTime_text.intValue ] ];
    s = [ s stringByAppendingString:@"<key>Cypress3FingerMaxTapTime</key>\n<integer>200</integer>\n<key>Cypress4FingerMaxTapTime</key>\n<integer>50</integer>\n<key>Cypress5FingerMaxTapTime</key>\n<integer>200</integer>\n" ];
    s = [ s stringByAppendingString:@"<key>CypressDragPressureAverage</key>\n" ];
    s = [ s stringByAppendingString:[ NSString stringWithFormat:@"<integer>%d</integer>\n" , oneFingerDragPressure.intValue ] ];
    s = [ s stringByAppendingString:@"<key>Cypress5FingerScreenLockTimer</key>\n<integer>800</integer>\n<key>Cypress5FingerSleepTimer</key>\n<integer>3000</integer>\n" ];
    s = [ s stringByAppendingString:@"<key>CypressPressureFiltering</key>\n" ];
    s = [ s stringByAppendingString:[ NSString stringWithFormat:@"<integer>%d</integer>\n" , (oneFingerDragFiltering.state == NSOnState ? oneFingerDragFiltering_Text.intValue : 0)] ];
    s = [ s stringByAppendingString:@"<key>Cypress2FingerFiltering</key>\n" ];
    s = [ s stringByAppendingString:[ NSString stringWithFormat:@"<integer>%d</integer>\n" , twoFingerNoiseLevelText.intValue ] ];
    s = [ s stringByAppendingString:@"<key>Cypress3FingerFiltering</key>\n" ];
    s = [ s stringByAppendingString:[ NSString stringWithFormat:@"<integer>%d</integer>\n" , EnableThreeFingerDragFiltering_Text.intValue ] ];
    s = [ s stringByAppendingString:@"<key>Cypress4FingerFiltering</key>\n" ];
    s = [ s stringByAppendingString:@"<integer>20</integer>\n"];
    [ InfoPlistText setString:s];
}

- (void) mainViewDidLoad
{
}

- (void) awakeFromNib
{
	io_service = IOServiceGetMatchingService(0, IOServiceMatching("ApplePS2CypressTouchPad"));
	if (!io_service)
	{
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"ApplePS2CypressTouchPad not found", "MsgBox"), 
								NSLocalizedString( @"Error", "MsgBoxTitle" ), nil, nil, nil );
 		return;
	}	
	dict=CFDictionaryCreateMutable(NULL,0, &kCFTypeDictionaryKeyCallBacks ,NULL);
	
    [ twoFingersRightTapTime_slide setDoubleValue:getNumber("Cypress2FingerMaxTapTime", io_service) ];
    [ twoFingersRightTapTime_text setStringValue:[NSString stringWithFormat:@"%d", getNumber("Cypress2FingerMaxTapTime", io_service)] ];
    [oneFingerRightTapTime_slide setDoubleValue:getNumber("Cypress1FingerMaxTapTime", io_service)];
    [oneFingerRightTapTime_text setStringValue:[NSString stringWithFormat:@"%d", getNumber("Cypress1FingerMaxTapTime", io_service)] ];
    [EnableOneFingerTapping setState:getBoolean("Clicking", io_service) ];

    int tmp;
    tmp = getNumber("CypressFiveFingerScreenLock", io_service);
    [EnableFiveFingersScreenLock setState:(tmp != 0 ? NSOnState : NSOffState) ];
//    tmp = getNumber("CypressFiveFingerSleep", io_service);
//    [EnableFiveFingersSleep setState:(tmp != 0 ? NSOnState : NSOffState) ];
        [EnableFiveFingersSleep setState:getBoolean("CypressFiveFingerSleep", io_service) ];
    tmp = getNumber("Cypress2FingerFiltering", io_service);
    [EnabletwoFingersRightTapFiltering setState:(tmp != 0 ? NSOnState : NSOffState)];
    if (tmp != 0)
        [twoFingerNoiseLevelText setEnabled:YES ];
    else
        [twoFingerNoiseLevelText setEnabled:NO ];
    [twoFingerNoiseLevelText setStringValue:[NSString stringWithFormat:@"%d", tmp] ];
    tmp = getNumber("CypressPressureFiltering", io_service);
    [oneFingerDragFiltering setState:(tmp != 0 ? NSOnState : NSOffState)];
    if (tmp != 0)
        [oneFingerDragFiltering_Text setEnabled:YES ];
    else
        [oneFingerDragFiltering_Text setEnabled:NO ];
    [oneFingerDragFiltering_Text setStringValue:[NSString stringWithFormat:@"%d", tmp] ];
    [oneFingerDragPressure setStringValue:[ NSString stringWithFormat:@"%d", getNumber("CypressDragPressureAverage", io_service)] ];
    [EnabletwoFingersHorizScroll setState:getBoolean("TrackpadHorizScroll", io_service)];
    [EnabletwoFingersVertScroll setState:getBoolean("TrackpadScroll", io_service)];
    tmp = getNumber("CypressThreeFingerDrag", io_service);
    [EnableThreeFingerDrag setState:(tmp != 0 ? NSOnState : NSOffState)];
    tmp = getNumber("Cypress3FingerFiltering", io_service);
    [EnableThreeFingerDragFiltering setState:(tmp != 0 ? NSOnState : NSOffState)];
    if (tmp != 0)
        [EnableThreeFingerDragFiltering_Text setEnabled:YES ];
    else
        [EnableThreeFingerDragFiltering_Text setEnabled:NO ];
    [EnableThreeFingerDragFiltering_Text setStringValue:[NSString stringWithFormat:@"%d", tmp] ];

    [self GenerateInfoPlistParams];
}

- (void)didUnselect
{
	FILE *f;
	CFDataRef dat1;
	UInt8 *dat2;
	
	if (!dict)
		return;
	dat1=CFPropertyListCreateXMLData (kCFAllocatorDefault, dict);
	if (!dat1)
	{
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Couldn't create XML", "MsgBox"),
								NSLocalizedString( @"Error creating XML data", "MsgBoxBody" ), nil, nil, nil );		
		return;
	}
	
	dat2=(UInt8 *) malloc (CFDataGetLength (dat1));
	CFDataGetBytes (dat1, CFRangeMake(0,CFDataGetLength(dat1)), dat2);
	if (!dat2)
	{
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Couldn't alocate memory ", "MsgBox"), 
								NSLocalizedString( @"Error allocating memory", "MsgBoxBody" ), nil, nil, nil );		
		return;
	}
	
    
      //f=fopen ([[NSHomeDirectory() stringByAppendingString:[NSString stringWithCString: "/Library/Preferences/org.voodoo.CypressTouchpad.plist"]] UTF8String], "wb");
    f=fopen ([[NSHomeDirectory() stringByAppendingString:[NSString stringWithCString: "/Library/Preferences/org.voodoo.CypressTouchpad.plist" encoding:NSASCIIStringEncoding]] UTF8String], "wb");
	//NSString *fName = nil;
	//f=fopen ([[fName  stringByAppendingString: @"/Library/Preferences/org.voodoo.CypressTouchpad.plist"] UTF8String], "wb");
    //f=authopen ([[fName  stringByAppendingString: @"/Library/Preferences/org.voodoo.CypressTouchpad.plist"] UTF8String], "wb");
	
    if (!f)
	{
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Couldn't save plist", "MsgBox"), 
								NSLocalizedString( @"Error opening file", "MsgBoxBody" ), nil, nil, nil );		
		return;
	}
	fwrite(dat2, 1, CFDataGetLength (dat1), f);
	fclose (f);
	return ;
}

- (IBAction) EnableFiveFingersScreenLockAction: (id) sender
{
    sendBoolean("CypressFiveFingerScreenLock", EnableFiveFingersScreenLock.state, io_service);
    [self GenerateInfoPlistParams];
}

- (IBAction) EnableFiveFingersSleepAction: (id) sender
{
    sendBoolean("CypressFiveFingerSleep", EnableFiveFingersSleep.state, io_service);
    [self GenerateInfoPlistParams];
}

- (IBAction) EnableThreeFingersDragAction: (id) sender
{
    sendBoolean("CypressThreeFingerDrag", EnableThreeFingerDrag.state, io_service);
    [self GenerateInfoPlistParams];
}

- (IBAction) EnableFourFingersVertSwipes: (id) sender
{
    sendBoolean("CypressFourFingerVertSwipeGesture", EnableFourFingerVertSwipe.state, io_service);
    [self GenerateInfoPlistParams];
}

- (IBAction) EnableFourFingersHorizSwipes: (id) sender
{
    sendBoolean("CypressFourFingerHorizSwipeGesture", EnableFourFingerHorizSwipe.state, io_service);
    [self GenerateInfoPlistParams];
}


- (IBAction) EnableThreeFingersTapFilteringAction: (id) sender
{
    sendNumber("Cypress3FingerFiltering", ( EnableThreeFingerDragFiltering.state == NSOnState ?  [EnableThreeFingerDragFiltering_Text intValue] : 0x0 ), io_service);
    if (EnableThreeFingerDragFiltering.state == NSOnState)
        [EnableThreeFingerDragFiltering_Text setEnabled:YES ];
    else
        [EnableThreeFingerDragFiltering_Text setEnabled:NO ];
    [self GenerateInfoPlistParams];
}

- (IBAction) EnableTwoFingersTapFilteringAction: (id) sender
{
    sendNumber("Cypress2FingerFiltering", ( EnabletwoFingersRightTapFiltering.state == NSOnState ?  [twoFingerNoiseLevelText intValue] : 0x0 ), io_service);
    if (EnabletwoFingersRightTapFiltering.state == NSOnState)
        [twoFingerNoiseLevelText setEnabled:YES ];
    else
        [twoFingerNoiseLevelText setEnabled:NO ];
    [self GenerateInfoPlistParams];
}

- (IBAction) oneFingerTapFilteringAction: (id) sender
{
    sendNumber("CypressPressureFiltering", ( oneFingerDragFiltering.state == NSOnState ?  [oneFingerDragFiltering_Text intValue] : 0x0 ), io_service);
    if (oneFingerDragFiltering.state == NSOnState)
        [oneFingerDragFiltering_Text setEnabled:YES ];
    else
        [oneFingerDragFiltering_Text setEnabled:NO ];
    [self GenerateInfoPlistParams];
}

- (IBAction) oneFingerDragPressureAction: (id) sender
{
    sendNumber("CypressDragPressureAverage", oneFingerDragPressure.intValue, io_service);
    [self GenerateInfoPlistParams];
}


- (IBAction) EnableTwoFingersTapAction: (id) sender
{
    sendBoolean("CypressTwoFingerRightClick", EnabletwoFingersRightTap.state, io_service);
    [self GenerateInfoPlistParams];
}

- (IBAction) EnableOneFingerTapAction: (id) sender
{
    sendNumber("Clicking", (EnableOneFingerTapping.state == NSOnState ? 0x1 : 0x0 ), io_service);
    [self GenerateInfoPlistParams];
}

- (IBAction) SlideOneFingerTapAction: (id) sender
{
    int tmp = [oneFingerRightTapTime_slide intValue ];
    oneFingerRightTapTime_text.stringValue = [NSString stringWithFormat:@"%d", tmp];
    sendNumber("Cypress1FingerMaxTapTime", tmp, io_service);
    [self GenerateInfoPlistParams];
}

- (IBAction) TextOneFingerTapAction: (id) sender
{
    int tmp = (int)[ oneFingerRightTapTime_text doubleValue ];
    oneFingerRightTapTime_slide.intValue = tmp;
    sendNumber("Cypress1FingerMaxTapTime", tmp, io_service);
    [self GenerateInfoPlistParams];
}


- (IBAction) SlideTwoFingersTapAction: (id) sender
{
    int tmp = [twoFingersRightTapTime_slide intValue ];
    twoFingersRightTapTime_text.stringValue = [NSString stringWithFormat:@"%d", tmp];
    sendNumber("Cypress2FingerMaxTapTime", tmp, io_service);
    [self GenerateInfoPlistParams];
}

- (IBAction) TextTwoFingersTapAction: (id) sender
{
    int tmp = (int)[ twoFingersRightTapTime_text doubleValue ];
    twoFingersRightTapTime_slide.intValue = tmp;
    sendNumber("Cypress2FingerMaxTapTime", tmp, io_service);
    [self GenerateInfoPlistParams];
}



//
// DEPRECATED
- (IBAction) SlideSpeedXAction: (id) sender
{
	sendNumber("DivisorX", 101-[speedSliderX doubleValue], io_service);
}

- (IBAction) SlideSpeedYAction: (id) sender
{
	sendNumber("DivisorY", 101-[speedSliderY doubleValue], io_service);
}

- (IBAction) ButtonHighRateAction: (id) sender
{
	sendBoolean("ExtendedWmode", [hRateButton state], io_service);
}

- (IBAction) SlideFingerZAction: (id) sender
{
	sendNumber("FingerZ", [fingerZSlider doubleValue], io_service);
}

- (IBAction) SlideTEdgeAction: (id) sender
{
	sendNumber("TopEdge", [tedgeSlider doubleValue]*70, io_service);
}
- (IBAction) SlideBEdgeAction: (id) sender
{
	sendNumber("BottomEdge", [bedgeSlider doubleValue]*70, io_service);
}
- (IBAction) SlideLEdgeAction: (id) sender
{
	sendNumber("LeftEdge", [ledgeSlider doubleValue]*70, io_service);
}

- (IBAction) SlideREdgeAction: (id) sender
{
	sendNumber("RightEdge", [redgeSlider doubleValue]*70, io_service);
}

- (IBAction) SlideCenterXAction: (id) sender
{
	sendNumber("CenterX", [centerXSlider doubleValue]*70, io_service);
}

- (IBAction) SlideCenterYAction: (id) sender
{
	sendNumber("CenterY", [centerYSlider doubleValue]*70, io_service);
}

- (IBAction) TapAction: (id) sender
{
	sendLongNumber("MaxTapTime", [maxTapTimeSlider doubleValue]*2500000.0,io_service);
	sendBoolean("StabilizeTapping", [stabTapButton state], io_service);
}

- (IBAction) HscrollAction: (id) sender
{
	if ([hScrollButton state])
	{
		[hscrollSlider setEnabled:1];
		[hsScrollButton setEnabled:1];
		sendNumber("HorizontalScrollDivisor", 101-[hscrollSlider doubleValue],io_service);
		sendBoolean("StickyHorizontalScrolling", [hsScrollButton state], io_service);
	}
	else
	{
		[hscrollSlider setEnabled:0];
		[hsScrollButton setEnabled:0];
		sendNumber("HorizontalScrollDivisor", 0, io_service);
	}		
}

- (IBAction) CscrollAction: (id) sender
{
	if ([cScrollButton state])
	{
		[cscrollSlider setEnabled:1];
		[cTrigger setEnabled:1];
		sendNumber("CircularScrollDivisor", 101-[hscrollSlider doubleValue],io_service);
		sendNumber("CircularScrollTrigger", (int)[cTrigger indexOfSelectedItem]+1, io_service);
	}
	else
	{
		[cscrollSlider setEnabled:0];
		[cTrigger setEnabled:0];
		sendNumber("CircularScrollDivisor", 0, io_service);
	}		
}

- (IBAction) VscrollAction: (id) sender
{
	if ([vScrollButton state])
	{
		[vscrollSlider setEnabled:1];
		[vsScrollButton setEnabled:1];
		sendNumber("VerticalScrollDivisor", 101-[vscrollSlider doubleValue],io_service);
		sendBoolean("StickyVerticalScrolling", [vsScrollButton state], io_service);
	}
	else
	{
		[vscrollSlider setEnabled:0];
		[vsScrollButton setEnabled:0];
		sendNumber("VerticalScrollDivisor", 0, io_service);
	}		
}

- (IBAction) MHscrollAction: (id) sender
{
	if ([hmScrollButton state])
	{
		[mhSlider setEnabled:1];
		sendNumber("MultiFingerHorizontalDivisor", 101-[mhSlider doubleValue],io_service);
	}
	else
	{
		[mhSlider setEnabled:0];
		sendNumber("MultiFingerHorizontalDivisor", 0, io_service);
	}		
}

- (IBAction) MVscrollAction: (id) sender
{
	if ([vmScrollButton state])
	{
		[mvSlider setEnabled:1];
		sendNumber("MultiFingerVerticalDivisor", 101-[mvSlider doubleValue],io_service);
	}
	else
	{
		[mvSlider setEnabled:0];
		sendNumber("MultiFingerVerticalDivisor", 0, io_service);
	}		
}

- (IBAction) ADWAction: (id) sender
{
	if ([adwButton state])
	{
		[mwSlider setEnabled:1];
		sendNumber("MultiFingerWLimit", 4+12*[mwSlider doubleValue]/100,
				   io_service);
	}
	else
	{
		[mwSlider setEnabled:0];
		sendNumber("MultiFingerWLimit", 17, io_service);
	}		
}


- (IBAction) ButtonMStickyAction: (id) sender
{
	sendBoolean("StickyMultiFingerScrolling", [msButton state], io_service);
}

@end
