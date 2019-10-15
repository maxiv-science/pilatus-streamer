# Tango imports
from tango import DevState
from tango.server import Device, attribute, command, server_run,  device_property


class PilatusProxyDS(Device):
    """ A Pilatus device

       Device States Description:
    #
    #   DevState.ON :     The device is in operation
    #   DevState.INIT :   Initialisation of the communication with the device and initial configuration
    #   DevState.FAULT :  The Device has a problem and cannot handle further requests.
    #   DevState.MOVING : The Device is engaged in an acquisition.
    """

    ### Properties ###

    Host = device_property(dtype=str, default_value="b-nanomax-mobile-ipc-01", doc="hostname")
    Port = device_property(dtype=int, default_value=41234)

    ### Attributes ###

    @attribute(label='nimages', dtype=int, doc='Number of images')
    def nimages(self):
        print('getting nimages')
        return 123

    @nimages.write
    def nimages(self, val):
        print('setting nimages to %u' % val)

    @attribute(label='imgpath', dtype=str, doc='Local DCU image path')
    def imgpath(self):
        print('getting imgpath')
        return '/some/path'

    @imgpath.write
    def imgpath(self, val):
        print('setting imgpath to %s' % val)

    @attribute(label='exptime', dtype=float, doc='Exposure time', unit='s')
    def exptime(self):
        print('getting exptime')
        return .007

    @exptime.write
    def exptime(self, val):
        print('setting exptime to %f' % val)

    @attribute(label='expperiod', dtype=float, doc='Exposure period (exposure time + readout)', unit='s')
    def expperiod(self):
        print('getting expperiod')
        return .010

    @expperiod.write
    def expperiod(self, val):
        print('setting expperiod to %f' % val)

    ### Commands ###
    
    @command(dtype_in=str)
    def exposure(self, filename):
        print('Software triggered exposure to %s' % filename)

    @command(dtype_in=str)
    def exttrigger(self, filename):
        print('Hardware triggered exposure to %s (one trigger for the whole train)' % filename)

    @command(dtype_in=str)
    def extmtrigger(self, filename):
        print('Hardware triggered exposure to %s (one trigger per frame)' % filename)


    ### Other stuff ###

    # Device methods
    def init_device(self):
        """Instantiate device object, do initial instrument configuration."""
        self.set_state(DevState.INIT)
        
        try:
            self.get_device_properties()
        
        except Exception as e:
            self.set_state(DevState.FAULT)
            self.set_status('Device failed at init: %s' % e.message)
        self.set_state(DevState.ON)

    # This sets the state before every command
    def always_executed_hook(self):
        print('Here we should update the state, is the pilatus ON, MOVING, FAULT, or INIT?')


def main():
    server_run((PilatusProxyDS,))

if __name__ == "__main__":
    main()


