/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions. (Revised 2020)
 *
 *  Starter code template
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node( Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = new Member;
    this->shouldDeleteMember = true;
	memberNode->inited = false;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member* member, Params *params, EmulNet *emul, Log *log, Address *address) {
    for( int i = 0; i < 6; i++ ) {
        NULLADDR[i] = 0;
    }
    this->memberNode = member;
    this->shouldDeleteMember = false;
    this->emulNet = emul;
    this->log = log;
    this->par = params;
    this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {
    if (shouldDeleteMember) {
        delete this->memberNode;
    }
}

/**
* FUNCTION NAME: recvLoop
*
* DESCRIPTION: This function receives message from the network and pushes into the queue
*                 This function is called by a node to receive messages currently waiting for it
*/
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
* FUNCTION NAME: nodeStart
*
* DESCRIPTION: This function bootstraps the node
*                 All initializations routines for a member.
*                 Called by the application layer.
*/
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
    /*
    * This function is partially implemented and may require changes
    */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
	// node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
	initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == strcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        /*
        Original
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));
        */
        size_t msgsize = sizeof(MessageHdr);
        msg = new MessageHdr();
        msg->msgType = JOINREQ;
        msg->address = &memberNode->addr;

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
* FUNCTION NAME: finishUpThisNode
*
* DESCRIPTION: Wind up this node and clean up state
*/
int MP1Node::finishUpThisNode(){
    /*
     * Your code goes here
     */
}

/**
* FUNCTION NAME: nodeLoop
*
* DESCRIPTION: Executed periodically at each member
*                 Check your messages in queue and perform membership protocol duties
*/
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 * 
 * introducer reply to a JOINREQ with a JOINREP message
 */
bool MP1Node::recvCallBack(void *env, char *data, int size) {
    /*
    * Your code goes here
    */
    MessageHdr* msg = (MessageHdr*) data;
    
    if (msg->msgType == JOINREQ) {

        int id;
        short port;
        int heartbeat = 1;
        memcpy(&id, &msg->address->addr[0], sizeof(int));
        memcpy(&port, &msg->address->addr[sizeof(int)], sizeof(short));

        addMember(id, port, heartbeat);
        logAddress(id, port, true);

        Address* sending = msg->address;
        emulNet->ENsend( &memberNode->addr, sending, (char*)msg, sizeof(MessageHdr));   
        

    } else if (msg->msgType == JOINREP){

        int id;
        short port;
        int heartbeat = 1;
        memcpy(&id, &msg->address->addr[0], sizeof(int));
        memcpy(&port, &msg->address->addr[sizeof(int)], sizeof(short));

        addMember(id, port, heartbeat);
        logAddress(id, port, true);

        memberNode->inGroup = true;
    }
    return true;
}

void MP1Node::addMember(int id, short port, long heartbeat) {
    long timestamp = this->par->getcurrtime();
    if (inMemberList(id, port)) return;
    MemberListEntry* entry = new MemberListEntry(id, port, heartbeat, timestamp); // issue
    memberNode->memberList.push_back(*entry);
}

Address* MP1Node::inMemberList(int id, short port) {
    for (std::vector<MemberListEntry>::iterator it = memberNode->memberList.begin();
        it != memberNode->memberList.end(); ++it)
    {
        if (it->id == id && it->port == port) {
            return getMember(it->id, it->port);
        }
    }
    return nullptr;
}

Address* MP1Node::getMember(int id, short port) {
    Address* adr = new Address();
    memcpy(&adr->addr[0], &id, sizeof(int));
    memcpy(&adr->addr[4], &port, sizeof(short));
    return adr;
}

void MP1Node::logAddress(int id, short port, bool add) {
    Address* address = new Address();
    memcpy(&address[0], &id, sizeof(int));
    memcpy(&address[4], &port, sizeof(short));
    if (add) {
        log->logNodeAdd(&memberNode->addr, address);
    } else {
        log->logNodeRemove(&memberNode->addr, address);
    }

    delete address;
}

/**
* FUNCTION NAME: nodeLoopOps
*
* DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
*                 the nodes
*                 Propagate your membership list
*/
void MP1Node::nodeLoopOps() {
    
    /*
     * Your code goes here
     */
    memberNode->heartbeat++;
    // 1, 2, 3, 4
    for (std::vector<MemberListEntry>::iterator it = memberNode->memberList.begin();
        it != memberNode->memberList.end(); ++it)
    {
        if(par->getcurrtime() >= TREMOVE + it->timestamp) {
            logAddress(it->id, it->port, false);
            memberNode->memberList.erase(it);
        }
    }


    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
