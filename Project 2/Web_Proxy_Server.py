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
maxConnections = 10 #number of connections server is allowing
bufferSize = 12800 # maybe should be bigger


def start():
    try:
        #Opening the socket to accept incoming connections
        proxsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #sslproxysocket = ssl.wrap_socket(proxsocket)
        proxsocket.bind(('', portNum))
        proxsocket.listen(maxConnections)
        print "Socket is binded on port ", (portNum), "\n"
        print "Server is accepting connections"
        threadA = threading.Thread(target = blocking, args=())
        threadA.setDaemon(True)
        threadA.start()
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
                
                threadt = threading.Thread(target=browserRequest, args=(conn, clientData, addr, proxsocket))
                threadt.setDaemon(True)
                threadt.start()
                
        except KeyboardInterrupt:
            #if user wants to to close server
            proxsocket.close()
            print "Closing down server \n"
            sys.exit(2)
    proxsocket.close()


def browserRequest(conn, data, address, proxsocket):
    try:
        if (len(data) == 0):
            #Nothing in request so close thread
            return
        
        request = data.split('\n')[0]
        prefix = request.split(' ')[0]
        url = request.split(' ')[1]
        #if first word is connect it is a https connection and deal with 
        if (prefix == "CONNECT"):
            #Call function to deal with https data
            httpsConnection(conn, data, address, proxsocket, url)
            print "https returned\n"
            return
        
        #Hash the request header (using md5 algorithm) to create a key
        #that is used to cache the html data returned from the webserver
        key = hashlib.md5(data)
        key = key.hexdigest()
        key += ".txt"
        blocked = blockedURLS(url)
        #Check to see if URL is on blacklist
        if (blocked == True):
            print "URL BLOCKED\n"
            reply = "HTTP/1.1 400 Bad Request\r\n"
            conn.send(reply)
            return
        #Check if there is cached data for the request
        if os.path.isfile(key):
            t=open(key, "r")
            print "Cache hit\n"
            cachedData = t.read()
            reply = "HTTP/1.1 200 Connection established\r\n"
            #Send cached data to browser
            conn.send (reply)
            conn.send(cachedData)
            #print "data sent to browser\n"
            return
        else:
            print "Cache Miss\n"
            print "file NOT is cache\n"
        #Find position where url starts
        http_index = url.find("://") 
        #get rid of http://
        if(http_index == -1):
            temp = url
        else:
            temp = url[(http_index+3):]    
        port_index = temp.find(":")
        webserver_index = temp.find ("/")
        #Find declared port or use default (Port 80)
        #Find webserver
        if webserver_index ==-1:
            webserver_index = len(temp)
        webserver = ''
        port = -1
        if ((port_index == -1) or (webserver_index < port_index)):
            port = 80 # 
            webserver = temp[:webserver_index]
        else:
            #specific port
            port = int((temp[(port_index+1):])[:webserver_index-port_index-1])
            webserver = temp[:port_index]
        #print "calling proxy. Port is %d \n" %port
        proxy(webserver, port, conn, address, data, prefix, proxsocket, key)
        #print "proxy returned\n"
        return
    except KeyboardInterrupt:
        print "closing proxy"
        return

def proxy(webserver, port, conn, addr, clientData, prefix, proxsocket, key):
    try: 
        #print "opening socket\n"
        websocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        websocket.connect((webserver,port))
        websocket.send((clientData))
        websocket.settimeout(5)
        #print "entering while, connection is http\n"
        temp = open(key, "a+")
        
        while (1):
            #check next request and maybe call browser request for other gets
            
            webreply = ""
            webreply = websocket.recv(bufferSize)
            
            if (len(webreply) > 0):
                
                temp.write(webreply)
                conn.send(webreply)
                
                dar = 0
                dar = float(len(webreply))
                dar = float(dar/1024)
                dar = "%.3s" % (str(dar))
                dar = "%s KB" % (dar)
                #print 'Request done: %s => %s <=' % (str(addr[0]), str(dar))
            
        return
    except socket.timeout:
        return            
    except socket.error, (value, message):
        #socket error
        websocket.close()
        conn.close()
        sys.exit(1)
        
def httpsConnection(conn, clientData, addr, proxsocket, url) :
    try:
        print "https\n"
        #Find position where url starts
        http_index = url.find("://") 
        #get rid of http://
        if(http_index == -1):
            temp = url
        else:
            temp = url[(http_index+3):]    
        port_index = temp.find(":")
        webserver_index = temp.find ("/")
        #Find declared port or use default (Port 80)
        #Find webserver
        if webserver_index ==-1:
            webserver_index = len(temp)
        webserver = ''
        port = -1
        if ((port_index == -1) or (webserver_index < port_index)):
            port = 443 
            webserver = temp[:webserver_index]
        else:
            #specific port
            port = int((temp[(port_index+1):])[:webserver_index-port_index-1])
            webserver = temp[:port_index]
        #copied to here from above

        print "port found\n"


        websocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            #websocket = ssl.wrap_socket(websocket)
            #websocket.create_connection((webserver,port)) #webserver ? instead of host
        #websocket.bind((webserver,port))
        websocket.connect((webserver,port))
        websocket.send(clientData)
        #websocket.listen(maxConnections)
        websocket.settimeout(5)
        reply = "HTTP/1.1 200 Connection established\r\n"
        reply += "Proxy-Agent: ProxyServer/1.0\r\n"
        reply += "\r\n"

        conn.send(reply)
        print "Preparing to ferry ssl data"
        threadTEMP = threading.Thread(target =tempfunct, args=(websocket, conn))
        threadTEMP.setDaemon(True)
        threadTEMP.start()
        print "here1\n"
        i =0
        while 1:

                webreply = websocket.recv(bufferSize)
                if (len(webreply) > 0):
                    print "here2\n"
                    conn.send(webreply)
                    print "webreply: %s\n" %webreply
                    print "sending web reply to https browser\n"
                    dar = 0
                    dar = float(len(webreply))
                    dar = float(dar/1024)
                    dar = "%.3s" % (str(dar))
                    dar = "%s KB" % (dar)
                    print 'Https Request done: %s => %s <=' % (str(addr[0]), str(dar))
                #if (i ==0):
                    #threadTEMP.start()
                #i = i+1

    except socket.timeout:
        print "timeout https\n"
        return            
    except socket.error, (value, message):
        #socket error
        websocket.close()
        conn.close()
        sys.exit(1)
 
def tempfunct(websocket, conn):
    try:
        print "here3\n"
        #while 1:
        browserreply = conn.recv(bufferSize)
        if (len(browserreply) > 0):
            print "here4\n"
            print "browserreply %s\n" %browserreply
            websocket.send(browserreply)
            print "sending browser reply to https socket\n"
    except socket.timeout:
        print "timeout https\n"
        return 
            

    


def blockedURLS(url):
    if os.path.isfile('blocked.txt'):
        with open('blocked.txt') as temp:
            length = sum(1 for line in 'blocked.txt') 
            length = length + 1
            while length > 0:
                urlBlocked = temp.readline()
                
                #urlBlocked = [x.strip('\n') for x in urlBlocked]
                urlBlocked = urlBlocked.strip('\n')
                print "urlBlocked: '%s'\n" %urlBlocked
                print "url: '%s'\n" %url
                if len(urlBlocked) > 3:
                    if (urlBlocked in url):
                        return True
                length = length - 1
        print "URL not blocked\n"
        return False 

def blocking():

    while(1):
         t = raw_input('URL to be blocked:')
         temp = open("blocked.txt", "a+")
         if 'www.' in t:
             t = t.strip('www.')
         if '.com' in t:
             t = t.strip('.com')
         t += "\n"
         if len(t) > 3:
            print "Blocking %s\n" %t
            temp.write(t)
         temp.close()
start()
