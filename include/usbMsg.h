
#define USB_PACKET_STR 0
#define USB_PACKET_MCT 1
#define USB_PACKET_LEN 2
#define USB_PACKET_PID 3
#define USB_PACKET_CMD 4
#define USB_PACKET_DATA 5


#define USB_CMD_NULL         0x00
#define USB_CMD_RESEND       0x01
#define USB_CMD_GET_VSTRING  0x17
#define USB_CMD_GET_CHAN     0x33
#define USB_CMD_TRACK        0x40
#define USB_CMD_PARAM        0x41
#define USB_CMD_GET_MIRRORS  0x42
#define USB_CMD_GET_STRING   0x60
#define USB_CMD_FIELD_STATE  0x61
#define USB_CMD_GET_FCE      0x62
#define USB_CMD_GET_RTU      0x63
#define USB_CMD_SEND_MCT485  0x64
#define USB_CMD_GET_MCT485   0x65
#define USB_CMD_RTC          0x66
#define USB_CMD_LOG          0x67
#define USB_CMD_DESICCANT    0x68
#define USB_CMD_SFC_PARAM    0x69
#define USB_CMD_MEMORY       0x6A
#define USB_CMD_TEST         0x6B

#define USB_RESP_NULL        0x80
#define USB_RESP_BAD_MSG     0x81
#define USB_RESP_GET_VSTRING 0x97
#define USB_RESP_GET_CHAN    0xB3
#define USB_RESP_TRACK       0xC0
#define USB_RESP_PARAM       0xC1
#define USB_RESP_GET_MIRRORS 0xC2
#define USB_RESP_GET_STRING  0xE0
#define USB_RESP_FIELD_STATE 0xE1
#define USB_RESP_GET_FCE     0xE2
#define USB_RESP_GET_RTU     0xE3
#define USB_RESP_SEND_MCT485 0xE4
#define USB_RESP_GET_MCT485  0xE5
#define USB_RESP_RTC         0xE6
#define USB_RESP_LOG         0xE7
#define USB_RESP_DESICCANT   0xE8
#define USB_RESP_SFC_PARAM   0xE9
#define USB_RESP_MEMORY      0xEA
#define USB_RESP_TEST        0xEB


#define USB_ERR_BAD_LEN 0xFF
#define USB_ERR_BAD_CMD 0xFE
#define USB_ERR_BAD_PRM 0xFD

void UsbSendResp(byte resp, byte *data);
