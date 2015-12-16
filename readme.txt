The zip contains the following folder and file

-programs
	client.c
	client_UDP.c
	makefile
	server.c
	server_UDP.c
-WriteUpSupportingFiles
	big3.txt
	nonPersistentResult.txt
	nonPersistentResult1File.txt
	nonTcpPersistent1File.pcap
	persistenResult1file.txt
	persistentResult.txt
	tcpNonPersistent.pcap
	tcpPersistent.pcap
	tcpPersistent1File.pcap
	udpResult.txt
-WriteUp.docx

Steps to be followed for execution

1)	Open teminal. Navigate to the program folder. 
2)	type "make" and press enter
3)	Four executable will be created. client & server for TCP & UDP
4)	Arguments required for server(TCP and UDP)
	arg1 - port number
5)	Argument required for client TCP
	arg1 - host name
	arg2 - port number
	arg3 - connection type(np - non persistent, p - persistent)
	arg4 - file name
6)	Argument required for client UDP
	arg1 - host name
	arg2 - port number
	arg3 - file name

NOTE : The TCP client persistence connection will have a timeout period of 10seconds. So after the file is transfered the client will take 10 seconds for the connection to get closed.
