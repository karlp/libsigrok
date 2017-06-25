import enum
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

# send command = header_cmd, pic_version, and then read 16

EP_CMD_IN = 0x83
EP_CMD_OUT = 0x2
EP_DATA = 0x81

x = dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.PIC_VERSION.value])
print("wrote bytes cmmand", x)

y = dev.read(EP_CMD_IN, 16)
print("got back on other ep: ", y)
print([hex(q) for q in y])


# src/Hardware/SmartScopeInterfaceUsb.cs is key
# Now, do an fpga i2c read of the fpga rom, to get the git version!

# ask the pic to make i2c comms to two different addresses the fpga responds on.  0xc for settings and 0xd for rom, 0xe for AWG
# FPGA_I2C_ADDRESS_SETTINGS = 0x0c;
# FPGA_I2C_ADDRESS_ROM = 0x0D;
#             FpgaRom = new Memories.ScopeFpgaRom(hardwareInterface, FPGA_I2C_ADDRESS_ROM);
#814:            return (UInt32)(FpgaRom[ROM.FW_GIT0].Read().GetByte() +

# for the next bits https://github.com/labnation/DeviceInterface/blob/master/src/Memories/ScopeFpgaI2cMemory.cs

# so, get controller register for FW_GIT0, 1,2,3) is

def maybe(idx):
    print("maybe for ", idx)
    x = dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.I2C_WRITE, 2, 0x0d<<1, idx]) # read reg 0 from i2c 0x0d.
    #print("set reg to read", x)
    # now the read itself
    x = dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.I2C_READ, 0x0d, 1]) # read 1 value from i2c 0x0d.
    #print("requested read", x)
    y = dev.read(EP_CMD_IN, 16)
    print("got back on other ep: ", y)
    print([hex(q) for q in y])
    return y[4]

fpgaver = [hex(maybe(x)) for x in range(0,4)]
#maybe(0)
#maybe(1)
#maybe(2)
#maybe(3)
fpgaver.reverse()
print(fpgaver)

# This works, but _only_ after running the smartscope softare to upload the fpga!
# now it gets the fpga rom version and it matches the version displayed in the ui
