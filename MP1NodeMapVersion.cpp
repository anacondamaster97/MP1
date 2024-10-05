/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 *              Definition of MP1Node class functions. (Revised 2020)
 *
 *  Starter code template
 **********************************/

#include "MP1Node.h"
#include <tuple>

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */

std::map<Address*, vector<MemberListEntry>> mainMap;

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

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        
        // Create JOINREQ message
        msg = new MessageHdr();
        msg->msgType = JOINREQ;
        mainMap[&memberNode->addr] = memberNode->memberList;
        msg->addr = &memberNode->addr;

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, sizeof(MessageHdr));

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
 *              Check your messages in queue and perform membership protocol duties
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
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
    MessageHdr* msg = (MessageHdr*) data;
    if(msg->msgType == JOINREQ){

        addMember(msg);
        Address* toaddr = msg->addr;
        handleMessage(toaddr, JOINREP); 

    }else if(msg->msgType == JOINREP){

        addMember(msg); 
        memberNode->inGroup = true;

    }else if(msg->msgType == PING){

        handlePing(msg);

    } else {
        assert(false);
    }
    delete msg;
    return true;
}

void MP1Node::addMember(MessageHdr* msg) {
    // id, port, heartbeat, timestamp
    std::tuple<int, short> idPort = getIdPort(msg);
    if(inMemberList(get<0>(idPort), get<1>(idPort)) == nullptr) {
        MemberListEntry newMember(get<0>(idPort), get<1>(idPort), 1, this->par->getcurrtime());
        memberNode->memberList.push_back(newMember);
        handleLogging(&memberNode->addr, getMemberAddress(get<0>(idPort), get<1>(idPort)), true);
    }
}

std::tuple<int, short> MP1Node::getIdPort(MessageHdr* msg) {
    int id;
    short port;
    memcpy(&id, &msg->addr->addr[0], sizeof(int));
    memcpy(&port, &msg->addr->addr[4], sizeof(short));
    std::tuple<int, short> idPort = make_tuple(id, port);
    return idPort;
}

Address* MP1Node::getMemberAddress(int id, short port) {
    Address* address = new Address();
    memcpy(&address->addr[0], &id, sizeof(int));
    memcpy(&address->addr[4], &port, sizeof(short));
    return address;
}

MemberListEntry* MP1Node::inMemberList(int id, short port) {
    for (int i = 0; i < memberNode->memberList.size(); i++){
        if(memberNode->memberList[i].id == id && memberNode->memberList[i].port == port)
            return &memberNode->memberList[i];
    } 
    return nullptr;
}

void MP1Node::handleMessage(Address* toaddr, MsgTypes t) {
    MessageHdr* msg = new MessageHdr();
    msg->msgType = t;
    mainMap[&memberNode->addr] = memberNode->memberList;
    msg->addr = &memberNode->addr;
    emulNet->ENsend( &memberNode->addr, toaddr, (char*)msg, sizeof(MessageHdr));    
}

void MP1Node::handlePing(MessageHdr* msg) {

    std::tuple<int, short> idPort = getIdPort(msg);
    int id = get<0>(idPort);
    short port = get<1>(idPort);
    
    MemberListEntry* receivedMember = inMemberList(id, port);

    if(receivedMember == nullptr){
        addMember(msg);
    }else{
        receivedMember->heartbeat++;
        receivedMember->timestamp = par->getcurrtime();
    }
    for(int i = 0; i < mainMap[msg->addr].size(); i++){
        
        MemberListEntry* node = inMemberList(mainMap[msg->addr][i].id, mainMap[msg->addr][i].port);  
        if(node == nullptr){
            MemberListEntry* memberEntry = &mainMap[msg->addr][i];
            Address* addr = getMemberAddress(memberEntry->id, memberEntry->port);
            
            if (*addr == memberNode->addr) {
                delete addr;
                return;
            }

            if (par->getcurrtime() - memberEntry->timestamp < TREMOVE) {
                memberNode->memberList.push_back(*memberEntry);
                handleLogging(&memberNode->addr, addr, true);
            }
            delete addr;
    
        } else {
            if(mainMap[msg->addr][i].heartbeat > node->heartbeat){
                node->heartbeat = mainMap[msg->addr][i].getheartbeat();
                node->timestamp = par->getcurrtime();
            }
        }
    }
}

void MP1Node::handleLogging(Address *thisNode, Address *addedNode, bool add) {
    if (add) {
        log->logNodeAdd(thisNode, addedNode);
    } else {
        log->logNodeRemove(thisNode, addedNode);
    }
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 *              the nodes
 *              Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

    memberNode->heartbeat++;

    handleTimeout();

    for (int i = 0; i < memberNode->memberList.size(); i++) {
        Address* address = getMemberAddress(memberNode->memberList[i].getid(), memberNode->memberList[i].getport());
        handleMessage(address, PING);
        delete address;
    }
    return;
}

void MP1Node::handleTimeout() {
    auto it = memberNode->memberList.begin();
    int position = 1;

    while (it != memberNode->memberList.end()) {
        if(par->getcurrtime() - it->timestamp >= TREMOVE) {
            Address* address = getMemberAddress(it->id, it->port);
            handleLogging(&memberNode->addr, address, false);
            it = memberNode->memberList.erase(it);
            delete address;
        } else {
            ++it;
        }
        ++position;
    }
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

    memset(&joinaddr, 0, sizeof(Address));
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
