#import <PreferencePanes/PreferencePanes.h>


@interface VoodooPS2Pref : NSPreferencePane 
{
    IBOutlet NSSlider *speedSliderX;
    IBOutlet NSSlider *speedSliderY;
	IBOutlet NSSlider * maxTapTimeSlider;
	IBOutlet NSSlider * fingerZSlider;
	IBOutlet NSSlider * tedgeSlider;
	IBOutlet NSSlider * bedgeSlider;
	IBOutlet NSSlider * ledgeSlider;
	IBOutlet NSSlider * redgeSlider;
	IBOutlet NSSlider * centerXSlider;
	IBOutlet NSSlider * centerYSlider;
	IBOutlet NSSlider * hscrollSlider;
	IBOutlet NSSlider * vscrollSlider;
	IBOutlet NSSlider * cscrollSlider;
	IBOutlet NSSlider * mhSlider;
	IBOutlet NSSlider * mvSlider;
	IBOutlet NSSlider * mwSlider;
	IBOutlet NSButton * stabTapButton;
	IBOutlet NSButton * hRateButton;
	IBOutlet NSButton * hScrollButton;
	IBOutlet NSButton * vScrollButton;
	IBOutlet NSButton * cScrollButton;
	IBOutlet NSButton * hsScrollButton;
	IBOutlet NSButton * vsScrollButton;
	IBOutlet NSButton * hmScrollButton;
	IBOutlet NSButton * vmScrollButton;
	IBOutlet NSButton * adwButton;
	IBOutlet NSButton * msButton;
    
    // General
    IBOutlet NSTextField *oneFingerRightTapTime_text;
    IBOutlet NSSlider *oneFingerRightTapTime_slide;
    IBOutlet NSButton *EnableOneFingerTapping;
    IBOutlet NSButton *oneFingerDragFiltering;
    IBOutlet NSTextField *oneFingerDragFiltering_Text;
    // Two Fingers
    IBOutlet NSTextField *twoFingersRightTapTime_text;
    IBOutlet NSSlider *twoFingersRightTapTime_slide;
    IBOutlet NSButton *EnabletwoFingersRightTap;
    IBOutlet NSButton *EnabletwoFingersRightTapFiltering;
    IBOutlet NSTextField *twoFingerNoiseLevelLabel;
    IBOutlet NSTextField *twoFingerNoiseLevelText;
    IBOutlet NSButton *EnabletwoFingersHorizScroll;
    IBOutlet NSButton *EnabletwoFingersVertScroll;
    IBOutlet NSSlider *twoFingersVertScroll_slide;
    // Three & Four Fingers
    IBOutlet NSSlider *ThreeFingersDragTime_slide;
    IBOutlet NSButton *EnableThreeFingerDrag;
    IBOutlet NSButton *EnableThreeFingerDragFiltering;
    IBOutlet NSTextField *EnableThreeFingerDragFiltering_Text;
    // Five Fingers
    IBOutlet NSButton *EnableFiveFingersScreenLock;
    IBOutlet NSButton *EnableFiveFingersSleep;
    
    
    IBOutlet NSPopUpButton * cTrigger;
}

- (void) mainViewDidLoad;
- (void) awakeFromNib;
- (void) didUnselect;

- (IBAction) SlideTwoFingersTapAction: (id) sender;
- (IBAction) TextTwoFingersTapAction: (id) sender;
- (IBAction) EnableTwoFingersTapAction: (id) sender;
- (IBAction) EnableOneFingerTapAction: (id) sender;
- (IBAction) SlideOneFingerTapAction: (id) sender;
- (IBAction) TextOneFingerTapAction: (id) sender;
- (IBAction) EnableTwoFingersTapFilteringAction: (id) sender;
- (IBAction) EnableThreeFingersDragAction: (id) sender;
- (IBAction) EnableThreeFingersTapFilteringAction: (id) sender;
- (IBAction) oneFingerTapFilteringAction: (id) sender;
- (IBAction) EnableFiveFingersScreenLockAction: (id) sender;
- (IBAction) EnableFiveFingersSleepAction: (id) sender;
- (IBAction) GenerateInfoPlistParams: (id) sender;

- (IBAction) SlideSpeedXAction: (id) sender;
- (IBAction) SlideSpeedYAction: (id) sender;
- (IBAction) TapAction: (id) sender;

- (IBAction) ButtonHighRateAction: (id) sender;
- (IBAction) SlideFingerZAction: (id) sender;
- (IBAction) SlideTEdgeAction: (id) sender;
- (IBAction) SlideBEdgeAction: (id) sender;
- (IBAction) SlideLEdgeAction: (id) sender;
- (IBAction) SlideREdgeAction: (id) sender;
- (IBAction) SlideCenterXAction: (id) sender;
- (IBAction) SlideCenterYAction: (id) sender;

- (IBAction) HscrollAction: (id) sender;
- (IBAction) VscrollAction: (id) sender;
- (IBAction) CscrollAction: (id) sender;
- (IBAction) MHscrollAction: (id) sender;
- (IBAction) MVscrollAction: (id) sender;
- (IBAction) ADWAction: (id) sender;
- (IBAction) ButtonMStickyAction: (id) sender;
@end
