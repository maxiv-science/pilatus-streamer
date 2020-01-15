import re
import time
import socket
import select

BUF_SIZE = 1024

class Pilatus:
    def __init__(self, hostname):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((hostname, 8888))
        self._started = False
        self.set_imgpath('/lima_data/')

    def _clear_buffer(self):
        while True:
            ready = select.select([self.sock], [], [], 0)
            if ready[0]:
                self.sock.recv(1024)
            else:
                break

    def query(self, command, timeout=1):
        if self.acquiring():
            print('Detector measuring, better not...')
            return ''
        self._clear_buffer()
        self.sock.send(bytes(command + '\0', encoding='ascii'))
        ready = select.select([self.sock], [], [], timeout)
        if ready[0]:
            response = self.sock.recv(BUF_SIZE).decode(encoding='ascii')
        else:
            response = None
        return response

    def parse_response(self, data, pattern):
        match = re.compile(pattern).match(data)
        ret = match.groups(1)[0] if match else None
        return ret

    def get_nimages(self):
        res = self.query('nimages')
        res = self.parse_response(res, '15 OK N images set to: (\d+)')
        nimages = int(res) if res else None
        return nimages

    def set_nimages(self, value):
        res = self.query('nimages %d' % value)
        if res is None or not res.startswith('15 OK'):
            print('Error setting nimages')

    def get_imgpath(self):
        res = self.query('imgpath')
        res = self.parse_response(res, '10 OK (.*)\x18')
        return res

    def set_imgpath(self, value):
        res = self.query('imgpath %s' % value)
        if res is None or not res.startswith('10 OK'):
            print('Error setting imgpath')

    def get_exptime(self):
        res = self.query('exptime')
        res = self.parse_response(res, '15 OK Exposure time set to: (.*) sec.\x18')
        return float(res)

    def set_exptime(self, value):
        res = self.query('exptime %f' % value)
        if res is None or not res.startswith('15 OK'):
            print('Error setting exptime')

    def get_expperiod(self):
        res = self.query('expperiod')
        res = self.parse_response(res, '15 OK Exposure period set to: (.*) sec\x18')
        return float(res)

    def set_expperiod(self, value):
        res = self.query('expperiod %f' % value)
        if res is None or not res.startswith('15 OK'):
            print('Error setting expperiod')

    def get_energy(self):
        res = self.query('setenergy', timeout=20)
        res = self.parse_response(res, '15 OK Energy setting: (.*) eV')
        energy = float(res) if res else None
        return energy

    def set_energy(self, value):
        res = self.query('setenergy %f' % value, timeout=20)
        if res is None or not res.startswith('15 OK'):
            print('Error setting energy')

    def exposure(self, filename=''):
        if self.get_exptime() > self.get_expperiod() - .003:
            print('Exposure time too long!')
            return
        res = self.query('exposure %s' % filename, timeout=10)
        if res is None or not res.startswith('15 OK  Starting'):
            print('Error starting exposure')
        else:
            self._started = True

    def acquiring(self):
        if not self._started:
            return False

        ready = select.select([self.sock], [], [], 0.0)
        if ready[0]:
            response = self.sock.recv(1024).decode(encoding='ascii')
            if response.startswith('7 OK'):
                self._started = False
                return False
        return True

    def stop(self):
        if not self.acquiring:
            return
        self.sock.send(b'k\0')
        buf = ''
        while '7 OK' not in buf:
            buf += self.sock.recv(1024).decode(encoding='ascii')
        self._started = False

