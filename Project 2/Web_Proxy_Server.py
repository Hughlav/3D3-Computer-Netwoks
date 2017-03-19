#TCD 3D3 project 2
#Created by Hugh Lavery 14313812
#On 15/3/17

#modules to import
import socket, sys
from thread import *


#Setting for port
try:
    portNum = int(raw_input("Please enter port number: \n"))
except KeyboardInterrupt:
     print "Closing proxy server\n"
     sys.exit()

#Global variables
maxConnections = 5 #number of connections server is allowing
bufferSize = 5120 # maybe should be bigger


def start():
    try:
        
        #Opening the socket to accept incoming connections
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.bind(('', portNum))
        sock.listen(maxConnections)
        print "Socket is binded on port [%d] \n" (portNum)
        print "Server is accepting connections"

    except Exception, e:
        #in case socket is uable to bind
        print "Socket bind unsucessfull"
        sys.exit(2)

    #deal with incoming connections
    while 1:
        try:
            #recieve request string from browser and start new thread
            #Calling the browserRequest function
            conn, addr = sock.accept()
            clientData = conn.recv(bufferSize)
            #start_new_thread(browserRequest, (conn, (conn, clientData, addr))
            threading.Thread(browserRequest, args=(conn, clientData, addr), kwargs={keyword: 'args'}).start()
            
        except KeyboardInterrupt:
            #if user wants to to close server
            sock.close()
            print "Closing down server \n"
            sys.exit(1)
    sock.close()


def browserRequest(connection, data, address):
    try:
        url_line = data.split('\n') #[0]
        url = url_line.split(' ') #[1]

    except Exception, e:
        pass


start()
