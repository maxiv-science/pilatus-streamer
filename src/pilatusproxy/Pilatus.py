import re
import time
import socket
import select

class Pilatus:
    def __init__(self, hostname):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((hostname, 8888))
        self.set_imgpath('/lima_data/')
        
    def parse_response(self, pattern, timeout):
        ready = select.select([self.sock], [], [], timeout)
        if ready[0]:
            response = self.sock.recv(1024)
            print(response)
            match = re.compile(pattern).match(response)
            ret = match.groups(1)[0] if match else None
            #print(ret)
            return ret

    def check_response(self, pattern, timeout):
        ready = select.select([self.sock], [], [], timeout)
        if ready[0]:
            response = self.sock.recv(1024)
            print(response)
            ret = True if response.startswith(pattern) else False
            return ret

    def get_nimages(self):
         self.sock.send(b'nimages\0')
         response = self.parse_response('15 OK N images set to: (\d+)', 10.0)
         nimages = int(response) if response else 0
         return nimages
   
    def set_nimages(self, value):
        self.sock.send(b'nimages %d\0' %value)
        if not self.check_response('15 OK', 10.0):
            print('Error setting nimages')
        self.nimages = value

    def get_imgpath(self):
        self.sock.send(b'imgpath\0')
        response = self.parse_response('10 OK (.*)\x18', 10.0)
        return response

    def set_imgpath(self, path):
        self.sock.send(b'imgpath %s\0' %path)
        if not self.check_response('10 OK', 10.0):
            print('Error setting imgpath')
            
    def get_exptime(self):
        self.sock.send(b'exptime\0')
        response = self.parse_response('15 OK Exposure time set to: (.*) sec.\x18', 10.0)
        exptime = float(response) if response else 0
        return exptime

    def set_exptime(self, value):
        self.sock.send(b'exptime %f\0' %value)
        if not self.check_response('15 OK', 10.0):
            print('Error setting imgpath')
            
    def get_expperiod(self):
        self.sock.send(b'expperiod\0')
        response = self.parse_response('15 OK Exposure period set to: (.*) sec\x18', 10.0)
        exptime = float(response) if response else 0
        return exptime

    def set_expperiod(self, value):
        self.sock.send(b'expperiod %f\0' %value)
        if not self.check_response('15 OK', 10.0):
            print('Error setting imgpath')
            
    def get_energy(self):
        self.sock.send(b'setenergy\0')
        response = self.parse_response('15 OK Energy setting: (.*) eV', 10.0)
        energy = float(response) if response else None
        return energy

    def set_energy(self, value):
        self.sock.send(b'setenergy %f\0' %value)
        if not self.check_response('15 OK', 20.0):
            print('Error setting energy')

    def exposure(self, filename=''):
        self.sock.send(b'exposure %s\0' %filename)
        if not self.check_response('15 OK  Starting', 10.0):
            print('Error starting exposure')
        
    def acquiring(self):
        ready = select.select([self.sock], [], [], 0.0)
        if ready[0]:
            response = self.sock.recv(1024)
            if response.startswith('7 OK'):
                return False
            else:
                print('Unkown response: %s' %response)
                return True
        else:
            return True
            

