#Gossip Membership Protocol with Heartbeating
##Overview
This project implements a Gossip Membership Protocol using heartbeating to manage membership in a distributed system. The protocol ensures that all nodes in the system are aware of each other’s presence by propagating membership information through a gossip mechanism. It's designed to handle node joins, failures, and departures in a scalable and fault-tolerant way.

##Background
###History of Gossip Protocols
Gossip protocols have been widely used in distributed systems due to their simplicity and robustness. First developed in the 1980s, gossip-based communication mimics the way information spreads in social networks. Every node in the system periodically selects a random peer to exchange information, ensuring the eventual dissemination of data.

#Key features of gossip protocols include:

Scalability: Due to the decentralized nature, gossip protocols scale well with the number of nodes.
Fault tolerance: The failure of one or more nodes has minimal impact on the protocol's operation.
Simplicity: The protocol is straightforward to implement and understand.
Gossip protocols have been adapted for various distributed system tasks, such as membership management, failure detection, and data replication.

#Protocol Design
###Overview
In this implementation, each node maintains a list of other nodes in the system along with their statuses (alive, suspected, failed). Nodes send periodic heartbeats to a subset of other nodes to update them on their state. If no heartbeat is received from a node within a timeout period, the node is suspected of failure.

###Components of the Protocol
JOINREQ (Join Request): When a new node attempts to join the system, it sends a JOINREQ message to an existing node. This message contains the joining node's ID and address.

JOINREP (Join Reply): Upon receiving a JOINREQ, an existing node replies with a JOINREP message that contains the current membership list, allowing the new node to integrate with the system.

Ping Mechanism: Each node periodically sends PING messages to a random subset of other nodes to ensure they are still alive. A PING includes the sender’s heartbeat counter.

Indirect Ping: If a direct PING does not receive a reply within a timeout period, the node sends an indirect ping by requesting another node to ping the unresponsive node on its behalf.

Heartbeat Counter: Each node maintains a heartbeat counter, which it increments at regular intervals and shares with other nodes in its PING messages.

Gossip: Membership changes (e.g., node failures or new joins) are gossiped between nodes through periodic exchanges of membership lists.
