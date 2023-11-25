// index.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFLEN		255	/* buffer length */

int s2;  // Socket descriptor
int udpSocket;

// Define a structure to represent registered content
struct RegisteredContent{
    char peerName[10];
    char contentName[10];
    struct sockaddr_in contentServerAddress;
};

#define MAX_REGISTERED_CONTENT 100

// Maintain an array to store registered content
struct RegisteredContent registeredContent[MAX_REGISTERED_CONTENT];

// Keep track of the current number of registered content entries
int numOfRegisteredContent = 0;

struct sockaddr_in reg_addr;

 struct RegistrationPDU {
    char type;  // 'R'
    char peerName[10];
    char contentName[10];
    struct sockaddr_in contentServerAddress;
};

struct AcknowledgementPDU {
    char type;  // 'A'
    // char peerName[10];
    char contentName[10];
    struct sockaddr_in contentServerAddress;
};

struct SearchPDU {
    char type;  // 'S'
    char peerName[10];
    char contentName[10];
};

struct SearchResponsePDU {
    char type;  // 'S'
    struct sockaddr_in contentServerAddress;
};

struct ListRequestPDU {
    char type;  // 'O'
    // Additional fields if needed
};

struct ContentListPDU {
    char type;  // 'O'
    char peerName[10];
    char contentName[10];
};

struct DeregistrationPDU {
    char type;  // 'T'
    char peerName[10];
    char contentName[10];
};

struct ContentDataPDU {
    char type;  // 'C'
    char peerName[10];
    char contentName[10];
    struct sockaddr_in contentServerAddress;
    // This PDU may be used for transferring content data over TCP
};

int isContentRegistered(const char *peerName, const char *contentName) {
    for (int i = 0; i < numOfRegisteredContent; i++) {
        if (strcmp(registeredContent[i].peerName, peerName) == 0 &&
            strcmp(registeredContent[i].contentName, contentName) == 0) {
            // Conflict detected
            return 1;
        }
    }
    // No conflict
    return 0;
}

int addContent(const char *peerName, const char *contentName, const struct sockaddr_in *contentServerAddress){
    if (numOfRegisteredContent<MAX_REGISTERED_CONTENT){
        strcpy(registeredContent[numOfRegisteredContent].peerName, peerName);
        strcpy(registeredContent[numOfRegisteredContent].contentName, contentName);
        memcpy(&registeredContent[numOfRegisteredContent].contentServerAddress, contentServerAddress, sizeof(struct sockaddr_in));
        numOfRegisteredContent++;
        return 0;
    }
    else {
        // Handle error: The array is full
        fprintf(stderr, "Error adding the content, please try again after deregistering content");
        return -1;
    }
}

int removeContent(const char *peerName, const char *contentName) {
    for (int i = 0; i < numOfRegisteredContent; i++) {
        if (strcmp(registeredContent[i].peerName, peerName) == 0 &&
            strcmp(registeredContent[i].contentName, contentName) == 0) {
            // Shift elements to overwrite the entry to be removed
            for (int j = i; j < numOfRegisteredContent - 1; j++) {
                strcpy(registeredContent[j].peerName, registeredContent[j + 1].peerName);
                strcpy(registeredContent[j].contentName, registeredContent[j + 1].contentName);
                memcpy(&registeredContent[j].contentServerAddress, &registeredContent[j + 1].contentServerAddress, sizeof(struct sockaddr_in));
            }
            numOfRegisteredContent--;
            return 0; // Found and removed, exit the function
        }
    }
    // If the function reaches here, the content was not found
    fprintf(stderr, "Content not found for deregistration: Peer: %s, Content: %s\n", peerName, contentName);
    return -1;
}

void handleContentRegistration(int udpSocket) {
    // Implementation of content registration handling
    struct RegistrationPDU registrationPDU;
    char peerName[10];
    char contentName[10];
    struct sockaddr_in contentServerAddress;
    recvfrom(udpSocket, &registrationPDU, sizeof(registrationPDU), 0, NULL, NULL);

    // Check for conflicts, update data structures, and send acknowledgment (A-type PDU) or error (E-type PDU)
    strcpy(peerName,registrationPDU.peerName);
    strcpy(contentName,registrationPDU.contentName);
    memcpy(&contentServerAddress,&registrationPDU.contentServerAddress, sizeof(struct sockaddr_in));
    
    if (isContentRegistered(peerName, contentName)) {
        // Conflict detected, send an error (E-type PDU)
        struct AcknowledgementPDU errorPDU;
        errorPDU.type = 'E';
        sendto(udpSocket, &errorPDU, sizeof(errorPDU), 0, NULL, 0);
    }
    
    int added = addContent(peerName, contentName, &contentServerAddress);


    // Send acknowledgment (A-type PDU)
    struct AcknowledgementPDU acknowledgementPDU;
    acknowledgementPDU.type = 'A';
    sendto(udpSocket, &acknowledgementPDU, sizeof(acknowledgementPDU), 0, NULL, 0);
}

void handleContentSearch(int udpSocket) {
    // Implementation of content search handling
    // Assume searchPDU is properly initialized
    struct SearchPDU searchPDU;
    recvfrom(udpSocket, &searchPDU, sizeof(searchPDU), 0, NULL, NULL);

    // Check for content, update data structures, and send response (S-type PDU or E-type PDU)
    // ...

    // Example: Send response (S-type PDU)
    struct SearchResponsePDU searchResponsePDU;
    searchResponsePDU.type = 'S';
    // Fill searchResponsePDU with content server address
    sendto(udpSocket, &searchResponsePDU, sizeof(searchResponsePDU), 0, NULL, 0);
}

void handleContentListing(int udpSocket) {
    // Implementation of content listing handling
    struct ListRequestPDU listRequestPDU;
    recvfrom(udpSocket, &listRequestPDU, sizeof(listRequestPDU), 0, NULL, NULL);

    // Prepare list of registered contents and send response (O-type PDU)

    // Send response (O-type PDU)
    struct ContentListPDU contentListPDU;
    contentListPDU.type = 'O';
    // Fill contentListPDU with list of registered contents
    for (int i = 0; i < numOfRegisteredContent; i++) {
        // Add each registered content entry to the PDU
        strcpy(contentListPDU.peerName, registeredContent[i].peerName);
        strcpy(contentListPDU.contentName, registeredContent[i].contentName);
    }
    sendto(udpSocket, &contentListPDU, sizeof(contentListPDU), 0, NULL, 0);
}

void handleContentDeregistration(int udpSocket) {
    
    struct DeregistrationPDU deregistrationPDU;
    recvfrom(udpSocket, &deregistrationPDU, sizeof(deregistrationPDU), 0, NULL, NULL);

    // Update data structures, send acknowledgment (A-type PDU), or error (E-type PDU)
    int removed = removeContent(deregistrationPDU.peerName, deregistrationPDU.contentName);
    if (removed ==0){
        // Send acknowledgment (A-type PDU)
        struct AcknowledgementPDU acknowledgementPDU;
        acknowledgementPDU.type = 'A';
        sendto(udpSocket, &acknowledgementPDU, sizeof(acknowledgementPDU), 0, NULL, 0);
    }
    else {
        // Send error (E-type PDU)

    }
}

int main() {
    
    // Initialize UDP socket for communication with peers (s2)  
    struct sockaddr_in myAddress;
    // Create UDP socket
    if ((s2 = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Error creating UDP socket");
        exit(EXIT_FAILURE);
    }

    // Set up the address structure
    memset(&myAddress, 0, sizeof(myAddress));
    myAddress.sin_family = AF_INET;
    myAddress.sin_port = htons(0);
    myAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(s2, (struct sockaddr *)&myAddress, sizeof(myAddress)) == -1) {
        perror("Error binding UDP socket");
        close(s2);  // Close the socket before exiting
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Wait for incoming PDUs and dispatch to appropriate handlers

        // Example: Dispatch based on PDU type
        char pduType;
        recvfrom(s2, &pduType, sizeof(pduType), 0, NULL, NULL);
   
        switch (pduType) {
            case 'R':
                handleContentRegistration(s2);
                break;
            case 'S':
                handleContentSearch(s2);
                break;
            case 'O':
                handleContentListing(s2);
                break;
            case 'T':
                handleContentDeregistration(s2);
                break;
            // Handle other PDU types as needed
            // ...
            default:
                // Handle unknown PDU type or ignore
                break;
        }
    }

    // Close the UDP socket
    close(s2);

    return 0;
}