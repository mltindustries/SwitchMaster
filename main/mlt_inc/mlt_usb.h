

#define USB_1_CTRL_GPIO     32
#define USB_2_CTRL_GPIO     33  // unused in v2

#define USB_1_FAULT_GPIO    34
#define USB_2_FAULT_GPIO    35 // unused in v2

typedef enum usb_e {
    USB_1,
    USB_2 // unused in v2
}usb_e;



void usb_init(void);

void usb_set(usb_e channel);

void usb_clear(usb_e channel);

bool usb_hasfault(usb_e channel);
