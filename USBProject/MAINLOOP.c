
#include <stdio.h>
#include "BasicTyp.h"
#include "common.h"
#include "mainloop.h"
#include "usb.h"
#include "Hal4D13.h"
#include "chap_9.h"
#include "D13BUS.h"
#include "ISO.h"

//extern IO_REQUEST idata ioRequest;
extern D13FLAGS bD13flags;
//extern USBCHECK_DEVICE_STATES bUSBCheck_Device_State;
extern CONTROL_XFER ControlData;
extern IDLE_TIMER idletime;
extern HID_KEYS_REPORT hid_report;

//*************************************************************************
// USB protocol function pointer arrays
//*************************************************************************

#define MAX_STANDARD_REQUEST    0x0D
code void (*StandardDeviceRequest[])(void) =
{
  Chap9_GetStatus,
  Chap9_ClearFeature,
  Chap9_StallEP0,
  Chap9_SetFeature,
  Chap9_StallEP0,
  Chap9_SetAddress,
  Chap9_GetDescriptor,
  Chap9_StallEP0,
  Chap9_GetConfiguration,
  Chap9_SetConfiguration,
  Chap9_GetInterface,
  Chap9_SetInterface,
  Chap9_StallEP0
};


code CHAR * _NAME_USB_REQUEST_DIRECTION[] =
{
"Host_to_device",
"Device_to_host"
};

code CHAR * _NAME_USB_REQUEST_RECIPIENT[] =
{
"Device",
"Interface",
"Endpoint(0)",
"Other"
};

code CHAR * _NAME_USB_REQUEST_TYPE[] =
{
"Standard",
"Class",
"Vendor",
"Reserved"
};

code CHAR * _NAME_USB_STANDARD_REQUEST[] =
{
"GET_STATUS",
"CLEAR_FEATURE",
"RESERVED",
"SET_FEATURE",
"RESERVED",
"SET_ADDRESS",
"GET_DESCRIPTOR",
"SET_DESCRIPTOR",
"GET_CONFIGURATION",
"SET_CONFIGURATION",
"GET_INTERFACE",
"SET_INTERFACE",
"SYNC_FRAME"
};

//*************************************************************************
// Class device requests
//*************************************************************************
   
#define MAX_CLASS_REQUEST    0x0B
code void (*ClassDeviceRequest[])(void) =
{
		ML_Reserved,
		Get_Report,
		Get_Idle,
		Get_Protocol,
		ML_Reserved,
		ML_Reserved,
		ML_Reserved,
		ML_Reserved,
		ML_Reserved,
		Set_Report,
		Set_Idle,
		Set_Protocol
};

code CHAR * _NAME_USB_CLASS_REQUEST[] =
{
		"ML_Reserved",
		"Get_Report",
	    "Get_Idle",
	    "Get_Protocol",
	    "ML_Reserved",
	    "ML_Reserved",
	    "ML_Reserved",
	    "ML_Reserved",
	    "ML_Reserved",
	    "Set_Report",
	    "Set_Idle",
	    "Set_Protocol"
};

//*************************************************************************
// Vendor Device Request
//*************************************************************************

#define MAX_VENDOR_REQUEST    0x0f

code void (*VendorDeviceRequest[])(void) =
{
	  EnableIsoMode,
	  D13Bus_ControlEntry,
	  reserved,
	  reserved,
	  reserved,
	  reserved,
	  reserved,
	  reserved,
	  reserved,
	  reserved,
	  reserved,
	  reserved,
	  read_write_register,
	  reserved,
	  reserved,
	  reserved
};

code CHAR * _NAME_USB_VENDOR_REQUEST[] =
{
	"Iso mode enable",
	"Philips D13bus handler",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"read Firmware version ",
	"RESERVED",
	"RESERVED",
	"RESERVED",
};

void SetupToken_Handler(void)
{
  unsigned short j;
  RaiseIRQL();
  bD13flags.bits.At_IRQL1 = 1;
  ControlData.Abort = FALSE;

  ControlData.wLength = 0;
  ControlData.wCount = 0;

  j = Hal4D13_ReadEndpointWOClearBuffer(EPINDEX4EP0_CONTROL_OUT, &ControlData.DeviceRequest, sizeof(ControlData.DeviceRequest) );
  
 // printf("j:%d\n",j);
//  printf("sizeof(DEVICE_REQUEST):%d\n",sizeof(DEVICE_REQUEST));
/*
  printf("1:%X 2:%X 34:%X 56:%X 78:%X\n",
  ControlData.DeviceRequest.bmRequestType,
  ControlData.DeviceRequest.bRequest,
  ControlData.DeviceRequest.wValue,
  ControlData.DeviceRequest.wIndex,
  ControlData.DeviceRequest.wLength);
  */
  if( j == sizeof(DEVICE_REQUEST) )

  
//  if( Hal4D13_ReadEndpointWOClearBuffer(EPINDEX4EP0_CONTROL_OUT, (UCHAR *)(&(ControlData.DeviceRequest)), sizeof(ControlData.DeviceRequest))
//   == sizeof(DEVICE_REQUEST) )
  {
/*
	printf("ControlData.DeviceRequest_size:%ld\n", sizeof(ControlData.DeviceRequest));
    printf("ControlData.DeviceRequest.bmRequestType:0x%X\n",ControlData.DeviceRequest.bmRequestType);   //1BYTE
    printf("ControlData.DeviceRequest.bRequest:0x%X\n",ControlData.DeviceRequest.bRequest);//1BYTE
    printf("ControlData.DeviceRequest.wValue:0x%X\n",ControlData.DeviceRequest.wValue); //2BYTE
    printf("ControlData.DeviceRequest.wIndex:0x%X\n",ControlData.DeviceRequest.wIndex); //2BYTE
    printf("ControlData.DeviceRequest.wLength:0x%X\n",ControlData.DeviceRequest.wLength);//2BYTE
*/
    bD13flags.bits.At_IRQL1 = 0;
    LowerIRQL();
    ControlData.wLength = ControlData.DeviceRequest.wLength;
    ControlData.wCount = 0;

    if (ControlData.DeviceRequest.bmRequestType & (UCHAR)USB_ENDPOINT_DIRECTION_MASK)
    {
      /* get command */
      RaiseIRQL();
      ML_AcknowledgeSETUP();
      if((ControlData.DeviceRequest.bRequest == 0) & (ControlData.DeviceRequest.bmRequestType == 0xc0))
    	  bD13flags.bits.DCP_state = USBFSM4DCP_HANDSHAKE;
      else
    	  bD13flags.bits.DCP_state = USBFSM4DCP_REQUESTPROC;

      LowerIRQL();
    }
    else
    {
      /* set command */

      if (ControlData.DeviceRequest.wLength == 0)
      {
        /* Set command  without Data stage*/
        RaiseIRQL();
        ML_AcknowledgeSETUP();
        bD13flags.bits.DCP_state = USBFSM4DCP_REQUESTPROC;
        LowerIRQL();
      }
      else
      {
        /*
        // Set command  with Data stage
        // get Data Buffer
        */
        if(ControlData.DeviceRequest.wLength <= MAX_CONTROLDATA_SIZE)
        {
          /* set command with OUT token */
          RaiseIRQL();
          bD13flags.bits.DCP_state = USBFSM4DCP_DATAOUT;
          LowerIRQL();
          ML_AcknowledgeSETUP();

        }
        else
        {
          RaiseIRQL();
          ML_AcknowledgeSETUP();
          Hal4D13_StallEP0InControlWrite();
          bD13flags.bits.DCP_state = USBFSM4DCP_STALL;
          printf("bD13flags.bits.DCP_state = x%hx\n Unknow set up command\n", bD13flags.bits.DCP_state);
          LowerIRQL();
        }
      }
    }
  }
  else
  {
    printf("wrong setup command\n");
    bD13flags.bits.At_IRQL1 = 0;
    LowerIRQL();
    Chap9_StallEP0();
  }
  
 // printf("To_Ha_end\n");
}


void DeviceRequest_Handler(void)
{
  UCHAR type, req;

  type = ControlData.DeviceRequest.bmRequestType & USB_REQUEST_TYPE_MASK;
  req =  ControlData.DeviceRequest.bRequest & USB_REQUEST_MASK;

  //if (bD13flags.bits.verbose==1)
 //   printf("type = 0x%02x, req = 0x%02x\n", type, req);

  //help_devreq(type, req); /* print out device request */

  if ((type == USB_STANDARD_REQUEST) && (req < MAX_STANDARD_REQUEST))
  {
    (*StandardDeviceRequest[req])();
  }
  else if ((type == USB_CLASS_REQUEST) && (req < MAX_CLASS_REQUEST)) {
	  printf("Class Req. %d\n", req);
  	  (*ClassDeviceRequest[req])();
  }
  else if ((type == USB_VENDOR_REQUEST) && (req   < MAX_VENDOR_REQUEST))
    (*VendorDeviceRequest[req])();
  else{
    Chap9_StallEP0();
  }
}

void help_devreq(UCHAR type, UCHAR req)
{
  UCHAR typ = type;
  typ >>= 5;

  if(type == USB_STANDARD_REQUEST)
  {
    printf("Request Type = %s, Request = %s.\n", _NAME_USB_REQUEST_TYPE[typ],
      _NAME_USB_STANDARD_REQUEST[req]);
  }
  else if(type == USB_CLASS_REQUEST)
  {
    printf("Request Type = %s, Request = %s.\n", _NAME_USB_REQUEST_TYPE[typ],
      _NAME_USB_CLASS_REQUEST[req]);
  }
  else
  {
    if(bD13flags.bits.verbose)
      printf("Request Type = %s, bRequest = 0x%x.\n", _NAME_USB_REQUEST_TYPE[typ],
        req);
  }
}

void disconnect_USB(void)
{
  printf("disconnect\n");
    Hal4D13_SetDevConfig(
             D13REG_DEVCNFG_NOLAZYCLOCK
                        |D13REG_DEVCNFG_PWROFF
                        |D13REG_DEVCNFG_CLOCKRUNNING
                        );
            Hal4D13_SetMode(
    
             D13REG_MODE_INT_EN   );           

}

void connect_USB(void)
{

    RaiseIRQL(); //Disconnect irq
    printf("connect_USB\n");
    bD13flags.value = 0; /* reset event flags*/
    bD13flags.bits.DCP_state = USBFSM4DCP_IDLE;
    config_endpoint();

    LowerIRQL();

    Hal4D13_SetMode(   
             D13REG_MODE_SOFTCONNECT|
                       D13REG_MODE_DMA16 |
        //             D13REG_MODE_OFFGOODLNK  |
//| D13REG_MODE_DBG                 
             D13REG_MODE_INT_EN              
//| D13REG_MODE_SUSPND            

                       );

}


void config_endpoint(void)
{
    /*Control Endpoint*/
	//printf("CONFIG\n");
    Hal4D13_SetEndpointConfig(D13REG_EPCNFG_FIFO_EN|D13REG_EPCNFG_NONISOSZ_64,EPINDEX4EP0_CONTROL_OUT);
    Hal4D13_SetEndpointConfig(D13REG_EPCNFG_FIFO_EN|D13REG_EPCNFG_IN_EN|D13REG_EPCNFG_NONISOSZ_64,EPINDEX4EP0_CONTROL_IN);

    // ENDP01 : HID Keyboard IN
    Hal4D13_SetEndpointConfig(D13REG_EPCNFG_FIFO_EN| ENDPOINT_DIR_IN | D13REG_EPCNFG_DBLBUF_EN | ENDPOINT_NOT_ISO | D13REG_EPCNFG_NONISOSZ_8, EPINDEX4EP01);

    /*DISABLED*/
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN| */ D13REG_EPCNFG_DBLBUF_EN| D13REG_EPCNFG_IN_EN|D13REG_EPCNFG_NONISOSZ_8,EPINDEX4EP02);
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN|*/D13REG_EPCNFG_DBLBUF_EN|D13REG_EPCNFG_NONISOSZ_64,EPINDEX4EP03);
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN|*/D13REG_EPCNFG_DBLBUF_EN|D13REG_EPCNFG_IN_EN|D13REG_EPCNFG_NONISOSZ_64,EPINDEX4EP04);
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN|*/D13REG_EPCNFG_DBLBUF_EN|D13REG_EPCNFG_ISOSZ_512|D13REG_EPCNFG_ISO_EN,EPINDEX4EP05);
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN|*/D13REG_EPCNFG_DBLBUF_EN|D13REG_EPCNFG_ISOSZ_512|D13REG_EPCNFG_ISO_EN|D13REG_EPCNFG_IN_EN,EPINDEX4EP06);
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN|*/D13REG_EPCNFG_DBLBUF_EN|D13REG_EPCNFG_ISOSZ_16|D13REG_EPCNFG_ISO_EN,EPINDEX4EP07);
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN|*/D13REG_EPCNFG_IN_EN|D13REG_EPCNFG_DBLBUF_EN|D13REG_EPCNFG_ISOSZ_16|D13REG_EPCNFG_ISO_EN,EPINDEX4EP08);
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN|*/D13REG_EPCNFG_DBLBUF_EN|D13REG_EPCNFG_ISOSZ_16|D13REG_EPCNFG_ISO_EN,EPINDEX4EP09);
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN|*/D13REG_EPCNFG_IN_EN|D13REG_EPCNFG_DBLBUF_EN|D13REG_EPCNFG_ISOSZ_16|D13REG_EPCNFG_ISO_EN,EPINDEX4EP0A);
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN|*/D13REG_EPCNFG_DBLBUF_EN|D13REG_EPCNFG_ISOSZ_64|D13REG_EPCNFG_ISO_EN,EPINDEX4EP0B);
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN|*/D13REG_EPCNFG_IN_EN|D13REG_EPCNFG_DBLBUF_EN|D13REG_EPCNFG_ISOSZ_64|D13REG_EPCNFG_ISO_EN,EPINDEX4EP0C);
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN|*/D13REG_EPCNFG_DBLBUF_EN|D13REG_EPCNFG_ISOSZ_64|D13REG_EPCNFG_ISO_EN,EPINDEX4EP0D);
    Hal4D13_SetEndpointConfig(/*D13REG_EPCNFG_FIFO_EN|*/D13REG_EPCNFG_IN_EN|D13REG_EPCNFG_DBLBUF_EN|D13REG_EPCNFG_ISOSZ_64|D13REG_EPCNFG_ISO_EN,EPINDEX4EP0E);
    /*DISABLED*/

    
    /*Set interrupt configuration*/
    Hal4D13_SetIntEnable(
    					 D13REG_INTSRC_EP0OUT
                        |D13REG_INTSRC_EP0IN
                        |D13REG_INTSRC_EP01
//                      |D13REG_INTSRC_EP02
//                      |D13REG_INTSRC_EP03
//               		|D13REG_INTSRC_EP04
//			            |D13REG_INTSRC_EP05
//			            |D13REG_INTSRC_EP06
                        |D13REG_INTSRC_SUSPEND
                        |D13REG_INTSRC_RESUME
                        |D13REG_INTSRC_BUSRESET
    );
                        
    /*Set Hardware Configuration*/
    Hal4D13_SetDevConfig(D13REG_DEVCNFG_NOLAZYCLOCK
                        |D13REG_DEVCNFG_CLOCKDIV_120M
                        |D13REG_DEVCNFG_DMARQPOL
//            			|D13REG_DEVCNFG_EXPULLUP
);


}

void reconnect_USB(void)
{
    disconnect_USB();
    connect_USB();
}

void suspend_change(void)
{
    printf("SUSPEND CHANGE \n");
 // disconnect_USB();
//  Suspend_Device_Controller();
}

void ML_AcknowledgeSETUP(void)
{

    if( Hal4D13_IsSetupPktInvalid() || ControlData.Abort)
    {
        return;
    }

    Hal4D13_AcknowledgeSETUP();
    Hal4D13_ClearBuffer(EPINDEX4EP0_CONTROL_OUT);
}



void ML_Reserved(void)
{
    Hal4D13_ClearBuffer(EPINDEX4EP0_CONTROL_OUT);
    printf("Reserved\n");
}

void Get_Report() {
	UCHAR   bDescriptor =      MSB(ControlData.DeviceRequest.wValue);
	UCHAR   bDescriptorIndex = LSB(ControlData.DeviceRequest.wValue);
	USHORT	bLen = ControlData.DeviceRequest.wLength;
//	printf("bDescri = %d : ",bDescriptor);
//	printf("Index = %d\n",bDescriptorIndex);

	//hid_report.modifier = 0x00;
	//hid_report.reserved = 0x00;
	//hid_report.keycode = 0x00;

	if(sizeof(hid_report) <= bLen){
		Chap9_BurstTransmitEP0((PUCHAR)&hid_report, sizeof(hid_report));
	}
	//printf("Get Report\n");
}

void Get_Idle() {
	printf("Get Idle\n");
}

void Get_Protocol() {
	printf("Get Protocol\n");
}

void Set_Report() {
	printf("Set Report\n");
}

void Set_Idle() {
	printf("Set Idle\n");

	UCHAR duration = MSB(ControlData.DeviceRequest.wValue);
	UCHAR reportID = LSB(ControlData.DeviceRequest.wValue);

	printf("Duration: %x, ReportID: %x\n", duration, reportID);

	idletime.wait_time = 0;
	if(duration != 0) {
		idletime.wait_time = duration * 4; //wait time in milliseconds
		printf("wait_time set to = %d millis\n", idletime.wait_time);
	}
	if(reportID != 0) {
		printf("ReportID = %ud\n", reportID);
	}
}

void Set_Protocol() {
	printf("Set Protocol\n");
}

unsigned short CHECK_CHIP_ID(void)
{
    unsigned short CHIP_ID;
    unsigned char LOW_ID, HIGH_ID;

    CHIP_ID = Hal4D13_ReadChipID();
    LOW_ID = (unsigned char)CHIP_ID;
    HIGH_ID = (unsigned char)(CHIP_ID >> 8) ;

    switch(HIGH_ID)
    {
     case 0x61 : {
                    
                         printf("               CHIP ID =0x%04x\n\n",CHIP_ID);
                        CHIP_ID =0x1161;
                        return CHIP_ID;
                         break;
                 }

     case 0x36 : {
                             printf("               CHIP ID =0x%04x\n\n",CHIP_ID);
                         CHIP_ID = 0x1362;
                         return CHIP_ID;
                         break;
                 }

     default   : {
                 printf("               UNKNOWN CHIP ID =0x%04x\n\n",CHIP_ID);
                 return CHIP_ID;
                 break;
                 }


    }


}
