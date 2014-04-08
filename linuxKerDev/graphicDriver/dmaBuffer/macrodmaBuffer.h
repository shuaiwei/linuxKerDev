#define PCI_VENDOR_ID_CCORSI (0x1234)
#define PCI_DEVICE_ID_KYOUKO2 (0x1113)
#define KYOUKO2_CONTROL_SIZE (65536)
#define BUFFERS 8

#define GRAPHICS_ON 1
#define GRAPHICS_OFF 0
#define MAGIC_NUM 0xcc
#define VMODE		_IOW(MAGIC_NUM, 0, unsigned long)
#define BIND_DMA	_IOW(MAGIC_NUM, 1, unsigned long)
#define START_DMA	_IOWR(MAGIC_NUM, 2, unsigned long)
#define SYNC 		_IO(MAGIC_NUM, 3)
#define FLUSH 		_IO(MAGIC_NUM, 4)