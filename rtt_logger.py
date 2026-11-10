import pylink
import asyncio


class AsyncRTT:

    def __init__(self, chip_name, serial_number=None):
        self.jlink = pylink.JLink()
        self.chip_name = chip_name
        self.serial_number = serial_number
        self.is_running = False

    async def connect(self):
        """
        Connect to the J-Link and start RTT.
        """
        try:
            self.jlink.open(serial_no=self.serial_number)
            self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
            self.jlink.connect(self.chip_name)
            self.jlink.rtt_start()

            await asyncio.sleep(2)

            self.is_running = True
            return True
        
        except Exception as e:
            return False

    async def write(self, data, channel=0):
        """
        Write data to the RTT channel.
        """
        if isinstance(data, str):
            data = data.encode('utf-8')
        return self.jlink.rtt_write(channel, data)

    async def read_stream(self, channel=0, polling_interval=0.01):
        """
        Asynchronously read from the RTT channel in a loop.
        """
        while self.is_running:
            
            data = self.jlink.rtt_read(channel, 8192)
            if data:
                yield bytes(data).decode('utf-8', errors='replace')
            
            await asyncio.sleep(polling_interval)

    async def disconnect(self):
        """
        Stop RTT and close the J-Link connection.
        """
        self.is_running = False
        self.jlink.rtt_stop()
        self.jlink.close()
