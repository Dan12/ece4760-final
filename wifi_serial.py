class WifiSerial:
    def __init__(self, ser, prt):
        self.ser = ser
        self.prt = prt
        self.proc_queue = []

    def pop_proc_queue(self):
        if self.proc_queue:
            return self.proc_queue.pop(0)
        return None

    def read_line(self):
        line = self.ser.readline()
        self.proc_queue.append(line)
        return line

    def write_data(self, linkId, data):
        ser = self.ser

        ser.flushInput()
        self.write_cmd("AT+CIPSEND=" + str(linkId) + "," + str(len(data)) + "\r\n")

        ser.write((data).encode())
        while(1):
            ret = self.read_line()
            self.prt("Command result: %s" % ret)
            if ("OK" in ret.decode()):
                break
            if ("ERROR" in ret.decode()):
                raise Exception("write data error")

    def write_cmd(self, cmd):
        ser = self.ser

        ser.flushInput()
        ser.write((cmd + "\r\n").encode())

        return self.read_for_ok()

    def read_for_ok(self):
        ser = self.ser

        ret = b''
        while(1):
            line = self.read_line()
            ret += line
            self.prt("Command result: %s" % line)
            try:
                if ("OK" in line.decode()):
                    break
                if ("ERROR" in line.decode()):
                    raise Exception("cmd error")
            except UnicodeDecodeError :
                self.prt("error decoding")
        return ret
