import enum
import time
import usb.core
import usb.util as uu
VENDOR_ID=0x04d8
PRODUCT_ID=0xf4b5


dev = usb.core.find(idVendor=VENDOR_ID, idProduct=PRODUCT_ID)
if not dev:
    print("failed to find smartscope?")

print("device iSerial == ", dev.serial_number) # good, this works


# get pic version..
HEADER_CMD_BYTE = 0xC0 #C0 as in Command
HEADER_RESPONSE_BYTE = 0xAD #AD as in Answer Dude


class PIC_CMDS(enum.IntEnum):
            PIC_VERSION = 1,
            PIC_WRITE = 2,
            PIC_READ = 3,
            PIC_RESET = 4,
            PIC_BOOTLOADER = 5,
            EEPROM_READ = 6,
            EEPROM_WRITE = 7,
            FLASH_ROM_READ = 8,
            FLASH_ROM_WRITE = 9,
            I2C_WRITE = 10,
            I2C_READ = 11,
            PROGRAM_FPGA_START = 12,
            PROGRAM_FPGA_END = 13,
            I2C_WRITE_START = 14,
            I2C_WRITE_BULK = 15,
            I2C_WRITE_STOP = 16,

class REG(enum.IntEnum):
		STROBE_UPDATE = 0,
		SPI_ADDRESS = 1,
		SPI_WRITE_VALUE = 2,
		DIVIDER_MULTIPLIER = 3,
		CHA_YOFFSET_VOLTAGE = 4,
		CHB_YOFFSET_VOLTAGE = 5,
		TRIGGER_PWM = 6,
		TRIGGER_LEVEL = 7,
		TRIGGER_MODE = 8,
		TRIGGER_PW_MIN_B0 = 9,
		TRIGGER_PW_MIN_B1 = 10,
		TRIGGER_PW_MIN_B2 = 11,
		TRIGGER_PW_MAX_B0 = 12,
		TRIGGER_PW_MAX_B1 = 13,
		TRIGGER_PW_MAX_B2 = 14,
		INPUT_DECIMATION = 15,
		ACQUISITION_DEPTH = 16,
		TRIGGERHOLDOFF_B0 = 17,
		TRIGGERHOLDOFF_B1 = 18,
		TRIGGERHOLDOFF_B2 = 19,
		TRIGGERHOLDOFF_B3 = 20,
		VIEW_DECIMATION = 21,
		VIEW_OFFSET_B0 = 22,
		VIEW_OFFSET_B1 = 23,
		VIEW_OFFSET_B2 = 24,
		VIEW_ACQUISITIONS = 25,
		VIEW_BURSTS = 26,
		VIEW_EXCESS_B0 = 27,
		VIEW_EXCESS_B1 = 28,
		DIGITAL_TRIGGER_RISING = 29,
		DIGITAL_TRIGGER_FALLING = 30,
		DIGITAL_TRIGGER_HIGH = 31,
		DIGITAL_TRIGGER_LOW = 32,
		DIGITAL_OUT = 33,
		GENERATOR_DECIMATION_B0 = 34,
		GENERATOR_DECIMATION_B1 = 35,
		GENERATOR_DECIMATION_B2 = 36,
		GENERATOR_SAMPLES_B0 = 37,
		GENERATOR_SAMPLES_B1 = 38,

# send command = header_cmd, pic_version, and then read 16

EP_CMD_IN = 0x83
EP_CMD_OUT = 0x2
EP_DATA = 0x81

#print("Sending PIC RESET command", dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.PIC_RESET]))

def get_pic_ver():
    x = dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.PIC_VERSION.value])
    assert(x == 2)
    y = dev.read(EP_CMD_IN, 16)
    assert(len(y) == 16)
    print("got back on other ep: ", y)
    print([hex(q) for q in y])
    print("as per labnation: %x%x%x or bytes 6,5,4" % (y[6],y[5],y[4]))
    print([hex(q) for q in y])

get_pic_ver()

# src/Hardware/SmartScopeInterfaceUsb.cs is key
# Now, do an fpga i2c read of the fpga rom, to get the git version!

# ask the pic to make i2c comms to two different addresses the fpga responds on.  0xc for settings and 0xd for rom, 0xe for AWG
FPGA_I2C_ADDRESS_SETTINGS = 0x0c
FPGA_I2C_ADDRESS_ROM = 0x0D
#             FpgaRom = new Memories.ScopeFpgaRom(hardwareInterface, FPGA_I2C_ADDRESS_ROM);
#814:            return (UInt32)(FpgaRom[ROM.FW_GIT0].Read().GetByte() +

# for the next bits https://github.com/labnation/DeviceInterface/blob/master/src/Memories/ScopeFpgaI2cMemory.cs

# so, get controller register for FW_GIT0, 1,2,3) is

def get_i2c_reg(i2caddr, idx):
    dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.I2C_WRITE, 2, i2caddr<<1, idx])
    dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.I2C_READ, i2caddr, 1])
    y = dev.read(EP_CMD_IN, 16)
    return y[4]

def get_hw_rev():
    """Needed for working out which blob to load"""
    return dev.serial_number[-3:]

print("hw rev is ", get_hw_rev())
    

def maybe(idx):
    print("maybe for ", idx)
    x = dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.I2C_WRITE, 2, FPGA_I2C_ADDRESS_ROM<<1, idx]) # read reg 0 from i2c 0x0d.
    #print("set reg to read", x)
    # now the read itself
    x = dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.I2C_READ, FPGA_I2C_ADDRESS_ROM, 1]) # read 1 value from i2c 0x0d.
    #print("requested read", x)
    y = dev.read(EP_CMD_IN, 16)
    print("got back on other ep: ", y)
    print([hex(q) for q in y])
    return y[4]

def get_fpga_ver():
    fpgaver = [get_i2c_reg(FPGA_I2C_ADDRESS_ROM, x) for x in range(0,4)]
    fpgaver.reverse()
    return [hex(x) for x in fpgaver]

#fpgaver = [get_i2c_reg(FPGA_I2C_ADDRESS_ROM, x) for x in range(0,4)]
#fpgaver.reverse()
#print("fpga version is ", [hex(x) for x in fpgaver])
print("fpga version is ", get_fpga_ver())

# This works, but _only_ after running the smartscope softare to upload the fpga!
# now it gets the fpga rom version and it matches the version displayed in the ui


# Try and grab data
# ScopeConstants_GEN.cs
FETCH_SIZE_MAX = 2048 * 2 
SZ_HDR = 64 
PACKAGE_MAX = 64



def get_acquisition():
    tries = 0
    while True:
        hdr = dev.read(EP_DATA, SZ_HDR, 1000) # smartscope uses 3000, but what....
        if hdr[:1] == ['L', 'N']:
            break
        tries += 1
        if tries > PACKAGE_MAX:
            raise RuntimeException("too many tries trying to get header")
        
    if tries > 0:
        print("had to try %d times to get a header" % tries)

    print("Found matchin hdr", hdr)

def get_acquisition_mode():
#   return (AcquisitionMode)((FpgaSettingsMemory[REG.TRIGGER_MODE].GetByte() & 0xC0) >> 6);
    x = get_i2c_reg(FPGA_I2C_ADDRESS_SETTINGS, REG.TRIGGER_MODE)
    print("raw trigger mode raw = %x" % x)
    x = get_i2c_reg(FPGA_I2C_ADDRESS_SETTINGS, REG.TRIGGER_LEVEL)
    print("raw trigger level = %x" % x)

get_acquisition_mode()
    

BLOBPATH = "/home/karlp/src/smartscope-DeviceInterface/blobs"

def load_blob(path, hwrev):
    fn = "%s/SmartScope_%s.bin" % (path, hwrev)
    PACKSIZE = 32
    PADDING = 2048/8 # wat
    print("loading blob from %s" % fn)
    blob = None
    with open(fn, "rb") as _in:
        # fw blob will never be that big...
        blob = _in.read()
    plen = len(blob)
    cmd = int(plen / PACKSIZE + PADDING)
    print("loaded blob of length %d bytes, with cmd %d" % (plen, cmd))

    print("write start =>", dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.PROGRAM_FPGA_START, cmd >> 8, cmd & 0xff]))
    usb.control.clear_feature(dev, usb.control.ENDPOINT_HALT, EP_DATA)
    # workaround copied from labnation, not _entirely_sure it's necessary
    time.sleep(1)
    chunk, rem = blob[:2048], blob[2048:]
    while chunk:
        blob = rem
        print("write chunk =>", dev.write(EP_CMD_OUT, chunk))
        chunk, rem = blob[:2048], blob[2048:]
    # moah wat honestly. but ok.
    for i in range(int(PADDING)):
        dev.write(EP_CMD_OUT, [0xff] * 32)
    print("write finish =>", dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.PROGRAM_FPGA_END]))
    usb.control.clear_feature(dev, usb.control.ENDPOINT_HALT, EP_DATA)

    
load_blob(BLOBPATH, get_hw_rev())
print("after loading, can still get pic rev: ", get_pic_ver())

print("after loading fw, fw rev is", get_fpga_ver())
print("after loading", get_acquisition_mode())
