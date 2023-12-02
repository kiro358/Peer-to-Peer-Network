#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BUFLEN 100
#define PEERLENGTH 20
#define CONTENTLENGTH 20
#define ADDRLENGTH 16
#define PORTLENGTH 6
#define MAXPEERS 100
#define MAXCONTENT 100
#define PORT 9002

int main(int argc, char **argv)
{
	//PDU structure
	typedef struct udpPDU
	{
		char type;
		char data[BUFLEN];
	} pdu;
	
	//Peer Structure
	typedef struct peerStruct
	{
		char name [PEERLENGTH];
		char contentList [MAXCONTENT][CONTENTLENGTH];
		char address [ADDRLENGTH];
		char port [PORTLENGTH];
		int numContent;
		int filesSentCount[MAXCONTENT];
	} peer;
	
	//Create an array of peer Structures to act as a database
	peer peerList [MAXPEERS];
	int i, j;
	//Initialize all values in every peer to default
	for (i = 0 ; i < MAXPEERS ; i++)
	{
		memset(peerList[i].name, '\0', sizeof(peerList[i].name));
		memset(peerList[i].address, '\0', sizeof(peerList[i].address));
		memset(peerList[i].port, '\0', sizeof(peerList[i].port));
		peerList[i].numContent = 0;		
		for (j = 0 ; j < MAXCONTENT ; j++)
		{
			memset(peerList[i].contentList[j], '\0', sizeof(peerList[i].contentList[j]));
			peerList[i].filesSentCount[j] = 0;
		}
	}
	
	int numPeers = 0;
	char clientAddrReadable [16];
	ssize_t recvResult;
	
	pdu recvData, response;
	
	//Create the server socket
	int server_socket;
	server_socket = socket(PF_INET, SOCK_DGRAM, 0); // SOCK_DGRAM for UDP socket
	
	if (server_socket < 0)
	{
		printf("There was an error creating the socket...");
		exit(1);
	}
	
	//Define Server and client address
	struct sockaddr_in server_address, client_address;
	socklen_t addrSize;
	memset(&server_address, 0, sizeof(server_address)); // set 0's
	memset(&client_address, 0, sizeof(client_address)); // set 0's
	server_address.sin_family = AF_INET; // Specifies that this is an internet server
	server_address.sin_port = htons(PORT); // Port number of the server
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Address of the local machine
	memset(server_address.sin_zero, '\0', sizeof (server_address.sin_zero)); 
	
	// Bind the socket to our specified IP and port
	int bindResult;
	bindResult = bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));
	if (bindResult < 0)
	{
		printf("There was an error binding the socket...");
		exit(1);
	}
	
	addrSize = sizeof(server_address);
	
	printf("Peer to Peer Network Started and Initialized\nWaiting for Client Requests...\n");
	
	//Start infinite loop waiting for requests from clients
	while (1)
	{
		memset(recvData.data, '\0', sizeof(recvData.data));
		memset(response.data, '\0', sizeof(response.data));
		recvResult = recvfrom(server_socket, (struct udpPDU *) &recvData, sizeof(recvData), 0, (struct sockaddr *) &client_address, &addrSize);
		//Retrieve IP address from request
		inet_ntop(AF_INET, &client_address.sin_addr, (char *)clientAddrReadable, sizeof(clientAddrReadable));
		clientAddrReadable [15] = '\0';
		//Register content with a specific peer name
		if (recvData.type == 'R')
		{
			bool reg = false, nameConflict = false, contentConflict = false;
			char peerName [PEERLENGTH];
			char contentName [CONTENTLENGTH];
			char tempPort [PORTLENGTH];
			memset(peerName, '\0', sizeof(peerName));
			memset(contentName, '\0', sizeof(contentName));
			memset(tempPort, '\0', sizeof(tempPort));
			int i = 0, j = 0, k = 0;

			//Fills the check string with the data received from client, first 20 characters is the peer name, down to 19 as char 20 needs to be \0
			for (i = 0 ; i < PEERLENGTH ; i++)
			{
				if (recvData.data [i] == '\0')
				{
					break;
				}
				peerName [i] = recvData.data [i];
			}
			peerName [i] = '\0';
			
			//Check for peer name conflict
			for (k = 0 ; k < MAXPEERS ; k++)
			{
				if (strcmp(peerName, peerList[k].name) == 0)
				{
					if (strcmp(clientAddrReadable, peerList[k].address) == 0)
					{
						reg = true;
						break;
					}
					else
					{
						nameConflict = true;
						break;
					}
				}
			}
			
			i++;
			
			//Get content from data received from client, it will be chars 20 - 39
			for (i = i ; i < PEERLENGTH + CONTENTLENGTH ; i++)
			{
				if (recvData.data [i] == '\0')
				{
					break;
				}
				contentName [j] = recvData.data [i];
				j++;
			}
			j++;
			contentName [j] = '\0';
			int y, x;
			
			//Check for content name conflict
			for (x = 0 ; x < MAXPEERS ; x++)
			{
				for (y = 0 ; y < MAXCONTENT ; y++)
				{
					if (strcmp(contentName, peerList[x].contentList[y]) == 0)
					{
						contentConflict = true;
						break;
					}
				}
			}
			
			//If peer is not registered
			if (reg == false)
			{
				//get Port number from data received from client, will be chars 40 - 45
				i++;
				j = 0;
				for (i = i ; i < PEERLENGTH + CONTENTLENGTH + PORTLENGTH ; i++)
				{
					if (recvData.data [i] == '\0')
					{
						break;
					}
					tempPort [j] = recvData.data [i];
					j++;
				}
				j++;
				tempPort [j] = '\0';
				printf("Incoming Resgistration Request:\nIP Address: %s\nPeer Name: %s\nContent Name: %s\nPort Number: %s\n", clientAddrReadable, peerName, contentName, tempPort);
				//If name conflict occured
				if (nameConflict == true)
				{
					printf("Error, Peer name already in use, Notifying Client...\n");
					response.type = 'E';
					memset(response.data, '\0', sizeof(response.data));
					strcpy(response.data, "Error, Peer name selected already in use. Please select another peer name...\0");
					sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
				}
				//If content conflict occured
				else if (contentConflict == true)
				{
					printf("Error, Content with This name has alreeady been registered\nNotifying Client...\n");
					response.type = 'E';
					memset(response.data, '\0', sizeof(response.data));
					strcpy(response.data, "Error, Content With that name already Registred, Please Change the name of your content...\0");
					sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
				}
				//No conflicts, register all data, and send A type PDU back
				else
				{
					printf("\nSuccessful Registration!\n\n");
					strcpy(peerList[numPeers].name, peerName);
					strcpy(peerList[numPeers].contentList[peerList[numPeers].numContent], contentName);
					strcpy(peerList[numPeers].address, clientAddrReadable);
					strcpy(peerList[numPeers].port, tempPort);
					response.type = 'A';
					memset(response.data, '\0', sizeof(response.data));
					sprintf(response.data, "Registration with the P2P Network was Successful!");
					sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
					peerList[numPeers].numContent++;
					numPeers++;
				}
			}
			//if already registered, this means registering another piece of content
			else
			{
				//if content name conflict occured
				if (contentConflict == true)
				{
					printf("Error, Content with This name has alreeady been registered\nNotifying Client...\n");
					response.type = 'E';
					memset(response.data, '\0', sizeof(response.data));
					strcpy(response.data, "Error, Content With that name already Registred, Please Change the name of your content...\0");
					sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
				}
				//Register next piece of content with the peer, send back A type PDU
				else
				{
					memset(peerList[k].contentList[peerList[k].numContent], '\0', sizeof(peerList[k].contentList[peerList[k].numContent]));
					strcpy(peerList[k].contentList[peerList[k].numContent], contentName);
					printf("\nNew Content Registration Request From:\nPeer: %s\nIP Address: %s\n", peerList[k].name, peerList[k].address);
					printf("Content Name: %s\n", peerList[k].contentList[peerList[k].numContent]);
					printf("\nNew Content Successfully Registered!\n\n");
					response.type = 'A';
					memset(response.data, '\0', sizeof(response.data));
					sprintf(response.data, "New Content Successfully Registered!\nContent Name: %s", peerList[k].contentList[peerList[k].numContent]);
					sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
					peerList[k].numContent++;
				}
			}
		}
		//Download Request
		else if (recvData.type == 'D')
		{
			printf("Incoming Content Search Request:\n");
			printf("Content Name: %s\n", recvData.data);

			int i, j, aLen, pLen;
			bool foundContent = false;
			int minSentFiles = MAXCONTENT;  // Initialize with a large value
			int selectedPeerIndex = -1;

			// Find peer hosting the content
			for (i = 0; i < numPeers; i++) {
				int sentFilesCount = 0;  // Count of files sent by this peer

				for (j = 0; j < MAXCONTENT; j++) {  // Assuming MAXCONTENT is the maximum number of content a peer can have
					if (strcmp(recvData.data, peerList[i].contentList[j]) == 0) {
						// Found the content in the peer's content list
						foundContent = true;
						sentFilesCount = peerList[i].filesSentCount[j];
						break;
					}
				}

				// Check if this peer has sent this file fewer times than the current minimum
				if (foundContent && sentFilesCount < minSentFiles) {
					minSentFiles = sentFilesCount;
					selectedPeerIndex = i;
				}
			}

			// If the content is found on the network, send peer details to the client
			if (foundContent && selectedPeerIndex != -1) {
				printf("\nContent Successfully Found:\n");
				printf("Hosting Peer Details:\n");
				printf("Peer Name: %s\n", peerList[selectedPeerIndex].name);
				printf("Peer IP Address: %s\n", peerList[selectedPeerIndex].address);
				printf("Peer Port Number: %s\n", peerList[selectedPeerIndex].port);

				// Update the count of files sent by the selected peer for the specific content
				int contentIndex;
				for (contentIndex = 0; contentIndex < MAXCONTENT; contentIndex++) {
					if (strcmp(recvData.data, peerList[selectedPeerIndex].contentList[contentIndex]) == 0) {
						// Found the content in the peer's content list
						peerList[selectedPeerIndex].filesSentCount[contentIndex]++;
						break;
					}
				}

				char contentDel[CONTENTLENGTH];
				strcpy(contentDel, recvData.data);
				response.type = 'D';
				aLen = strlen(peerList[selectedPeerIndex].address);
				aLen++;
				pLen = strlen(peerList[selectedPeerIndex].port);
				pLen++;
				memcpy(response.data, peerList[selectedPeerIndex].address, aLen);
				memcpy(response.data + aLen, peerList[selectedPeerIndex].port, pLen);
				sendto(server_socket, (struct udpPDU *)&response, sizeof(response), 0, (struct sockaddr *)&client_address, addrSize);
				printf("\nHosting Peer Details Successfully Sent to Client!\n");
				memset(response.data, '\0', sizeof(response.data));
				memset(recvData.data, '\0', sizeof(recvData.data));
				recvResult = recvfrom(server_socket, (struct udpPDU *)&recvData, sizeof(recvData), 0, (struct sockaddr *)&client_address, &addrSize);
				//Waiting for confirmation of successful download, if successful, register the content under the client and de-register it from the original peer
				if (recvData.type == 'T')
				{
					for (j = 0 ; j < peerList[i].numContent ; j++)
					{
						if (strcmp(contentDel, peerList[i].contentList[j]) == 0)
						{
							if (j == MAXCONTENT - 1)
							{
								memset(peerList[i].contentList[j], '\0', sizeof(peerList[i].contentList[j]));
							}
							else
							{
								for (int k = j ; k < MAXCONTENT ; k++)
								{
									memset(peerList[i].contentList[k], '\0', sizeof(peerList[i].contentList[k]));
									strcpy(peerList[i].contentList[k], peerList[i].contentList[k + 1]);
								}
							}
							break;
						}
					}
					peerList[i].numContent--;
					for (i = 0 ; i < numPeers ; i++)
					{
						if (strcmp(clientAddrReadable, peerList[i].address) == 0)
						{
							strcpy(peerList[i].contentList[peerList[i].numContent], contentDel);
							peerList[i].numContent++;
							break;
						}
					}
				}
			}
			//If content doesnt exist
			else
			{
				response.type = 'E';
				strcpy(response.data, "Error! Content Does Not Exist!\0");
				sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
			}
		}
		//Deregistration
		else if (recvData.type == 'T')
		{
			memset(response.data, '\0', sizeof(response.data));
			char contentT[CONTENTLENGTH];
			strcpy(contentT, recvData.data);
			bool test = false;
			int i;
			//Test if client is registered
			for (i = 0 ; i < MAXPEERS ; i++)
			{
				if (strcmp(clientAddrReadable, peerList[i].address) == 0)
				{
					test = true;
					break;
				}
			}
			//If not registered
			if (test == false)
			{
				response.type = 'E';
				strcpy(response.data, "Error! You are not registered with the network!\0");
				sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
			}
			//If registered
			else
			{
				printf("De-Registration Request:\n");
				printf("Peer: %s\n", peerList[i].name);
				printf("IP Address: %s\n", peerList[i].address);
				printf("Content: %s\n", contentT);
				int j, k;
				//Find content in database, delete it, and rebuild database
				for (j = 0 ; j < peerList[i].numContent ; j++)
				{
					if (strcmp(contentT, peerList[i].contentList[j]) == 0)
					{
						if (j == MAXCONTENT - 1)
						{
							memset(peerList[i].contentList[j], '\0', sizeof(peerList[i].contentList[j]));
						}
						else
						{
							for (k = j ; k < MAXCONTENT ; k++)
							{
								memset(peerList[i].contentList[k], '\0', sizeof(peerList[i].contentList[k]));
								strcpy(peerList[i].contentList[k], peerList[i].contentList[k + 1]);
							}
						}
						break;
					}
				}
				peerList[i].numContent--;
				printf("\nSuccessfully De-Registered Content!\n\n");
				//Send T type PDU to client to confirm DeRegistration
				response.type = 'T';
				strcpy(response.data, "Successfully De-Registered Content!");
				sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
				//If number of content registered under peer is now 0, perform a quit
				if (peerList[i].numContent == 0)
				{
					printf("Peer Quit Request:\n");
					printf("Peer Name: %s\n", peerList[i].name);
					printf("IP Address: %s\n", peerList[i].address);
					//Delete all data about the peer from the database, then rebuild database if necessary
					memset(response.data, '\0', sizeof(response.data));
					if (i == MAXPEERS - 1)
					{
						memset(peerList[i].address, '\0', sizeof(peerList[i].address));
						memset(peerList[i].name, '\0', sizeof(peerList[i].name));
						memset(peerList[i].port, '\0', sizeof(peerList[i].port));
						for (j = 0 ; j < peerList[i].numContent ; j++)
						{
							memset(peerList[i].contentList[j], '\0', sizeof(peerList[i].contentList[j]));
						}
					}
					else
					{
						j = 0;
						k = 0;
						for (j = i ; j < numPeers ; j++)
						{
							memset(peerList[j].address, '\0', sizeof(peerList[j].address));
							strcpy(peerList[j].address, peerList[j + 1].address);
							memset(peerList[j].name, '\0', sizeof(peerList[j].name));
							strcpy(peerList[j].name, peerList[j + 1].name);
							memset(peerList[j].port, '\0', sizeof(peerList[j].port));
							strcpy(peerList[j].name, peerList[j + 1].name);
							for (k = 0 ; k < MAXCONTENT ; k++)
							{
								memset(peerList[j].contentList[k], '\0', sizeof(peerList[i].contentList[k]));
								strcpy(peerList[i].contentList[k], peerList[i].contentList[k + 1]);
							}
							peerList[j].numContent = peerList[j + 1].numContent;
						}
					}
					//Send confirming quit notification
					printf("\nPeer Successfully De-Registered from the Network!\n\n");
					numPeers--;
					memset(response.data, '\0', sizeof(response.data));
					strcpy(response.data, "You are de-registered from the P2P Network\0");
					sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
				}
			}
		}
		//Content listing request
		else if (recvData.type == 'O')
		{
			printf("Incoming Content Listing Request:\n");
			printf("IP Address: %s\n", clientAddrReadable);
			bool test = false;
			//If no content on the network
			if (peerList[0].contentList[0][0] == '\0')
			{
				response.type = 'F';
				memset(response.data, '\0', sizeof(response.data));
				sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
				test = true;
				printf("No Content On The Network...Error Sent to Client...\n\n");
			}
			//if content exists on the network
			if (test == false)
			{
				int i = 0, j = 0, bytes = 0, contentSize = 0;
				bool newPacket = false;
				memset(response.data, '\0', sizeof(response.data));
				//Dynamically fill packets to the max size of 100 bytes, send O type PDUs with content names, last PDU is sent with an F type to indicate final transmission
				for (i = 0 ; i < MAXPEERS ; i++)
				{
					for (j = 0 ; j < MAXCONTENT ; j++)
					{
						if (peerList[i].contentList[j][0] == '\0')
						{
							break;
						}
						contentSize = strlen(peerList[i].contentList[j]);
						contentSize++;
						bytes = bytes + contentSize;
						
						if ((100 - bytes) >= 0)
						{
							if (newPacket == false)
							{
								memcpy(response.data, peerList[i].contentList[j], contentSize);
								newPacket = true;
							}
							else
							{
								memcpy(response.data + bytes - contentSize, peerList[i].contentList[j], contentSize);
							}
						}
						else
						{
							response.type = 'O';
							sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
							memset(response.data, '\0', sizeof(response.data));
							memcpy(response.data, peerList[i].contentList[j], contentSize);
							bytes = contentSize;
						}
					}
				}
				if (response.data[0] != '\0')
				{
					response.type = 'F';
					sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
				}
				printf("Content List Sent To Client Successfully!\n\n");
			}
		}
		//Content Consistency check
		else if (recvData.type == 'H')
		{
			int i, j;
			bool contCheck = false;
			//If content does not match the record on the database
			response.type = 'E';
			strcpy(response.data, "Error, Content Does not match...\0");
			//Check content in the PDU received to see if it exists on the database
			for (i = 0 ; i < numPeers ; i++)
			{
				if (strcmp(clientAddrReadable, peerList[i].address) == 0)
				{
					for (j = 0 ; j < peerList[i].numContent ; j++)
					{
						if (strcmp(peerList[i].contentList[j], recvData.data) == 0)
						{
							//if content is found
							response.type = 'H';
							memset(response.data, '\0', sizeof(response.data));
							strcpy(response.data, "Content Matches!\0");
							contCheck = true;
							break;
						}
					}
				}
				if (contCheck == true)
				{
					break;
				}
			}
			sendto(server_socket, (struct udpPDU *) &response, sizeof(response), 0, (struct sockaddr*) &client_address, addrSize);
		}
	}
	
	return 0;
}

