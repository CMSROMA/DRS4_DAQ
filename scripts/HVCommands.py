import serial
con = serial.Serial('/dev/ttyACM0')

def waitAnswer(response):
        reply = ''
        while True:
            x = con.readline()
            reply += x
            if x == response+'\r\n':
                return reply

def sendCommand(command):
    if command == 'i':
        con.write("i".encode())
        return waitAnswer('INH')
    elif command == 'o':
        con.write("o".encode())
        return waitAnswer('ON')
    elif command == '1':
        con.write("1".encode())
        return waitAnswer('V1')
    elif command == '0':
        con.write("0".encode())
        return waitAnswer('V0')
    else:
        return 'Command not supported. Possible commands:\n\ti\tInhibit\n\to\tTurn on HV\n\t1\tVSEL 1\n\t0\tVSEL 0\n\n\tq\tQuit shell'

print 'CAEN N126 HV Shell V0.1'
while True:
    x = raw_input("At command: ")
    if x.strip() == 'q':
        break
    reply = sendCommand(x)
    print(reply.strip())

con.close()
