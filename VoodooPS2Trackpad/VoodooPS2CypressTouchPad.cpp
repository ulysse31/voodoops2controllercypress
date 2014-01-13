/*
 * Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.2 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 *
 * =====================================================================
 * This file as been created/added by Ulysse31, in order to implement the
 * Cypress PS2 Trackpad protocol/support.
 * Some basic functions as been ported/adapted from linux, but the processing 
 * could not be done the same way ...
 * Thanks to Dudley Du from cypress for his short, but enought doc sheet
 * Ulysse31 aka Nix
 * ulysse31<at>gmail<dot>com
 */


#define UNDOCUMENTED_INIT_SEQUENCE_POST

#include <IOKit/IOLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include "VoodooPS2Controller.h"
#include "VoodooPS2CypressTouchPad.h"

enum {
    //
    //
    kTapEnabled  = 0x01
};

// =============================================================================
// kalFilter: kalman filtering for mouse coordinates (three fingers smoothing)
//

kalFilter::kalFilter(int noiseLevel)
{
  _it = 0;
  _xk[KFLAST] = 0;
  _xk[KFCURRENT] = 0;
  _pk[KFLAST] = 0;
  _pk[KFCURRENT] = 0;
  _k = 0;
  _zk = 0;
  _r = noiseLevel;
  _r /= 1000.0f;
}

int
kalFilter::getNewValue(int rawval)
{
  int   result;


  if (_it == 0)
    {
      _xk[KFLAST] = 0;
      _pk[KFLAST] = 1;
    }
  // TIME UPDATE
  _xk[KFCURRENT] = _xk[KFLAST];
  _pk[KFCURRENT] = _pk[KFLAST];
  _zk = rawval;
  _zk /= 1000.0f; // ratio to 0.001
  // MEASUREMENT UPDATE
  _k = (float)((float)_pk[KFLAST] / (float)(_pk[KFLAST] + _r));
  _xk[KFCURRENT] = _xk[KFLAST] + (_k * (_zk - _xk[KFLAST]));
  _pk[KFCURRENT] = _pk[KFLAST]*(1 - _k);
  // PREPARE NEXT ROUND
  result = (int)KFCEIL((float)(_xk[KFCURRENT] * 1000.0f));
  _xk[KFLAST] = _xk[KFCURRENT];
  _pk[KFLAST] = _pk[KFCURRENT];
  _it++;
  return (result);
}

void
kalFilter::resetFilter()
{
  _it = 0;
  _xk[KFLAST] = 0;
  _xk[KFCURRENT] = 0;
  _pk[KFLAST] = 0;
  _pk[KFCURRENT] = 0;
  _k = 0;
  _zk = 0;
}

int
kalFilter::noiseLevel(int noiseLevel)
{
  if (noiseLevel)
    {
      _r = noiseLevel;
      _r = (float)( _r / 1000.0f);
      DEBUG_LOG("CYPRESS: kalFilter: set noise level to %lu\n", (unsigned long)(_r * 1000.0f));
    }
  return ((int)(_r * 1000.0f));
}

// =============================================================================
// cypressFrame: coord analysis Class implementation
//

#define MAX_REPORTS	4
#define abs(x) ((x) < 0 ? -(x) : (x))

cypressFrame::cypressFrame(int max)
{
  _maxElems = max;
  _list = 0;
  _end = 0;
  _elems = 0;
}


void		cypressFrame::addReport(t_reportData *p)
{
  //  int		k;
  //  t_reportList	*l;

  if (_elems == 0)
    {
      
    }
}

// =============================================================================
// ApplePS2CypressTouchPad Class Implementation
//

OSDefineMetaClassAndStructors(ApplePS2CypressTouchPad, IOHIPointing);

UInt32 ApplePS2CypressTouchPad::deviceType()
{ return NX_EVS_DEVICE_TYPE_MOUSE; };

UInt32 ApplePS2CypressTouchPad::interfaceID()
{ return NX_EVS_DEVICE_INTERFACE_BUS_ACE; };

IOItemCount ApplePS2CypressTouchPad::buttonCount() { return 2; };
IOFixed     ApplePS2CypressTouchPad::resolution()  { return _resolution; };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool ApplePS2CypressTouchPad::init(OSDictionary * dict)
{
    //
    // Initialize this object's minimal state. This is invoked right after this
    // object is instantiated.
    //
    
    if (!super::init(dict))
        return false;

    // find config specific to Platform Profile
    OSDictionary* list = OSDynamicCast(OSDictionary, dict->getObject(kPlatformProfile));
    OSDictionary* config = ApplePS2Controller::makeConfigurationNode(list);
    if (config)
    {
        // if DisableDevice is Yes, then do not load at all...
        OSBoolean* disable = OSDynamicCast(OSBoolean, config->getObject(kDisableDevice));
        if (disable && disable->isTrue())
        {
            config->release();
            return false;
        }
#ifdef DEBUG
        // save configuration for later/diagnostics...
        setProperty(kMergedConfiguration, config);
#endif
    }
    
    // initialize state/default settings...
    _device			= 0;
    _interruptHandlerInstalled	= false;
    _packetByteCount		= 0;
    _resolution			= (100) << 16; // (100 dpi, 4 counts/mm)
    _xpos			= -1;
    _ypos			= -1;
    _x4pos			= -1;
    _y4pos			= -1;
    _xscrollpos			= -1;
    _yscrollpos			= -1;
    _pendingButtons		= 0;
    _frameCounter		= 0;
    _frameType			= -1;
    _framePressure		= 0;
    _swipey			= 0;
    _swipex			= 0;
    _swiped			= false;
    _tapFrameMax		= 5;
    _lockFrameMin		= 40;
    _swipexThreshold		= 10;
    _swipeyThreshold		= 10;
    _twoFingersMaxCount		= 2;
    _threeFingersMaxCount	= 2;
    _fourFingersMaxCount	= 3;
    _onefingervdivider		= 2;
    _onefingerhdivider		= 2;
    _twoFingerDivider		= 65536;
    _twofingervdivider		= 2;
    _twofingerhdivider		= 2;
    _threefingervdivider	= 1;
    _threefingerhdivider	= 1;
    _onefingermaxtaptime	= 200000000;
    _twofingermaxtaptime	= 200000000;
    _threefingermaxtaptime	= 200000000;
    _fourfingermaxtaptime	= 50000000;
    _fivefingermaxtaptime	= 200000000;
    _fivefingerscreenlocktime	= 800000000;
    _fivefingersleeptime	= 3000000000;
    _dragPressureAverage	= 85;
//     _onefingervdivider		= 1;
//     _onefingerhdivider		= 1;
//     _twofingervdivider		= 1;
//     _twofingerhdivider		= 1;
//     _threefingervdivider	= 1;
//     _threefingerhdivider	= 1;
    _activeDragLock		= false;
    _lastOneTap			= 0;
    _oneTapCounter		= 0;

    _frameTimer			= 0;
    _slept			= false;

    _cytp_resolution[0] =  0x00;
    _cytp_resolution[1] =  0x01;
    _cytp_resolution[2] =  0x02;
    _cytp_resolution[3] =  0x03;
    _cytp_rate[0] = 10;
    _cytp_rate[1] = 20;
    _cytp_rate[2] = 40;
    _cytp_rate[3] = 60;
    _cytp_rate[4] = 100;
    _cytp_rate[5] = 200;

    _wakeDelay = 350;

    // Default Props
    _clicking				= true;
    _twoFingerRightClick		= true;
    _threeFingerDrag			= true;
    _fourFingerHorizSwipeGesture	= true;
    _fourFingerVertSwipeGesture		= true;
    _fiveFingerScreenLock		= true;
    _fiveFingerSleep			= true;
    _pressureFiltering			= 20;
    _twoFingerFiltering			= 1;
    _threeFingerFiltering		= 20;
    _fourFingerFiltering		= 20;

    _packetLength = 8; // default len
    _tpMode = 0;
    _tpWidth = CYTP_DEFAULT_WIDTH;
    _tpHigh = CYTP_DEFAULT_HIGH;
    _tpMaxAbsX = CYTP_ABS_MAX_X;
    _tpMaxAbsY = CYTP_ABS_MAX_Y;
    _tpMinPressure = CYTP_MIN_PRESSURE;
    _tpMaxPressure = CYTP_MAX_PRESSURE;
    _tpResX = _tpMaxAbsY / _tpWidth;
    _tpResY = _tpMaxAbsY / _tpHigh;

    this->setParamProperties(config);
    OSSafeRelease(config);

    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ApplePS2CypressTouchPad* ApplePS2CypressTouchPad::probe( IOService * provider, SInt32 * score )
{
    DEBUG_LOG("ApplePS2CypressTouchPad::probe entered...\n");
    //
    // The driver has been instructed to verify the presence of the actual
    // hardware we represent. We are guaranteed by the controller that the
    // mouse clock is enabled and the mouse itself is disabled (thus it
    // won't send any asynchronous mouse data that may mess up the
    // responses expected by the commands we send it).
    //

    bool                  success = false;
    
    if (!super::probe(provider, score))
        return 0;

    _device = (ApplePS2MouseDevice *) provider;

    cypressReset();
    success = cypressReadFwVersion();
    if (success &&_touchPadVersion > 11)
      {
	_tapFrameMax = 11;
	_lockFrameMin = 80; // sync packet rate is superior on > 11 firmware ... should be good ... (not tested) ...
      }
    _device = 0;
    DEBUG_LOG("CYPRESS: ApplePS2CypressTouchPad::probe leaving.\n");

    return (success) ? this : 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool ApplePS2CypressTouchPad::start( IOService * provider )
{
    UInt64 enabledProperty;
    UInt64 disabledProperty;

    //
    // The driver has been instructed to start. This is called after a
    // successful probe and match.
    //

    if (!super::start(provider))
        return false;

    //
    // Maintain a pointer to and retain the provider object.
    //

    _device = (ApplePS2MouseDevice *) provider;
    _device->retain();

    //
    // Announce hardware properties.
    //

    IOLog("CYPRESS: ApplePS2Trackpad: Cypress Trackpad firmware v%d\n", (UInt8)(_touchPadVersion));

    //
    // Advertise some supported features (tapping, edge scrolling).
    //


    enabledProperty = 1;
    disabledProperty = 0; 

    setProperty("Clicking", enabledProperty, 
       sizeof(enabledProperty) * 8);
    setProperty("DragLock", disabledProperty, 
        sizeof(disabledProperty) * 8);
    setProperty("Dragging", disabledProperty, 
       sizeof(disabledProperty) * 8);
    setProperty("TrackpadScroll", enabledProperty, 
       sizeof(enabledProperty) * 8);
    setProperty("TrackpadHorizScroll", enabledProperty, 
       sizeof(enabledProperty) * 8);

    setProperty("CypressFourFingerHorizSwipeGesture", disabledProperty, 
		sizeof(disabledProperty) * 8);
    setProperty("CypressFourFingerVertSwipeGesture", disabledProperty, 
        sizeof(disabledProperty) * 8);
    setProperty("CypressFiveFingerScreenLock", disabledProperty, 
        sizeof(disabledProperty) * 8);
    setProperty("CypressFiveFingerSleep", disabledProperty, 
        sizeof(disabledProperty) * 8);

    setProperty("CypressThreeFingerDrag", disabledProperty, 
        sizeof(disabledProperty) * 8);
//        float	i = 200;
//     setProperty("1FingersMaxTapTime", i, sizeof(i) * 8);
//     setProperty("2FingersMaxTapTime", i, sizeof(i) * 8);
//     setProperty("3FingersMaxTapTime", i, sizeof(i) * 8);

    //
    // Must add this property to let our superclass know that it should handle
    // trackpad acceleration settings from user space.  Without this, tracking
    // speed adjustments from the mouse prefs panel have no effect.
    //

    setProperty(kIOHIDPointerAccelerationTypeKey, kIOHIDTrackpadAccelerationType);
    setProperty(kIOHIDScrollAccelerationTypeKey, kIOHIDTrackpadScrollAccelerationKey);

    //
    // Lock the controller during initialization
    //
    
    _device->lock();
    if (_touchPadVersion > 11)
      _tapFrameMax = 11;
    setTouchpadModeByte();
    _device->installInterruptAction(this,
                                    OSMemberFunctionCast(PS2InterruptAction, this, &ApplePS2CypressTouchPad::interruptOccurred),
                                    OSMemberFunctionCast(PS2PacketAction, this, &ApplePS2CypressTouchPad::packetReady));
    _interruptHandlerInstalled = true;
    
    // now safe to allow other threads
    _device->unlock();
    
    //
	// Install our power control handler.
	//

	_device->installPowerControlAction( this, OSMemberFunctionCast(PS2PowerControlAction,this,
             &ApplePS2CypressTouchPad::setDevicePowerState) );
	_powerControlHandlerInstalled = true;

    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ApplePS2CypressTouchPad::stop( IOService * provider )
{
    //
    // The driver has been instructed to stop.  Note that we must break all
    // connections to other service objects now (ie. no registered actions,
    // no pointers and retains to objects, etc), if any.
    //

    assert(_device == provider);

    //
    // Disable the mouse itself, so that it may stop reporting mouse events.
    //

    setTouchPadEnable(false);

    //
    // Uninstall the interrupt handler.
    //

    if ( _interruptHandlerInstalled )  _device->uninstallInterruptAction();
    _interruptHandlerInstalled = false;

    //
    // Uninstall the power control handler.
    //

    if ( _powerControlHandlerInstalled ) _device->uninstallPowerControlAction();
    _powerControlHandlerInstalled = false;

    //
    // Release the pointer to the provider object.
    //
    
    OSSafeReleaseNULL(_device);
    
	super::stop(provider);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


PS2InterruptResult ApplePS2CypressTouchPad::interruptOccurred(UInt8 data)
{
  int index = _packetByteCount;
  UInt8* packet = _ringBuffer.head();

  packet[_packetByteCount++] = data;
  if (index == 0)
    this->updatePacketSize(packet[0]);
  //DEBUG_LOG("CYPRESS: interruptOccured: rec 0x%02x byte number %d of %d (queued %d)\n", data, _packetByteCount - 1, this->packetSize(), _ringBuffer.count());
  if (this->packetSize() == _packetByteCount)
    {
      if (this->packetSize() == kPacketLengthSmall)
	{
	  packet[_packetByteCount++] = 0;
	  packet[_packetByteCount++] = 0;
	  packet[_packetByteCount++] = 0;
	}
      _ringBuffer.advanceHead(kPacketLengthLarge);
      _packetByteCount = 0;
      return kPS2IR_packetReady;
    }
  return kPS2IR_packetBuffering;
}

void	ApplePS2CypressTouchPad::cypressSimulateEvent(char button)
{
  uint64_t	now_abs;

  clock_get_uptime(&now_abs);
  dispatchRelativePointerEventX(0, 0, button, now_abs);
  if (_activeDragLock)
    return ;
  clock_get_uptime(&now_abs);
  dispatchRelativePointerEventX(0, 0, 0x00, now_abs);
}

void	ApplePS2CypressTouchPad::cypressSimulateLastEvents()
{
  uint64_t	now_abs;

  clock_get_uptime(&now_abs);
#ifdef DEBUG
  if (_frameType >= 0)
    DEBUG_LOG("CYPRESS: _frameType = %d, _frameCounter = %d, _frameTimer %llu, diff %llu\n", _frameType, _frameCounter, _frameTimer, now_abs - _frameTimer);
#endif
  if (_clicking && _frameType == 1 && _frameCounter > 0 && ((now_abs - _frameTimer) < _onefingermaxtaptime)) // 200 ms, all value less than that should be considered as a tap
    {
      // simulate a tap here
      if ((now_abs - _lastOneTap) < 250000000) // 250ms for triple tap drag locking
	_oneTapCounter++;
      else
	{
	  _oneTapCounter = 0;
	  _activeDragLock = false;
	}
      if (_dragLock && _oneTapCounter >= 2)
	_activeDragLock = true;
      this->cypressSimulateEvent(0x01);
      DEBUG_LOG("CYPRESS: one finger tap detected (_oneTapCounter = %d, _lastOneTap = %llu, diff = %llu)\n", _oneTapCounter, _lastOneTap, now_abs - _lastOneTap);
      _lastOneTap = now_abs;
    }
  if (_twoFingerRightClick && _frameType == 2 && _frameCounter > 0 && ((now_abs - _frameTimer) < _twofingermaxtaptime)) // 200 ms, all value less than that should be considered as a tap
    {
      // simulate a tap right click here
      this->cypressSimulateEvent(0x02);
      DEBUG_LOG("CYPRESS: two fingers tap detected\n");
    }
  if (_frameType > 1)
    {
      _kalX.resetFilter();
      _kalY.resetFilter();
    }
  _kalZ.resetFilter();
  if (_frameType == 3 && _frameCounter > 0 && ((now_abs - _frameTimer) < _threefingermaxtaptime)) // 200 ms, all value less than that should be considered as a tap
    {
      // simulate a tap here
      this->cypressSimulateEvent(0x01);
      DEBUG_LOG("CYPRESS: three fingers tap detected\n");
    }
  // Finally thinking that it may not be a good idea to implement a third action on 5 fingers ... may be confusing use between screenlock and show desktop ...
  // More generally, adding more than 2 time based actions on the same finger count may be a bad idea ... so commenting out this code
//   if (_fiveFingerShowDesktop && _frameType == 5 && (now_abs - _frameTimer) < _fivefingermaxtaptime) // 100ms
//     {
//       // Show Desktop here
//       _device->dispatchKeyboardMessage(kPS2M_showDesktop, &now_abs);
//       DEBUG_LOG("CYPRESS: 5 fingers short frame (tap) detected (size=%d), Showing Desktop\n", _frameCounter);
//     }
  if (_fiveFingerScreenLock && _frameType == 5 && (now_abs - _frameTimer) > _fivefingerscreenlocktime && _slept == false) // 800ms, a bit less than 1 sec
    {
      // Lock Screen here
      _device->dispatchKeyboardMessage(kPS2M_screenLock, &now_abs);
      DEBUG_LOG("CYPRESS: 5 fingers long frame detected (size=%d), locking screen\n", _frameCounter);
    }
}

void	ApplePS2CypressTouchPad::cypressResetCounters()
{
  _xpos = -1;
  _ypos = -1;
  _x4pos = -1;
  _y4pos = -1;
  _xscrollpos = -1;
  _yscrollpos = -1;
  _swipey = 0;
  _swipex = 0;
  _swiped = false;
  _slept = false;
  _frameType = -1;
  _frameCounter = 0;
  _frameTimer = 0;
  _framePressure = 0;
}

bool	ApplePS2CypressTouchPad::cypressCheckPacketEndFrame(UInt8 *packet)
{
  if (packet[0] == 0x00 && packet[1] == 0x00 && packet[2] == 0x00 && packet[3] == 0x00
      && packet[4] == 0x00 && packet[5] == 0x00 && packet[6] == 0x00 && packet[7] == 0x00)
    {
      // Empty packet, received lots (500 bytes) when an action ends (finger leave)
      _ringBuffer.advanceTail(kPacketLengthLarge);
      this->cypressSimulateLastEvents();
      this->cypressResetCounters();
      if (_pendingButtons &&  packet[0] == 0 && packet[1] == 0 && packet[2] == 0 && packet[3] == 0
	  && packet[4] == 0 && packet[5] == 0 && packet[6] == 0 && packet[7] == 0) // buttons were used need to clear events
	{
	  this->cypressProcessPacket(packet);
	  _pendingButtons = 0;
	}
      return true;
    }
  return false;
}


bool		ApplePS2CypressTouchPad::cypressCheckPacketValidity(UInt8 *packet)
{
  UInt8	header;
  UInt8	fingers;

  fingers = fingersCount(packet[0]);
  header = packet[0];
  if (((((header & 0x40) != 0x40) && ((header & 0x80) != 0x80) && ((header & 0x20) != 0x20) && (header > 2))) || ((header & 0xe0) == 0xe0 || (header & 0xf0) == 0xf0) // palm detect packet
      || ((header & 0x08) == 0x08 && fingers < 0) || ((header & 0x01) == 0x01 && (header & 0x02) == 0x02) || ((fingers == 0 || fingers > 2) && ((header & 0x03) == 0x03)) // trying to match de-sync with tap bit
      || (packet[0] == 0 && (packet[1] != 0x00 || packet[2] != 0x00 || packet[3] != 0x00 || packet[4] != 0x00 || packet[5] != 0x00 || packet[6] != 0x00 || packet[7] != 0x00) ))
    {
      // incomplete packet : invalid header, de-sync'ed packets (start and ends in middle of 2 stored packets), etc ... => drop queue and ask re-send
      // Thanks to dudley at cypress for is (short but enought) doc sheet
      this->cypressResetCounters();
      _packetByteCount = 0;
      _pendingButtons = 0;
      _ringBuffer.advanceTail(_ringBuffer.count());
      if (!((header & 0xe0) == 0xe0 || (header & 0xf0) == 0xf0)) // only ask resend if packet is not palm detect
	cypressSendByte(0xFE);
      return (false);
    }
  return (true);
}

void		ApplePS2CypressTouchPad::packetReady()
{
  while (_ringBuffer.count() >= kPacketLengthLarge)
    {
      UInt8 *packet = _ringBuffer.tail();
      this->updatePacketSize(packet[0]);
      UInt8 size = packetSize();
      if (_ringBuffer.count() < size)
	return ;
      // should be deleted once communication with PS2 cypress trackpad stable
      // v34 and later seems to send 0x04 header with 4 fingers touch (??), so lets make a little trick ...
      if (_touchPadVersion >= 34 && (packet[0] == 0x04))
	    packet[0] = 0x20; // 4 fingers should be 0x20 ...
      if (_touchPadVersion >= 34 && (packet[0] == 0x44))
	    packet[0] = 0xa0; // 5 fingers should be 0xa0 ...
#ifdef DEBUG
      if (packet[0] || packet[1] || packet[2] || packet[3] || packet[4] || packet[5] || packet[6] || packet[7])
	DEBUG_LOG("CYPRESS: %s: packet dump { 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x }\n", getName(), packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6], packet[7]);
#endif
      if (this->cypressCheckPacketEndFrame(packet))
	continue ;
      if (this->cypressCheckPacketValidity(packet) == false)
	return ;
      this->cypressProcessPacket(packet);
      _ringBuffer.advanceTail(kPacketLengthLarge);
    }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void		ApplePS2CypressTouchPad::setTouchPadEnable( bool enable )
{
    //
    // Instructs the trackpad to start or stop the reporting of data packets.
    // It is safe to issue this request from the interrupt/completion context.
    //

    // (mouse enable/disable command)                                                                                                                                                                                                                                   
  cypressSendByte(enable ? kDP_Enable : kDP_SetDefaultsAndDisable);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

IOReturn	ApplePS2CypressTouchPad::setParamProperties( OSDictionary * dict )
{
  if (dict == 0)
    return super::setParamProperties(dict);

  OSNumber * clicking = OSDynamicCast( OSNumber, dict->getObject("Clicking") );
  OSNumber * dragging = OSDynamicCast( OSNumber, dict->getObject("Dragging") );
  OSNumber * draglock = OSDynamicCast( OSNumber, dict->getObject("DragLock") );
  OSNumber * hscroll  = OSDynamicCast( OSNumber, dict->getObject("TrackpadHorizScroll") );
  OSNumber * vscroll  = OSDynamicCast( OSNumber, dict->getObject("TrackpadScroll") );
  OSNumber * scrollspeed = OSDynamicCast( OSNumber, dict->getObject("HIDTrackpadScrollAcceleration") );

  OSNumber * onefingermaxtaptime = OSDynamicCast( OSNumber, dict->getObject("Cypress1FingerMaxTapTime") );
  OSNumber * twofingermaxtaptime = OSDynamicCast( OSNumber, dict->getObject("Cypress2FingerMaxTapTime") );
  OSNumber * threefingermaxtaptime = OSDynamicCast( OSNumber, dict->getObject("Cypress3FingerMaxTapTime") );
  OSNumber * fourfingermaxtaptime = OSDynamicCast( OSNumber, dict->getObject("Cypress4FingerMaxTapTime") );
  OSNumber * fivefingermaxtaptime = OSDynamicCast( OSNumber, dict->getObject("Cypress5FingerMaxTapTime") );
  OSNumber * fourFingerHorizSwipeGesture = OSDynamicCast( OSNumber, dict->getObject("CypressFourFingerHorizSwipeGesture") );
  OSNumber * fourFingerVertSwipeGesture = OSDynamicCast( OSNumber, dict->getObject("CypressFourFingerVertSwipeGesture") );
  OSNumber * twoFingerRightClick = OSDynamicCast( OSNumber, dict->getObject("CypressTwoFingerRightClick") );
  OSNumber * threeFingerDrag = OSDynamicCast( OSNumber, dict->getObject("CypressThreeFingerDrag"));
  OSNumber * dragPressureAverage = OSDynamicCast( OSNumber, dict->getObject("CypressDragPressureAverage"));
  OSNumber * fiveFingerSleep = OSDynamicCast( OSNumber, dict->getObject("CypressFiveFingerSleep"));
  OSNumber * fiveFingerScreenLock = OSDynamicCast( OSNumber, dict->getObject("CypressFiveFingerScreenLock"));
  OSNumber * fiveFingerSleepTimer = OSDynamicCast( OSNumber, dict->getObject("Cypress5FingerSleepTimer"));
  OSNumber * fiveFingerScreenLockTimer = OSDynamicCast( OSNumber, dict->getObject("Cypress5FingerScreenLockTimer"));
  OSNumber * pressureFiltering = OSDynamicCast( OSNumber, dict->getObject("CypressPressureFiltering"));
  OSNumber * twoFingerFiltering = OSDynamicCast( OSNumber, dict->getObject("Cypress2FingerFiltering"));
  OSNumber * threeFingerFiltering = OSDynamicCast( OSNumber, dict->getObject("Cypress3FingerFiltering"));
  OSNumber * fourFingerFiltering = OSDynamicCast( OSNumber, dict->getObject("Cypress4FingerFiltering"));

#ifdef DEBUG
  OSCollectionIterator* iter = OSCollectionIterator::withCollection( dict );
  OSObject* obj;

  iter->reset();
  while ((obj = iter->getNextObject()) != NULL)
    {
      OSString* str = OSDynamicCast( OSString, obj );
      OSNumber* val = OSDynamicCast( OSNumber, dict->getObject( str ) );
      if (val)
	DEBUG_LOG("%s: Dictionary Object: %s Value: %d\n", getName(),
		  str->getCStringNoCopy(), val->unsigned32BitValue());
      else
	DEBUG_LOG("%s: Dictionary Object: %s Value: ??\n", getName(),
		  str->getCStringNoCopy());
    }
#endif
  if (pressureFiltering)
    {
      _pressureFiltering = pressureFiltering->unsigned32BitValue();
      _kalZ.noiseLevel(_pressureFiltering);
      setProperty("CypressPressureFiltering", pressureFiltering);
    }
  if (twoFingerFiltering)
    {
      _twoFingerFiltering = twoFingerFiltering->unsigned32BitValue();
      setProperty("Cypress2FingerFiltering", twoFingerFiltering);
    }
  if (threeFingerFiltering)
    {
      _threeFingerFiltering = threeFingerFiltering->unsigned32BitValue();
      setProperty("Cypress3FingerFiltering", threeFingerFiltering);
    }
  if (fourFingerFiltering)
    {
      _fourFingerFiltering = fourFingerFiltering->unsigned32BitValue();
      setProperty("Cypress4FingerFiltering", fourFingerFiltering);
    }
  if (clicking)
    {
      _clicking = clicking->unsigned32BitValue() & 0x1 ? true : false;
      setProperty("Clicking", clicking);
    }
  if (dragging)
    {
      _dragging = dragging->unsigned32BitValue() & 0x1 ? true : false;
      setProperty("Dragging", dragging);
    }
  if (draglock)
    {
      _dragLock = draglock->unsigned32BitValue() & 0x1 ? true : false;
      setProperty("DragLock", draglock);
    }
  if (hscroll)
    {
      _trackpadHorizScroll = hscroll->unsigned32BitValue() & 0x1 ? true : false;
      setProperty("TrackpadHorizScroll", hscroll);
    }
  if (vscroll)
    {
      _trackpadScroll = vscroll->unsigned32BitValue() & 0x1 ? true : false;
      setProperty("TrackpadScroll", vscroll);
    }

  if (scrollspeed)
    {
      _twoFingerDivider = (float)(scrollspeed->unsigned32BitValue());
      setProperty("HIDTrackpadScrollAcceleration", scrollspeed);
    }
  if (dragPressureAverage)
    {
      _dragPressureAverage = dragPressureAverage->unsigned32BitValue();
      setProperty("CypressDragPressureAverage", dragPressureAverage);
    }
  if (onefingermaxtaptime)
    {
      _onefingermaxtaptime = ((onefingermaxtaptime->unsigned32BitValue()) * 1000000);
      setProperty("Cypress1FingerMaxTapTime", onefingermaxtaptime);
    }
  if (twofingermaxtaptime)
    {
      _twofingermaxtaptime = ((twofingermaxtaptime->unsigned32BitValue()) * 1000000);
      setProperty("Cypress2FingerMaxTapTime", twofingermaxtaptime);
    }
  if (threefingermaxtaptime)
    {
      _threefingermaxtaptime = ((threefingermaxtaptime->unsigned32BitValue()) * 1000000);
      setProperty("Cypress3FingerMaxTapTime", threefingermaxtaptime);
    }
  if (fourfingermaxtaptime)
    {
      _fourfingermaxtaptime = ((fourfingermaxtaptime->unsigned32BitValue()) * 1000000);
      setProperty("Cypress4FingerMaxTapTime", fourfingermaxtaptime);
    }
  if (fivefingermaxtaptime)
    {
      _fivefingermaxtaptime = ((fivefingermaxtaptime->unsigned32BitValue()) * 1000000);
      setProperty("Cypress3FingerMaxTapTime", fivefingermaxtaptime);
    }
  if (fourFingerHorizSwipeGesture)
    {
      _fourFingerHorizSwipeGesture = fourFingerHorizSwipeGesture->unsigned32BitValue() != 0 ? true : false;
      setProperty("CypressFourFingerHorizSwipeGesture", fourFingerHorizSwipeGesture);
    }
  if (fourFingerVertSwipeGesture)
    {
      _fourFingerVertSwipeGesture = fourFingerVertSwipeGesture->unsigned32BitValue() != 0 ? true : false;
      setProperty("CypressFourFingerVertSwipeGesture", fourFingerVertSwipeGesture);
    }
  if (threeFingerDrag)
    {
      _threeFingerDrag = threeFingerDrag->unsigned32BitValue() != 0 ? true : false;
      setProperty("CypressThreeFingerDrag", threeFingerDrag);
    }
  if (twoFingerRightClick)
    {
      _twoFingerRightClick = twoFingerRightClick->unsigned32BitValue() != 0 ? true : false;
      setProperty("CypressTwoFingerRightClick", twoFingerRightClick);
    }
  if (fiveFingerScreenLock)
    {
      _fiveFingerScreenLock = fiveFingerScreenLock->unsigned32BitValue() != 0 ? true : false;
      setProperty("CypressFiveFingerScreenLock", fiveFingerScreenLock);
    }
  if (fiveFingerSleep)
    {
      _fiveFingerSleep = fiveFingerSleep->unsigned32BitValue() != 0 ? true : false;
      setProperty("CypressFiveFingerSleep", fiveFingerSleep);
    }
  if (fiveFingerSleepTimer)
    {
      _fivefingersleeptime = ((fiveFingerSleepTimer->unsigned32BitValue()) * 1000000);
      setProperty("Cypress5FingerSleepTimer", fiveFingerSleepTimer);
    }
  if (fiveFingerScreenLockTimer)
    {
      _fivefingerscreenlocktime = ((fiveFingerScreenLockTimer->unsigned32BitValue()) * 1000000);
      setProperty("Cypress5FingerScreenLockTimer", fiveFingerScreenLockTimer);
    }
  return super::setParamProperties(dict);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void		ApplePS2CypressTouchPad::setDevicePowerState( UInt32 whatToDo )
{
    switch ( whatToDo )
    {
        case kPS2C_DisableDevice:
            
            //
            // Disable touchpad.
            //

	  setTouchPadEnable( false );
            break;

        case kPS2C_EnableDevice:
            
            //
            // Finally, we enable the trackpad itself, so that it may
            // start reporting asynchronous events.
            //
	  IOSleep(_wakeDelay);
	  initTouchPad();
            break;
	}
}

bool		ApplePS2CypressTouchPad::cypressVerifyCmdState(UInt8 cmd, UInt8 *param)
{
  bool		rate_match = false;
  bool		resolution_match = false;
  int		i;
    
    /* callers will do further checking. */
    if (cmd == CYTP_CMD_READ_CYPRESS_ID ||
        cmd == CYTP_CMD_STANDARD_MODE ||
        cmd == CYTP_CMD_READ_TP_METRICS)
      return true;
    
    if ((~param[0] & DFLT_RESP_BITS_VALID) == DFLT_RESP_BITS_VALID &&
        (param[0] & DFLT_RESP_BIT_MODE) == DFLT_RESP_STREAM_MODE)
      {
	for (i = 0; i < sizeof(_cytp_resolution); i++)
	  if (_cytp_resolution[i] == param[1])
	    resolution_match = true;
	for (i = 0; i < sizeof(_cytp_rate); i++)
	  if (_cytp_rate[i] == param[2])
	    rate_match = true;
	if (resolution_match && rate_match)
	  return true;
      }

    DEBUG_LOG("CYPRESS: cypressVerifyCmdState: verify cmd state failed\n");    
    return false;
}

UInt8			ApplePS2CypressTouchPad::cypressSendByte(UInt8 cmd)
{
  TPS2Request<1>	request;

  request.commands[0].command  = kPS2C_SendMouseCommandAndCompareAck;
  request.commands[0].inOrOut  = cmd & 0xFF;  // & 0xFF is useless but here to hint/guess from where it comes from ^^'

  request.commandsCount = 1;
  assert(request.commandsCount <= countof(request.commands));
  _device->submitRequestAndBlock(&request);

  if (request.commandsCount != 1)
    {
      DEBUG_LOG("CYPRESS: cypressSendByte: RETRY\n");
      return (CYTP_PS2_RETRY);
    }
  return (0);
}

UInt8		ApplePS2CypressTouchPad::cypressExtCmd(UInt8 cmd, UInt8 data)
{
  int		tries = CYTP_PS2_CMD_TRIES;
  UInt8		rc;

  do {
    /*
     * Send extension command byte (0xE8 or 0xF3).
     * If sending the command fails, send recovery command
     * to make the device return to the ready state.
     */
    rc = cypressSendByte(cmd & 0xff); // & 0xFF is useless but here to hint/guess from where it comes from ^^'
    if (rc == CYTP_PS2_RETRY)
      {
	rc = cypressSendByte(0x00);
	if (rc == CYTP_PS2_RETRY)
	  rc = cypressSendByte(0x0a);
      }
    if (rc == CYTP_PS2_ERROR)
      continue;

    rc = cypressSendByte(data);
    if (rc == CYTP_PS2_RETRY)
      rc = cypressSendByte(data);
    if (rc == CYTP_PS2_ERROR)
      continue;
    else
      break;
  } while (--tries > 0);
  
  return (rc);
}

UInt8		ApplePS2CypressTouchPad::cypressReadCmdStatus(UInt8 cmd, UInt8 *param)
{
  TPS2Request<> request;
  int		i;

  request.commands[0].command  = kPS2C_SendMouseCommandAndCompareAck;
  request.commands[0].inOrOut  = kDP_GetMouseInformation;
  for (i = 1; i < ((cmd == CYTP_CMD_READ_TP_METRICS ? 8 : 3) + 1); i++)
    {
      request.commands[i].command = kPS2C_ReadDataPort;
      request.commands[i].inOrOut = 0;
    }
  request.commandsCount = i;
  assert(request.commandsCount <= countof(request.commands));
  _device->submitRequestAndBlock(&request);
  DEBUG_LOG("CYPRESS: cypressReadCmdStatus: storing { ");
  for (i = 1; i < ((cmd == CYTP_CMD_READ_TP_METRICS ? 8 : 3) + 1); i++)
    {
      param[i - 1] = request.commands[i].inOrOut;
      DEBUG_LOG(" 0x%02x,", param[i - 1]);
    }
  DEBUG_LOG(" }\n");
  return (0);
}

bool		ApplePS2CypressTouchPad::cypressSendCmd(UInt8 cmd, UInt8 *param)
{
  int		tries = CYTP_PS2_CMD_TRIES;
  int		rc;

  DEBUG_LOG("CYPRESS: cypressSendCmd: send extension cmd 0x%02x, [%d %d %d %d]\n", cmd, DECODE_CMD_AA(cmd), DECODE_CMD_BB(cmd), DECODE_CMD_CC(cmd), DECODE_CMD_DD(cmd));
  do {
    cypressExtCmd(kDP_SetMouseResolution, DECODE_CMD_DD(cmd));
    cypressExtCmd(kDP_SetMouseResolution, DECODE_CMD_CC(cmd));
    cypressExtCmd(kDP_SetMouseResolution, DECODE_CMD_BB(cmd));
    cypressExtCmd(kDP_SetMouseResolution, DECODE_CMD_AA(cmd));
    rc = cypressReadCmdStatus(cmd, param);
    if (rc)
      continue;
    if (cypressVerifyCmdState(cmd, param))
      return (true);
  } while (--tries > 0);

  return (false);
}


bool		ApplePS2CypressTouchPad::cypressReadFwVersion()
{
  UInt8		param[3];
  bool		success;

  if (this->cypressSendCmd(CYTP_CMD_READ_CYPRESS_ID, param) == false)
    DEBUG_LOG("CYPRESS: Identify Bytes : Could not communicate with trackpad\n");
  else
    DEBUG_LOG("CYPRESS: Identify Bytes : { 0x%02x, 0x%02x, 0x%02x }\n", param[0], param[1], param[2]);

  if (param[0] == 0x33 && param[1] == 0xcc)
    success = true;
  else
    success = false;
  _touchPadVersion = (param[2] & FW_VERSION_MASX);
  _tpMetricsSupported = ((param[2] & TP_METRICS_MASK) && _touchPadVersion < 11) ? 1 : 0; // bogus fw 11 for getting metrics ... cf linux driver ...
  return (success);
}

bool            ApplePS2CypressTouchPad::cypressQueryHardware()
{
  if (cypressReadFwVersion() == false)
    return false;
  if (cypressReadTpMetrics() == false)
    return false;
  return true;
}

bool		ApplePS2CypressTouchPad::cypressDetect()
{
  unsigned char param[3];

  if (cypressSendCmd(CYTP_CMD_READ_CYPRESS_ID, param) == false)
    return false;
  /* Check for Cypress Trackpad signature bytes: 0x33 0xCC */
  if (param[0] != 0x33 || param[1] != 0xCC)
    return false;
  return true;
}

bool            ApplePS2CypressTouchPad::cypressReconnect()
{
  int		tries = CYTP_PS2_CMD_TRIES;
  bool		rc;

  if (_device)
    _device->lock();
  do {
    cypressReset();
    rc = cypressDetect();
  } while ((rc == false) && (--tries > 0));
  if (_device)
    _device->unlock();
  if (rc == false) {
    DEBUG_LOG("CYPRESS: cypressReconnect: unable to detect trackpad\n");
    return false;
  }

  if (setAbsoluteMode() == false)
    {
      DEBUG_LOG("CYPRESS: cypressReconnect: Unable to initialize Cypress absolute mode.\n");
      return false;
    }

  return true;
}

char		ApplePS2CypressTouchPad::fingersCount(unsigned char header)
{
  unsigned char	bits6_7 = header >> 6;
  unsigned char	finger_count = bits6_7 & 0x03;

  if (finger_count != 1 && header & ABS_HSCROLL_BIT)
    {
      /* HSCROLL gets added on to 0 finger count. */
      switch (finger_count)
        {
        case 0: return 4;
        case 2: return 5;
        default:
          /* Invalid contact (e.g. palm). Ignore it. */
          return -1;
        }
    }
  return ((UInt8)finger_count);
}

bool		ApplePS2CypressTouchPad::cypressReadTpMetrics()
{
  unsigned char	param[8];

  _tpWidth = CYTP_DEFAULT_WIDTH;
  _tpHigh = CYTP_DEFAULT_HIGH;
  _tpMaxAbsX = CYTP_ABS_MAX_X;
  _tpMaxAbsY = CYTP_ABS_MAX_Y;
  _tpMinPressure = CYTP_MIN_PRESSURE;
  _tpMaxPressure = CYTP_MAX_PRESSURE;
  _tpResX = _tpMaxAbsY / _tpWidth;
  _tpResY = _tpMaxAbsY / _tpHigh;

  if (!_tpMetricsSupported)
    return 0;

  this->myMemset(param, 0, sizeof(param));
  if (this->cypressSendCmd(CYTP_CMD_READ_TP_METRICS, param) == true)
    {
      _tpMaxAbsY = (param[1] << 8) | param[0];
      _tpMaxAbsX = (param[3] << 8) | param[2];
      _tpMinPressure = param[4];
      _tpMaxPressure = param[5];
    }

  if (!_tpMaxPressure || _tpMaxPressure < _tpMinPressure || !_tpWidth || !_tpHigh ||
      !_tpMaxAbsX || _tpMaxAbsX < _tpWidth || !_tpMaxAbsY || _tpMaxAbsY < _tpHigh)
    {
      // REVERT TO DEFAULT
      _tpWidth = CYTP_DEFAULT_WIDTH;
      _tpHigh = CYTP_DEFAULT_HIGH;
      _tpMaxAbsX = CYTP_ABS_MAX_X;
      _tpMaxAbsY = CYTP_ABS_MAX_Y;
      _tpMinPressure = CYTP_MIN_PRESSURE;
      _tpMaxPressure = CYTP_MAX_PRESSURE;
      _tpResX = _tpMaxAbsY / _tpWidth;
      _tpResY = _tpMaxAbsY / _tpHigh;
    }
  _tpResX = _tpMaxAbsX / _tpWidth;
  _tpResY = _tpMaxAbsY / _tpHigh;  
  return (0);
}

void		*ApplePS2CypressTouchPad::myMemset(void *s, int c, unsigned int n)
{
  unsigned int	k;
  unsigned char	*ptr;

  for (k = 0, ptr = (unsigned char *)s; k < n; k++)
    *(ptr + k) = (unsigned char )(c);
  return (s);
}

bool		ApplePS2CypressTouchPad::setAbsoluteMode()
{
  unsigned char	param[3];

  if (this->cypressSendCmd(CYTP_CMD_ABS_WITH_PRESSURE_MODE, param) == false)
    {
      DEBUG_LOG("CYPRESS: Error setting: CYTP_CMD_ABS_WITH_PRESSURE_MODE\n");
      return false;
    }
  _tpMode = (_tpMode & ~CYTP_BIT_ABS_REL_MASK)
    | CYTP_BIT_ABS_PRESSURE;
  _tpMode &= ~CYTP_BIT_HIGH_RATE;
  this->packetSize(5);
  return true;
}

int		ApplePS2CypressTouchPad::packetSize(int len)
{
#ifdef DEBUG
  if (len != -1 && len != _packetLength)
    DEBUG_LOG("CYPRESS: Changing Pcket Size From %d to %d\n", _packetLength, len);
#endif
  if (len == -1)
    return (_packetLength);
  _packetLength = len;
  return (_packetLength);
}

void		ApplePS2CypressTouchPad::updatePacketSize(unsigned char header)
{
  if (this->_tpMode & CYTP_BIT_ABS_NO_PRESSURE)
    this->packetSize(this->fingersCount(header) == 2 ? 7 : 4);
  else
    this->packetSize(this->fingersCount(header) == 2 ? 8 : 5);
}

void				ApplePS2CypressTouchPad::cypressProcessPacket(UInt8 *pkt)
{
  int				xdiff, ydiff;
  UInt32			buttons = 0;
  uint64_t			now_abs;
  struct cytp_report_data	report_data;
  int				n;

  if (cypressParsePacket(pkt, &report_data))
    return;
  n = report_data.contact_cnt;
  if (_frameType == n)
    _frameCounter++;
  else
    _frameCounter = (_frameType >= 4 && n == 1 ? 0 : 1); // last was 4 fingers, may have leading fingers ...
  _frameType = n;
  clock_get_uptime(&now_abs);
  if (_frameTimer == 0)
    _frameTimer = now_abs;
  if (_frameType > 1 && _frameCounter <= 1)
    {
      _kalX.noiseLevel((_frameType == 2 ? _twoFingerFiltering : (_frameType == 3 ? _threeFingerFiltering : _fourFingerFiltering)));
      _kalY.noiseLevel((_frameType == 2 ? _twoFingerFiltering : (_frameType == 3 ? _threeFingerFiltering : _fourFingerFiltering)));
    }
  _framePressure += (_pressureFiltering ? _kalZ.getNewValue(report_data.contacts[0].z) : report_data.contacts[0].z);
  if (n > 1)
    {
      // two, or more fingers
      if (n == 5 && _fiveFingerSleep && (now_abs - _frameTimer) > _fivefingersleeptime && _slept == false)
	{
	  _slept = true;
	  _device->dispatchKeyboardMessage(kPS2M_sleepComputer, &now_abs);
	  return ;
	}
      if (n == 4)
	{
	  if (_frameCounter > 4 && (now_abs - _frameTimer) > _fourfingermaxtaptime && (abs(_swipex) > _swipexThreshold ||  abs(_swipey * 2) > _swipeyThreshold))
	    {
	      int x = (_fourFingerFiltering ? _kalX.getNewValue(report_data.contacts[0].x) : report_data.contacts[0].x);
	      int y = (_fourFingerFiltering ? _kalY.getNewValue(report_data.contacts[0].y) : report_data.contacts[0].y);
	      DEBUG_LOG("CYPRESS: old coord: _xpos=%d _ypos=%d\n", _xpos, _ypos);
	      if (_x4pos < 0)
		_x4pos = x;
	      if (_y4pos < 0)
		_y4pos = y;
	      xdiff = x - _x4pos;
	      ydiff = y - _y4pos;
	      _swipey += ydiff;
	      _swipex += xdiff;
	      if (_swiped == false)// && (abs(_swipex) > _swipexThreshold ||  abs(_swipey) > _swipeyThreshold))
		{
		  DEBUG_LOG("CYPRESS: 4 Finger swipe : _swipex=%d _swipey=%d\n", _swipex, _swipey);
		  if (abs(_swipex) < abs((_swipey * 2)) && _fourFingerVertSwipeGesture) // need to keep ratio between x and y => y is less surface in size
		    _device->dispatchKeyboardMessage((_swipey < 0 ? kPS2M_swipeUp : kPS2M_swipeDown), &now_abs);
		  else
		    if (_fourFingerHorizSwipeGesture)
		      _device->dispatchKeyboardMessage((_swipex < 0 ? kPS2M_swipeRight : kPS2M_swipeLeft), &now_abs);
		  _swiped = true;
		  _swipey = 0;
		  _swipex = 0;
		}
	    }
	  else
	    {
	      int x = (_fourFingerFiltering ? _kalX.getNewValue(report_data.contacts[0].x) : report_data.contacts[0].x);
	      int y = (_fourFingerFiltering ? _kalY.getNewValue(report_data.contacts[0].y) : report_data.contacts[0].y);
	      if (_x4pos < 0)
		_x4pos = x;
	      if (_y4pos < 0)
		_y4pos = y;
	      xdiff = x - _x4pos;
	      ydiff = y - _y4pos;
	      _swipey += ydiff;
	      _swipex += xdiff;
	    }
	}
      if (_threeFingerDrag && n == 3)
	{
	  if (((now_abs - _frameTimer) > _threefingermaxtaptime))
	    {
	      int x = (_threeFingerFiltering ? _kalX.getNewValue(report_data.contacts[0].x) : report_data.contacts[0].x);
	      int y = (_threeFingerFiltering ? _kalY.getNewValue(report_data.contacts[0].y) : report_data.contacts[0].y);
	      DEBUG_LOG("CYPRESS: old coord: _xpos=%d _ypos=%d\n", _xpos, _ypos);
	      if (_xpos < 0)
		_xpos = x;
	      if (_ypos < 0)
		_ypos = y;
	      xdiff = x - _xpos;
	      ydiff = y - _ypos;

	      _xpos = x;
	      _ypos = y;
	      buttons |= 0x01; // three fingers window move
	      _pendingButtons = buttons;
	      xdiff = (int)((float)((float)xdiff / (float)_threefingerhdivider));
	      ydiff = (int)((float)((float)ydiff / (float)_threefingervdivider));
	      DEBUG_LOG("CYPRESS: Sending pointer event: %d,%d,%d\n", xdiff, ydiff,(int)buttons);
	      dispatchRelativePointerEventX(xdiff, ydiff, buttons, now_abs);
	    }
	  else
	    if (_threeFingerFiltering)
	      {
		_kalX.getNewValue(report_data.contacts[0].x);
		_kalY.getNewValue(report_data.contacts[0].y);
	      }
	}
      if (_trackpadScroll && n == 2) // cannot link with tap timer => create a delay on scrolling, which is a wrong/unwanted behaviour
	{
	  // two fingers
	  if ( _frameCounter >= _twoFingersMaxCount)
	    {
	      int x = (_twoFingerFiltering ? _kalX.getNewValue(report_data.contacts[0].x) : report_data.contacts[0].x);
	      int y = (_twoFingerFiltering ? _kalY.getNewValue(report_data.contacts[0].y) : report_data.contacts[0].y);
	      DEBUG_LOG("CYPRESS: old scroll coord: _xscrollpos=%d _yscrollpos=%d\n", _xscrollpos, _yscrollpos);
	      if (_xscrollpos < 0)
		_xscrollpos = x;
	      if (_yscrollpos < 0)
		_yscrollpos = y;
	      xdiff = x - _xscrollpos;
	      ydiff = y - _yscrollpos;
	      _xscrollpos = x;
	      _yscrollpos = y;
	      xdiff = (int)((float)((float)xdiff / (float)(((196608.0f - (float)_twoFingerDivider) / (_twoFingerFiltering ? 65536.0f : 32768.0f)) + 1.0f)));
	      ydiff = (int)((float)((float)ydiff / (float)(((196608.0f - (float)_twoFingerDivider) / (_twoFingerFiltering ? 65536.0f : 32768.0f)) + 1.0f)));
	      DEBUG_LOG("CYPRESS: Sending Scroll event: %d,%d\n", xdiff, ydiff);
	      if (_trackpadHorizScroll && abs(xdiff) > abs(ydiff))
		dispatchScrollWheelEventX(0, -xdiff, 0, now_abs);
	      else
		dispatchScrollWheelEventX(-ydiff, 0, 0, now_abs);
	    }
	  else
	    if (_twoFingerFiltering)
	      {
		_kalX.getNewValue(report_data.contacts[0].x);
		_kalY.getNewValue(report_data.contacts[0].y);
	      }
	}
    }
  else
    {
      // one finger
      int x = report_data.contacts[0].x;
      int y = report_data.contacts[0].y;
      DEBUG_LOG("CYPRESS: old coord: _xpos=%d _ypos=%d\n", _xpos, _ypos);
      if (_xpos < 0)
	_xpos = x;
      if (_ypos < 0)
	_ypos = y;
      xdiff = (n == 0 ? 0 : x - _xpos);
      ydiff = (n == 0 ? 0 : y - _ypos);
      _xpos = x;
      _ypos = y;
      if (_dragging)
	report_data.tap = (_frameCounter > 2 && (_framePressure / _frameCounter) > _dragPressureAverage ? 1 : 0);
      buttons |= (report_data.left || report_data.tap || _activeDragLock) ? 0x01 : 0;
      buttons |= report_data.right ? 0x02 : 0;
      _pendingButtons = buttons;
      xdiff = (int)((float)(((float)(xdiff)) / ((float)(_onefingerhdivider))));
      ydiff = (int)((float)(((float)(ydiff)) / ((float)(_onefingervdivider))));
      DEBUG_LOG("CYPRESS: Sending pointer event: %d,%d,%d\n", xdiff, ydiff,(int)buttons);
      dispatchRelativePointerEventX(xdiff, ydiff, buttons, now_abs);
    }
}


int		ApplePS2CypressTouchPad::cypressParsePacket(UInt8 *packet, struct cytp_report_data *report_data)
{
  int		contact_cnt;
  unsigned char	header_byte = packet[0];

  this->myMemset(report_data, 0, sizeof(struct cytp_report_data));
  contact_cnt = this->fingersCount(header_byte);
  if (contact_cnt < 0)
    {
      DEBUG_LOG("CYPRESS: cypressParsePacket: Error in contact_cnt\n");
      return (1);
    }
  else
    report_data->contact_cnt = contact_cnt;
  report_data->tap = (header_byte & ABS_MULTIFINGER_TAP) ? 1 : 0;
  if (report_data->contact_cnt == 1)
    {
      report_data->contacts[0].x = ((packet[1] & 0x70) << 4) | packet[2];
      report_data->contacts[0].y = ((packet[1] & 0x07) << 8) | packet[3];
      if (_tpMode & CYTP_BIT_ABS_PRESSURE)
	report_data->contacts[0].z = packet[4];
    }
  else if (report_data->contact_cnt >= 2)
    {
      report_data->contacts[0].x = ((packet[1] & 0x70) << 4) | packet[2];
      report_data->contacts[0].y = ((packet[1] & 0x07) << 8) | packet[3];
      if (_tpMode & CYTP_BIT_ABS_PRESSURE)
	report_data->contacts[0].z = packet[4];
      report_data->contacts[1].x = ((packet[5] & 0xf0) << 4) | packet[6];
      report_data->contacts[1].y = ((packet[5] & 0x0f) << 8) | packet[7];
      if (_tpMode & CYTP_BIT_ABS_PRESSURE)
	report_data->contacts[1].z = report_data->contacts[0].z;
    }
  report_data->left = (header_byte & BTN_LEFT_BIT) ? 1 : 0;
  report_data->right = (header_byte & BTN_RIGHT_BIT) ? 1 : 0;
  // remove that for prod
  DEBUG_LOG("CYPRESS: Counting Fingers: %d, tap %d\n", report_data->contact_cnt, report_data->tap);
#ifdef DEBUG
  int n = report_data->contact_cnt;
//   if (n > CYTP_MAX_MT_SLOTS)
//     n = CYTP_MAX_MT_SLOTS;
  int i;
  for (i = 0; i < n; i++)
    DEBUG_LOG("CYPRESS: contacts[%d] = {%d, %d, %d}\n", i, report_data->contacts[i].x, report_data->contacts[i].y, report_data->contacts[i].z);
  DEBUG_LOG("CYPRESS: l=%d, r=%d, m=%d\n", report_data->left, report_data->right, report_data->middle);
#endif
  return (0);
}

void		ApplePS2CypressTouchPad::initTouchPad()
{
  if (_device)
    _device->lock();
  _packetByteCount = 0;
  _ringBuffer.reset();
  setTouchpadModeByte();
  if (_device)
    _device->unlock();
}


bool		ApplePS2CypressTouchPad::setTouchpadModeByte()
{
  cypressSendByte(0xF5); // Disable Auto packet sending
  cypressReset();
  cypressQueryHardware();
  setAbsoluteMode();
  cypressSendByte(0xF6); // SetDefaults
  if (_touchPadVersion <= 11)
    {
      cypressSendByte(0xF3); // Set Rate
      cypressSendByte(200); // to 200dpi
      cypressSendByte(0xE8); // Set Resolution
      cypressSendByte(0x03); // to 8 count/mm
    }
  cypressSendByte(0xF4); // Set data reporting
  return (true);
}


void			ApplePS2CypressTouchPad::cypressReset()
{
  TPS2Request<2>	request;

  cypressSendByte(kDP_Reset);
  if (_wakeDelay)
    IOSleep(_wakeDelay);
  request.commands[0].command = kPS2C_ReadDataPort;
  request.commands[0].inOrOut = 0;
  request.commands[1].command = kPS2C_ReadDataPort;
  request.commands[1].inOrOut = 0;
  request.commandsCount = 2;
  assert(request.commandsCount <= countof(request.commands));
  _device->submitRequestAndBlock(&request);
#ifdef DEBUG
  if (request.commandsCount != 2)
    DEBUG_LOG("CYPRESS: reset warning: incomplete answer\n");
  if (request.commands[0].inOrOut != 0xAA && request.commands[1].inOrOut != 0x00)
    DEBUG_LOG("CYPRESS: Failed to reset mouse, return values did not match. [0x%02x, 0x%02x]\n", request.commands[1].inOrOut, request.commands[2].inOrOut);
  else
    DEBUG_LOG("CYPRESS: Successful mouse reset [ 0x%02x, 0x%02x ]\n", request.commands[1].inOrOut, request.commands[2].inOrOut);
#endif
  _tpMode = 0;
}
