# Tango imports
from tango import DevState
from tango.server import Device, attribute, command, server_run,  device_property
from .Pilatus import Pilatus

class PilatusDS(Device):
    """
    A Pilatus device
    """

    ### Properties ###

    Host = device_property(dtype=str, default_value="b-nanomax-pilatus1m-ipc-01", doc="hostname")

    ### Attributes ###

    @attribute(label='nimages', dtype=int, doc='Number of images')
    def nimages(self):
        return self.det.get_nimages()

    @nimages.write
    def nimages(self, val):
        self.det.set_nimages(val)

    @attribute(label='imgpath', dtype=str, doc='Local DCU image path', memorized=True, hw_memorized=True)
    def imgpath(self):
        return self.det.get_imgpath()

    @imgpath.write
    def imgpath(self, val):
        self.det.set_imgpath(val)

    @attribute(label='exptime', dtype=float, doc='Exposure time', unit='s')
    def exptime(self):
        return self.det.get_exptime()

    @exptime.write
    def exptime(self, val):
        self.det.set_exptime(val)

    @attribute(label='expperiod', dtype=float, doc='Exposure period (exposure time + readout)', unit='s')
    def expperiod(self):
        return self.det.get_expperiod()

    @expperiod.write
    def expperiod(self, val):
        self.det.set_expperiod(val)

    ### Commands ###
    
    @command(dtype_in=str)
    def exposure(self, filename):
        self.set_state(DevState.RUNNING)
        self.det.start(filename=filename, command='exposure')

    @command(dtype_in=str)
    def exttrigger(self, filename):
        self.set_state(DevState.RUNNING)
        self.det.start(filename=filename, command='exttrigger')

    @command(dtype_in=str)
    def extmtrigger(self, filename):
        self.set_state(DevState.RUNNING)
        self.det.start(filename=filename, command='extmtrigger')

    @command(dtype_in=str)
    def extenable(self, filename):
        self.set_state(DevState.RUNNING)
        self.det.start(filename=filename, command='extenable')

    @command()
    def stop(self):
        self.det.stop()

    ### Other stuff ###

    # Device methods
    def init_device(self):
        """Instantiate device object, do initial instrument configuration."""
        print('init_device()')
        self.set_state(DevState.INIT)
        
        try:
            self.get_device_properties()
        except Exception as e:
            self.set_state(DevState.FAULT)
            self.set_status('Device failed at init: %s' % e.message)

        print('going to connect to %s' % self.Host)
        self.det = Pilatus(self.Host)
        self.set_state(DevState.ON)

    # This sets the state before every command
    def always_executed_hook(self):
        if self.get_state() == DevState.RUNNING:
            if not self.det.acquiring():
                self.set_state(DevState.ON)

def main():
    server_run((PilatusDS,))

if __name__ == "__main__":
    main()


