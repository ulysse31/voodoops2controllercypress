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
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 * THIS CODE IS UNDER DEVELOPMENT, IT IS PRE-ALPHA, INCOMPLETE AND BUGGY
 *			DO NOT USE IT ON PRODUCTION
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 *
 * Some basic functions as been ported from linux, but the processing 
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
    
    // initialize state...
    _device                    = 0;
    _interruptHandlerInstalled = false;
    _packetByteCount           = 0;
    _resolution                = (100) << 16; // (100 dpi, 4 counts/mm)
    _xpos		       = -1;
    _ypos		       = -1;
    _x4pos		       = -1;
    _y4pos		       = -1;
    _xscrollpos		       = -1;
    _yscrollpos		       = -1;
    _pendingButtons	       = 0;
    _frameCounter	       = 0;
    _frameType		       = -1;
    _swipey		       = 0;
    _swipex		       = 0;
    _swiped		       = false;

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
    _device = 0;
    DEBUG_LOG("CYPRESS: ApplePS2CypressTouchPad::probe leaving.\n");

    return (success) ? this : 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool ApplePS2CypressTouchPad::start( IOService * provider )
{ 
    UInt64 enabledProperty;

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

    setProperty("Clicking", enabledProperty, 
        sizeof(enabledProperty) * 8);
    setProperty("TrackpadScroll", enabledProperty, 
        sizeof(enabledProperty) * 8);
    setProperty("TrackpadHorizScroll", enabledProperty, 
        sizeof(enabledProperty) * 8);

    //
    // Must add this property to let our superclass know that it should handle
    // trackpad acceleration settings from user space.  Without this, tracking
    // speed adjustments from the mouse prefs panel have no effect.
    //

    setProperty(kIOHIDPointerAccelerationTypeKey, kIOHIDTrackpadAccelerationType);

    //
    // Lock the controller during initialization
    //
    
    _device->lock();
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

void ApplePS2CypressTouchPad::packetReady()
{
  //UInt8	packet[8];
  UInt8	fingers;
    // empty the ring buffer, dispatching each packet...
  // here need to implement cypress_protocol_handler / cypress_process_packet
  // minimum size = 5
  while (_ringBuffer.count() >= kPacketLengthLarge)
    {
      UInt8 *packet = _ringBuffer.tail();
      this->updatePacketSize(packet[0]);
      UInt8 size = packetSize();
      //DEBUG_LOG("CYPRESS:  %s: packetReady BEGIN Loop: rest %d bytes in ringbuffer, size is %d\n", getName(), _ringBuffer.count(), size);
      // this call to myMemset is only temporary debug ... should use directly the pointer to ringbuffer on prod
      //this->myMemset(packet, 0, 8);
      if (_ringBuffer.count() < size)
	return ;
      // should be deleted once communication with PS2 cypress trackpad stable
#ifdef DEBUG
      if (packet[0] || packet[1] || packet[2] || packet[3] || packet[4] || packet[5] || packet[6] || packet[7])
	DEBUG_LOG("CYPRESS: %s: packet dump { 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x }\n", getName(), packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6], packet[7]);
#endif
      fingers = fingersCount(packet[0]);
      if (packet[0] == 0x00 && packet[1] == 0x00 && packet[2] == 0x00 && packet[3] == 0x00
	  && packet[4] == 0x00 && packet[5] == 0x00 && packet[6] == 0x00 && packet[7] == 0x00)
	{
	  // Empty packet, received lots (500 bytes) when an action ends (finger leave)
	  _ringBuffer.advanceTail(kPacketLengthLarge);
	  _xpos = -1;
	  _ypos = -1;
	  _x4pos = -1;
	  _y4pos = -1;
	  _xscrollpos = -1;
	  _yscrollpos = -1;
	  if (_frameType == 1 && _frameCounter > 0 && _frameCounter < 5)
	    {
	      // simulate a tap here
	      uint64_t	now_abs;
	      clock_get_uptime(&now_abs);
	      dispatchRelativePointerEventX(0, 0, 0x01, now_abs);
	      clock_get_uptime(&now_abs);
	      dispatchRelativePointerEventX(0, 0, 0x00, now_abs);
	    }
	  if (_frameType == 3 && _frameCounter > 0 && _frameCounter < 5)
	    {
	      // simulate a tap here
	      uint64_t	now_abs;
	      clock_get_uptime(&now_abs);
	      dispatchRelativePointerEventX(0, 0, 0x01, now_abs);
	      clock_get_uptime(&now_abs);
	      dispatchRelativePointerEventX(0, 0, 0x00, now_abs);
	    }
	  _frameCounter = 0;
	  _frameType = -1;
	  _swipey = 0;
	  _swipex = 0;
	  _swiped = false;
	  if (_pendingButtons &&  packet[0] == 0 && packet[1] == 0 && packet[2] == 0 && packet[3] == 0
	      && packet[4] == 0 && packet[5] == 0 && packet[6] == 0 && packet[7] == 0) // buttons were used need to clear events
	    {
	      this->cypressProcessPacket(packet);
	      _pendingButtons = 0;
	    }
	  continue ;
	}
      if (((((packet[0] & 0x40) != 0x40) && ((packet[0] & 0x80) != 0x80) && ((packet[0] & 0x20) != 0x20) && (packet[0] > 2)))
	  || ((packet[0] & 0x08) == 0x08 && fingers < 0) || ((packet[0] & 0x01) == 0x01 && (packet[0] & 0x02) == 0x02)
	  || (packet[0] == 0 && (packet[1] != 0x00 || packet[2] != 0x00 || packet[3] != 0x00 || packet[4] != 0x00 || packet[5] != 0x00 || packet[6] != 0x00 || packet[7] != 0x00) ))
	{
	  // incomplete packet : invalid header, de-sync'ed packets (start and ends in middle of 2 stored packets), etc ... => drop queue and ask re-send
	  // Thanks to dudley at cypress for is (short but enought) doc sheet
	  _xpos = -1;
	  _ypos = -1;
	  _x4pos = -1;
	  _y4pos = -1;
	  _xscrollpos = -1;
	  _yscrollpos = -1;
	  _packetByteCount = 0;
	  _frameCounter = 0;
	  _frameType = -1;
	  _swipey = 0;
	  _swipex = 0;
	  _swiped = false;
	  _pendingButtons = 0;
	  _ringBuffer.advanceTail(_ringBuffer.count());
	  cypressSendByte(0xFE);
	  return ;
	}
      this->cypressProcessPacket(packet);
      _ringBuffer.advanceTail(kPacketLengthLarge);
      //DEBUG_LOG("CYPRESS:  %s: packetReady END Loop: rest %d bytes in ringbuffer, size is %d\n", getName(), _ringBuffer.count(), size);
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
  if (n < 0)
    return ;
//   if (n > CYTP_MAX_MT_SLOTS)
//     n = CYTP_MAX_MT_SLOTS;
//   if ((_frameType >= 1) && _frameType != n) // discard packet, need to reset counters
//     return ;
  //  if (_frameType == -1)
  //    _frameType = n;
  if (_frameType == n)
    _frameCounter++;
  else
    _frameCounter = 1;
  _frameType = n;
  clock_get_uptime(&now_abs);
  if (n > 1)
    {
      // two, or more fingers
      if (n == 4)
	{
	  int x = report_data.contacts[0].x;
	  int y = report_data.contacts[0].y;
	  DEBUG_LOG("CYPRESS: old coord: _xpos=%d _ypos=%d\n", _xpos, _ypos);
	  if (_x4pos < 0)
	    _x4pos = x;
	  if (_y4pos < 0)
	    _y4pos = y;
	  xdiff = x - _x4pos;
	  ydiff = y - _y4pos;
	  _swipey += ydiff;
	  _swipex += xdiff;
	  if (_frameCounter >= 2 && _swiped == false && (abs(_swipex) > 10 ||  abs(_swipey) > 10))
	    {
	      DEBUG_LOG("CYPRESS: 4 Finger swipe : _swipex=%d _swipey=%d\n", _swipex, _swipey);
	      if (abs(_swipex) < abs(_swipey))
		_device->dispatchKeyboardMessage((_swipey < 0 ? kPS2M_swipeUp : kPS2M_swipeDown), &now_abs);
	      else
		_device->dispatchKeyboardMessage((_swipex < 0 ? kPS2M_swipeLeft : kPS2M_swipeRight), &now_abs);
	      _swiped = true;
	      _swipey = 0;
	      _swipex = 0;
	    }
	}
      if (n == 3 && _frameCounter >= 2)
	{
	  int x = report_data.contacts[0].x;
	  int y = report_data.contacts[0].y;
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
	  xdiff /= 2;
	  ydiff /= 2;
	  DEBUG_LOG("CYPRESS: Sending pointer event: %d,%d,%d\n", xdiff, ydiff,(int)buttons);
	  dispatchRelativePointerEventX(xdiff, ydiff, buttons, now_abs);
	}
      if (n == 2 && _frameCounter >= 2)
	{
	  // two fingers
	  int x = report_data.contacts[0].x;
	  int y = report_data.contacts[0].y;
	  DEBUG_LOG("CYPRESS: old scroll coord: _xscrollpos=%d _yscrollpos=%d\n", _xscrollpos, _yscrollpos);
	  if (_xscrollpos < 0)
	    _xscrollpos = x;
	  if (_yscrollpos < 0)
	    _yscrollpos = y;
	  xdiff = x - _xscrollpos;
	  ydiff = y - _yscrollpos;
	  _xscrollpos = x;
	  _yscrollpos = y;
	  //buttons |= report_data.left ? 0x01 : 0;
	  //buttons |= report_data.right ? 0x02 : 0;
	  xdiff /= 4;
	  ydiff /= 4;
	  DEBUG_LOG("CYPRESS: Sending Scroll event: %d,%d\n", xdiff, ydiff);
	  dispatchScrollWheelEventX(-ydiff, -xdiff, 0, now_abs);
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
      buttons |= (report_data.left || report_data.tap) ? 0x01 : 0;
      buttons |= report_data.right ? 0x02 : 0;
      _pendingButtons = buttons;
      xdiff /= 2;
      ydiff /= 2;
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
  cypressSendByte(0xF3); // Set Rate
  cypressSendByte(200); // to 200dpi
  cypressSendByte(0xE8); // Set Resolution
  cypressSendByte(0x03); // to 8 count/mm
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
