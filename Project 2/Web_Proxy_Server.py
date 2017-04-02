#TCD 3D3 project 2
#Created by Hugh Lavery 14313812
#On 15/3/17

#modules to import
import socket, sys, thread, os, ssl, threading, hashlib, os.path, time

#Setting for port
try:
    portNum = 8001
except KeyboardInterrupt:
     print "Closing proxy server\n"
     sys.exit()

#Global variables
maxConnections = 5 #number of connections server is allowing
bufferSize = 8192 # maybe should be bigger


def start():
    try:
        #Opening the socket to accept incoming connections
        proxsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #sslproxysocket = ssl.wrap_socket(proxsocket)
        proxsocket.bind(('', portNum))
        proxsocket.listen(maxConnections)
        print "Socket is binded on port ", (portNum), "\n"
        print "Server is accepting connections"

    except socket.error, (value, message):
        #in case socket is uable to bind
        if proxsocket:
            proxsocket.close()
        print "Socket bind unsucessfull"
        sys.exit(1)

    #deal with incoming connections
    while 1:
        try:
            #recieve request string from browser and start new thread
            #Calling the browserRequest function
            conn, addr = proxsocket.accept()
            clientData = conn.recv(bufferSize)
            #start_new_thread(browserRequest, (conn, (conn, clientData, addr))
            
           
            if (len(clientData) > 0):
                print "starting thread\n"
                #print ", browser request is '%s' \n", clientData"
                threadt = threading.Thread(target=browserRequest, args=(conn, clientData, addr, proxsocket))
                #threadg = thread.start_new_thread(browserRequest, (conn, clientData, addr, proxsocket))
                threadt.setDaemon(True)
                threadt.start()
                
                print "thread started\n"
        except KeyboardInterrupt:
            #if user wants to to close server
            proxsocket.close()
            print "Closing down server \n"
            sys.exit(2)
    proxsocket.close()


def browserRequest(conn, data, address, proxsocket):
    try:
        if (len(data) == 0):
            print "No data. Closing thread\n"
            return
        #if first word is connect it is a https connection and deal with 
        request = data.split('\n')[0]
        prefix = request.split(' ')[0]
        url = request.split(' ')[1]
        if (prefix == "CONNECT"):
            reply = "HTTP/1.1 400 Bad Request\r\n"
            conn.send(reply)
            print "Https request so ignoring\n"
            print "url %s\n" %url
            return
        print " url: '%s' \n " %url
        key = hashlib.md5(data)
        key = key.hexdigest()
        key += ".txt"
        print "key '%s'\n" %key
        print "prefix: '%s' \n" %prefix
        if os.path.isfile(key):
            t=open(key, "r")
            print "File is in cache. Sending data to browser\n"
            cachedData = t.read()
            reply = "HTTP/1.1 200 Connection established\r\n"
            conn.send (reply)
            conn.send(cachedData)
            print "data sent to browser \n"
            return
        else:
            print "file NOT is cache\n"
        http_pos = url.find("://") #change names
        if(http_pos == -1):
            temp = url
        else:
            temp = url[(http_pos+3):]
                
        port_pos = temp.find(":")
            
        webserver_pos = temp.find ("/")
        if webserver_pos ==-1:
            webserver_pos = len(temp)
        webserver = ''
        port = -1
        if ((port_pos == -1) or (webserver_pos < port_pos)):
            port = 80 # default port
            if(prefix == "CONNECT"):
                port = 443
            webserver = temp[:webserver_pos]
        else:
            #specific port
            port = int((temp[(port_pos+1):])[:webserver_pos-port_pos-1])
            webserver = temp[:port_pos]
        print "calling proxy. Port is %d \n" %port
        proxy(webserver, port, conn, address, data, prefix, proxsocket, key)
        print "proxy returned\n"
        return
    except KeyboardInterrupt:
        print "closing proxy"
        return

def proxy(webserver, port, conn, addr, clientData, prefix, proxsocket, key):
    try:
        
        if (prefix == "CONNECT"):
            #thread.start_new_thread(httpsConnection, (conn, clientData, addr, proxsocket, websocket, conn2))
            return
        else:
            print "opening socket\n"
            websocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            websocket.connect((webserver,port))
            websocket.send((clientData))
        print "entering while, connection is http\n"
        temp = open(key, "a+")
        t1 = time.time()
        t2 = time.time()
        while (t1 + 3 > t2):
            
            webreply = ""
            webreply = websocket.recv(bufferSize)
            
            if (len(webreply) > 0):
                #print "webreply is %s" %webreply
                temp.write(webreply)
                #pos = temp.tell()
                print "added to cache\n"
                
                #send reply to clientData
                if (prefix == "CONNECT"):
                    #print "attempting to send ssl to browser\n"
                    #conn.send("HTTP/1.1 200 Connection established\r\n\r\n")
                    #conn.sendall(webreply)
                    #conn.sendall(webreply)
                    print "send ssl back to client\n"
                else:
                    conn.sendall(webreply)
                print "reply sent to client \n"
                #not sure what dar is
                dar = 0
                dar = float(len(webreply))
                dar = float(dar/1024)
                dar = "%.3s" % (str(dar))
                dar = "%s KB" % (dar)
                'req complete'
                print 'Request done: %s => %s <=' % (str(addr[0]), str(dar))
                t2 = time.time()
        return        
    except socket.error, (value, message):
        #socket error
        websocket.close()
        conn.close()
        sys.exit(1)
        
def httpsConnection(conn, clientData, addr, proxsocket, websocket) :
    try:
            #websocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            #websocket = ssl.wrap_socket(websocket)
            #websocket.create_connection((webserver,port)) #webserver ? instead of host
            #websocket.connect(webserver,port)
            #websocket.bind(webserver,port)
            #conn2, add2 = websocket.accept()
            #conn2.send(clientData)
            #reply = "HTTP/1.1 200 Connection established\r\n"
            #reply += "Proxy-connection: Keep-alive\r\n"
            #reply += "\r\n"
            #conn.sendall(reply)
        print "connection is https \n"
        while 1:
            webreply = conn2.recv(bufferSize)
            browserreply = conn.recv(bufferSize)

            if (len(webreply) > 0):
                conn.sendall(webreply)
                print "sending web reply to https browser\n"
                dar = 0
                dar = float(len(webreply))
                dar = float(dar/1024)
                dar = "%.3s" % (str(dar))
                dar = "%s KB" % (dar)
                print 'Https Request done: %s => %s <=' % (str(addr[0]), str(dar))

            if (len(browserreply) > 0):
                conn2.sendall(browserreply)
                print "sending browser reply to https socket\n"

    except socket.error, (value, message):
        #socket error
        websocket.close()
        conn.close()
        sys.exit(1)




start()
