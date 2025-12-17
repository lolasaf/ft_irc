# ft_irc

## 🧠 Some Networking basics first..

### So, what's a server anyway?

A **server** is just a program that *listens* for incoming connections and *responds* when someone asks for something. In other words, a server “serves.” If you open your browser and hit `https://www.google.com`, you (the **client**) send a request, and Google’s **server** sends a response. Same deal in IRC:
The IRC **client** says `NICK John`, the **server** hears it and says, “Cool, welcome John!”

**Common Kinds of Servers:**

| Type | What It Does | Example |
|------|---------------|----------|
| Web Server | Serves web pages | Apache, Nginx (HTTP/HTTPS) |
| File Server | Shares files | FTP Server (FTP/SFTP) |
| Mail Server | Sends/receives email | Postfix, Sendmail (SMTP, IMAP, POP3) |
| Database Server | Stores data | MySQL, PostgreSQL |
| DNS Server | Translates names → IPs | Google DNS (8.8.8.8) |
| IRC Server | Handles chat messages | UnrealIRCd, InspIRCd |
| Game Server | Runs online matches | Minecraft, CS:GO |
| Proxy Server | Forwards requests | Squid, HAProxy |
| App Server | Runs backend logic for apps (e.g., APIs) | Node.js, Django, Java EE |

- 🖥️ Hardware server: A physical machine running continuously (e.g., a datacenter computer)
- ⚙️ Software server: A program running on that machine (e.g., Nginx, ft_irc executable)
In **ft_irc**, we build a software server: **IRC server** — a small program that listens for client connections and speaks the IRC protocol (RFC 1459).

### 🌍 Okay, But How Does the data travel?

Imagine the Internet as a gigantic *postal system for data*. Every computer has an **address** (called an IP), and when you send something, it’s broken into tiny envelopes called **packets**. Your packet hops across routers, switches, and networks until it reaches the right destination. It’s a bit like your data playing “hot potato” around the globe. Basic Flow:
- Client --> Request --> Server --> Response --> Client
Example:
- You: “GET /index.html”
- Server: “Here’s the file!”

#### Key Components
- **IP Address:** Unique identifier for a device (e.g. `142.250.184.206`)
- **DNS Server:** Translates domain names to IPs
- **Ports:** Virtual “doors” into a machine (e.g. port 80 = HTTP, 6667 = IRC)
- **Protocols:** Standardized rules for communication (HTTP, TCP, IP, etc.)

#### 🧩 What’s Inside Those Packets?

When you send data — say, an IRC message like `PRIVMSG #42 :Hello!` — your computer doesn’t just blast that text straight onto the Internet. It wraps it up in several layers of information. Each packet contains:
- The **payload** (your actual data)
- Metadata (addresses, sequence numbers, etc.)

Think of it like a bunch of envelopes inside each other, Russian-doll style:
- [Ethernet Header] - [IP Header] - [TCP Header] - PRIVMSG #42 :Hello!

This is called Data Encapsulation. Each layer adds its own info:
- **Ethernet/WiFi:** “Source and Destination MAC (Media/Medium Access Control) addresses”
- **IP:** “Source & Destination IPs”
- **TCP:** “Source & destination port, sequence number, flags”
- **Payload:** Your actual message.

The server unwraps the packet layer by layer, reads the message, and replies.

#### Data Encapsulation Notes

> A packet is born then  wrapped (encapsulated) in a header by the first protocol (eg. TFTP), then the whole thing inc. TFTP header is encapsulated again by the next protocol (e.g. UDP), then again by the next (IP), then again by the final protocol on the hardware (physical) layer (say, Ethernet). When another computer receives the packet, the hardware strips the Ethernet header, the kernel strips the IP and UDP headers, the TFTP program strips the TFTP header, and it finally has the data. [Beej’s Guide to Network Programming]

<img width="855" height="203" alt="image" src="https://github.com/user-attachments/assets/63fd167b-8149-4f6a-8cff-24411439a365" />
Image Source: https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf

#### 🪜 The 5-Layer Model (aka The Great Stack)

Here’s how your data climbs down and back up the network stack:

| Layer | Example Protocols | What It Does |
|--------|------------------|---------------|
| **Application** | HTTP, FTP, IRC | What you actually care about |
| **Transport** | TCP, UDP | Splits and reassembles data |
| **Network** | IP | Routes data across networks |
| **Data Link** | serial, Ethernet, Wi-Fi | Local delivery on your LAN |
| **Physical** | Copper, Fiber, Radio | Sends the actual bits (1s and 0s) through electrons / light / radio waves|

Each header just adds routing info to make sure the right data gets to the right place. Each layer only knows about its own job and can only talk to the layers above or below it.  
That’s what makes the Internet modular and beautiful.

### 🌐 IP Addresses (IPv4 vs IPv6)
*NET PRACTICE RECAP*

In the early days of the internet, there was a great network routing system called Internet Protocol Version 4 (IPv4) - written in number-decimal format - which provided about 4 billion unique addresses, but we ran out! (especially since a lot of companies took millions of addresses back then). To solve the shortage, IPv6 was born. It uses 128-bit addresses, giving us 340 undecillion possible combinations for practically unlimited use.
IPv6 addresses are written in hexadecimal and separated by colons, e.g. 2001:0db8:c9d2:aee5:73e3:934a:a5ae:9551. They can be shortened by omitting leading zeros or replacing consecutive zeros with ::, e.g. 2001:db8:c9d2:12::51.
::1 is the loopback address (equivalent to 127.0.0.1 in IPv4) aka this machine I am on, and ::ffff:192.0.2.33 represents an IPv4 address embedded inside an IPv6 address.

So, an **IP address** identifies a device on a network.
- **IPv4:** 32 bits → around **4 billion** addresses (`192.0.2.12`)
- **IPv6:** 128 bits → around **340 trillion trillion trillion** addresses (`2001:db8::1`)

The Structure of an IP Address each IP address has **two parts**:
- **Network part** — identifies the network
- **Host part** — identifies the device (host) on that network
`192.0.2.12` with a **netmask** of `255.255.255.0` →  
Network: `192.0.2.0`, Host: `.12`

A **netmask** (or “subnet mask”) tells you which bits belong to the network and which belong to hosts. It’s often written in either full or **CIDR** (slash) notation:

| Form | Example | Meaning |
|------|----------|----------|
| Dotted decimal | `255.255.255.0` | Network = first 24 bits |
| CIDR notation | `192.0.2.12/24` | “/24” means 24 bits for network, 8 bits for host |

The network address is found by performing a bitwise AND between the IP and the mask.

**The Old System (Classful Networks)**: originally, IPs were divided into fixed-size “classes”:

| Class | Network Bits | Host Bits | Example | Hosts |
|--------|---------------|------------|----------|--------|
| A | 8 | 24 | 10.0.0.0 | ~16 million |
| B | 16 | 16 | 172.16.0.0 | ~65,000 |
| C | 24 | 8 | 192.168.0.0 | ~256 |

This system was simple but wasteful — networks were either too large or too small.

**Modern System (CIDR – Classless Inter-Domain Routing)** replaced classes with a flexible format that lets you choose *any number* of bits for the network part. Examples:
- `192.0.2.12/30` → 30 bits for network, 2 bits for host
- `192.0.2.12/24` → 24 bits for network, 8 bits for host
- `2001:db8::/64` → IPv6 example, 64 bits for network

CIDR makes it possible to divide a large network into smaller *subnets*, optimizing address usage.

**Subnets** — Organizing the Network: it is simply a smaller section of a network - helps organize devices logically and efficiently.
Example:
- `192.168.1.0/24` → one large network (up to 254 devices)
- Split into smaller subnets:
  - `192.168.1.0/26` → 62 usable hosts
  - `192.168.1.64/26` → another 62 hosts
  - etc.

Every subnet reserves two special addresses:
- **Network address** → identifies the subnet itself (e.g. `192.0.2.0`)
- **Broadcast address** → used to contact all hosts in that subnet (e.g. `192.0.2.255`)

That means:
> **Usable hosts = 2ⁿ − 2**  
> where *n* = number of host bits.

| CIDR | Host Bits | Total IPs | Usable Hosts | Example |
|------|------------|------------|---------------|----------|
| `/30` | 2 | 4 | 2 | Smallest subnet with usable hosts |
| `/29` | 3 | 8 | 6 | Small private subnet |
| `/24` | 8 | 256 | 254 | Common LAN subnet |
| `/16` | 16 | 65,536 | 65,534 | Large enterprise network |

So the **minimum number of usable hosts** on a normal IPv4 subnet is **2** (`/30`).  
(A `/31` can be used only for special point-to-point connections.)

**Quick Summary**
- **IPv4** = 32-bit, limited addresses  
- **IPv6** = 128-bit, virtually unlimited  
- **Netmask/CIDR** = defines network vs. host bits  
- **Subnet** = smaller piece of a network  
- **Usable hosts = 2ⁿ − 2** (because of network + broadcast)  
- **Minimum usable hosts = 2** (`/30` network)
Subnetting is just a way to **divide networks efficiently** — making sure every router, server, and client fits neatly into its own digital neighborhood.

---

## 🔌 Socket Programming Basics

### What Is a Socket?
A **socket** is just a way for programs to talk to each other — locally or over a network — using file descriptors (just like reading/writing a file!). Every open connection in Unix gets a file descriptor (an integer). So, network I/O = file I/O.
You open this connection with `socket()` system call, send data with `send()`, and receive it with `recv()`.

### Socket Types
There are several socket “families”: DARPA Internet addresses (Internet Sockets), path names on a local node (Unix Sockets), CCITT X.25 addresses (X.25 Sockets), and many others.
For ft_irc, we only care about **Internet sockets** (IPv4/IPv6):

| Type | Constant | Description |
|------|-----------|-------------|
| **Stream Socket** | `SOCK_STREAM` | Reliable, ordered, connection-based (uses TCP) |
| **Datagram Socket** | `SOCK_DGRAM` | Unreliable, fast, connectionless (uses UDP) |

### 💬 Stream Sockets (`SOCK_STREAM`)
- Think of it as a **phone call** 📞 — once connected, both sides can send/receive continuously.  
- Uses **TCP (Transmission Control Protocol)** for reliability: data arrives **in order**, **error-free**, or it’s retransmitted.
- Used by: `HTTP`, `SSH`, `Telnet`, and yes — the **IRC server**.

TCP/IP is really two things:
- **IP (Internet Protocol):** Finds where to send the data (routing).
- **TCP:** Ensures it gets there intact.

### 🚀 Datagram Sockets (`SOCK_DGRAM`)
- aka connectionless sockets. Think of it as **sending letters without waiting for a reply** ✉️
- If you send a datagram, it may or may not arrive; but if it does, it will be error-free.
- Uses **UDP (User Datagram Protocol)** — no guaranteed delivery, but *fast* and *lightweight*.
- Used for: games, streaming, video calls, DHCP, and TFTP.

When reliability *does* matter (like in TFTP), programs add their own small acknowledgment system (sending “ACK” packets and retrying).

### Summary
| Feature | Stream (TCP) | Datagram (UDP) |
|----------|---------------|----------------|
| Connection | Yes 🔗 | No 🚫 |
| Reliability | High ✅ | Low ⚠️ |
| Speed | Slower 🐢 | Faster ⚡ |
| Example | IRC, HTTP | Games, VoIP |

For ft_irc, we’ll use **TCP stream sockets** — because chat needs reliable, ordered communication between multiple clients and our server.

## How sockets handle IP addresses and other data

**Ports**: The IP address used on the routing layer within the Layered Network Model is not the only address needed. TCP (stream sockets) and UDP (datagram sockets) also need a *port number* - a 16-bit number representing the local address of the connection. Each port is used for specific kinds of communications which is pre-defined in the BIG IANA Port List. On a UNIX machine: `cat /etc/services` to check them. e.g. HTTP (the web) is port 80, telnet is 23, etc.  

**Byte Orders**: Two byte orders -> Big Endian (storing big byte first) and Little Endian (storing small byte first). Big-Endian is also called *Network Byte Order* because that's what networks like to use and because this order makes more sense.
Now the order used in the computer is called *Host Byte Order*. When building packets or filling out data structures, we need to know the native Host Byte Order in order to properly and accurately translate it to the network. To convert numbers from Host to Network Byte order, we use funcions depending if the number is short (2 bytes) or long (4 bytes).
- `htons` h (Host) to n (Network) s (Short)
- similarly `htonl` Host to Network Long
- and in reverse: `ntohs` and `ntohl` 

### Socket Structs

Struct Data structures are used like containers for network information. It will store useful data needs by the OS like what kind of connection we want (IPv4 or IPv6), what port/address and what protocol (TCP, UDP).

Opening a socket:
`int sockfd = socket(); // keep fd of open connection`

`struct addrinfo`
Data needed to prepare the connection setup is stored in `struct addrinfo` <- filled by calling `getaddrinfo()` which fills out a linked list of these structs (because sometimes a hostname can resolve to multiple IPs).
```text
struct addrinfo {
    int		ai_flags;				// AI_PASSIVE, AI_CANONNAME, etc.
    int		ai_family;				// AF_INET, AF_INET6, AF_UNSPEC (IPv4/v6/agnostic)
    int 	ai_socktype;			// SOCK_STREAM, SOCK_DGRAM
    int		ai_protocol;			// use 0 for "any"
    size_t	ai_addrlen;				// size of ai_addr in bytes
    struct	sockaddr *ai_addr;		// struct sockaddr_in or _in6
    char	*ai_canonname;			// full canonical hostname

    struct addrinfo *ai_next;		// next node
};
```

`struct sockaddr`
Inside `struct addrinfo` there is a pointer to a `struct sockaddr` which is the base struct used to represent a socket address.
```text
struct sockaddr {
	unsigned short	sa_family;		// address family, AF_XXX
	char			sa_data[14];	// 14 bytes of protocol address
};
```
`sa_family` can hold many values but for our purpose it will be `AF_INET` for IPv4 and `AF_INET6` for IPv6. `sa_data` contains a destination address and a port number for the socket. It is hard to work this in this form, so programmers created specialized versions for IPv4 and IPv6: `struct sockaddr_in` and `struct sockaddr_in6` respectively (in for internet).
Pointers to these structs can be typecasted to `(struct sockaddr *)` when calling functions like `cnnect()` or `bind()`.

#### IPv4 Address Structs
`struct sockaddr_in`
```text
struct sockaddr_in {
	short int			sin_family;		// address family, AF_INET
	unsigned short int	sin_port;		// port number
	struct in_addr		sin_addr;		// Internet address
	unsigned char		sin_zero[8];	// Padding to length of struct sockaddr
};
``` 
- `sin_family`	→ always AF_INET
- `sin_port`	→ must be in network byte order using `htons()`
- `sin_zero`	→ padding (set to 0 with memset)
- `sin_addr`	→ holds the actual IPv4 address (in another struct):
```text
// internet address
struct in_addr {
	uint32_t s_addr; // a 32-bit int (4 bytes)
};
```
`struct sockaddr_in	ina;` -> `ina.sin_addr.s_addr` -> 4-byte IP address in Network Byte Order

#### IPv6 Address Structs
`struct sockaddr_in6` and `struct in6_addr`
```text
struct sockaddr_in6 {
	u_int16_t		sin6_family;	// address family, AF_INET6
	u_int16_t		sin6_port;		// Port, Network Byte Order
	u_int32_t		sin6_flowinfo;	// IPv6 flow information (usually 0)
	struct in6_addr	sin6_addr;		// IPv6 address
	u_int32_t		sin6_scope_id;	// Scope ID (used for special cases)
};

struct in6_addr {
unsigned char s6_addr[16]; // IPv6 address
};
```

#### Struct used for any IP address
`struct sockaddr_storage` can be used when we do not know with kind of IP address we will get. For instance, for `accept()` calls when any kind of client can connect.
```text
struct sockaddr_storage {
	sa_family_t		ss_family;  // AF_INET or AF_INET6

	// all this is padding, implementation specific, ignore it:
	char	__ss_pad1[_SS_PAD1SIZE];
	int64_t	__ss_align;
	char	__ss_pad2[_SS_PAD2SIZE];
};
```
This struct can be cast to an IPv4 or IPv6 struct after checking ss_family value (if `AF_INET` or `AD_INET6`).

#### Structs Summary
In Simple Terms...
- `addrinfo` → tells you where to connect
- `sockaddr_in` / `sockaddr_in6` → store the address + port
- `in_addr` / `in6_addr` → store the raw IP
- `sockaddr_storage` → “universal bucket” that can hold any address
You often don’t fill these manually — `getaddrinfo()` does it for you. Once filled, you just pass pointers to these structs into functions like `connect()`, `bind()`, or `sendto()`, usually cast to `(struct sockaddr *)`.

*How Socket Structs Fit Together:*

```text
┌─────────────────────────────────────────┐
│ struct addrinfo (linked list)           │
│-----------------------------------------│
│ ai_family    → AF_INET / AF_INET6       │
│ ai_socktype  → SOCK_STREAM / SOCK_DGRAM │
│ ai_protocol  → usually 0                │
│ ai_addrlen   → size of ai_addr          │
│ ai_addr ──────────────────────┐         │
│ ai_next      → next result    │         │
└────────────────────────────── │ ────────┘
                                │
                                ▼
                  ┌───────────────────────────┐
                  │ struct sockaddr (generic) │
                  │---------------------------│
                  │ sa_family → AF_INET / 6   │
                  │ sa_data[14] → raw address │
                  └───────────────────────────┘
                                │
             ┌──────────────────┴──────────────────┐
             │                                     │
             ▼                                     ▼
┌─────────────────────────┐       ┌──────────────────────────┐
│ struct sockaddr_in      │       │ struct sockaddr_in6      │
│ (IPv4 specific)         │       │ (IPv6 specific)          │
│-------------------------│       │--------------------------│
│ sin_family = AF_INET    │       │ sin6_family = AF_INET6   │
│ sin_port   = htons(port)│       │ sin6_port = htons(port)  │
│ sin_addr ───────────┐   │       │ sin6_addr ────────────┐  │
│ sin_zero[8] padding │   │       │ flowinfo, scope_id... │  │
└──────────────────── │ ──┘       └────────────────────── │ ─┘
    ┆                 │                     ┆             │
    ┆                 ▼                     ┆             ▼
    ┆         ┌──────────────────────────┐  ┆  ┌────────────────────────────┐
    ┆         │ struct in_addr           │  ┆  │ struct in6_addr            │
    ┆         │ (IPv4 address = 4 bytes) │  ┆  │ (IPv6 address = 16 bytes)  │
    ┆         │--------------------------│  ┆  │----------------------------│
    ┆         │ s_addr (uint32_t)        │  ┆  │ s6_addr[16] (bytes array)  │
    ┆         └──────────────────────────┘  ┆  └────────────────────────────┘
    ┆                                       ┆
    ┆   ┌───────────────────────────────┐   ┆
    ┆   │ struct sockaddr_storage       │   ┆
    └---│ (universal container)         │---┘
        │-------------------------------│
        │ ss_family → AF_INET / AF_INET6│
        │ big enough for either type    │
        └───────────────────────────────┘

```
In Practice:
- You call `getaddrinfo()` → gives you a list of `addrinfo` structs.
- Each addrinfo contains a pointer to a `sockaddr` (which can represent IPv4 or IPv6).
- You cast that sockaddr to `sockaddr_in` or `sockaddr_in6` depending on `ai_family`.
- The nested `in_addr` or `in6_addr` holds the actual numeric IP address.

### IP-Address Handling
- `inet_pton()`: pton stands for Presentation to Network, to convert string IP addresses to their binary presentation and store them in socket address structs
```text
struct sockaddr_in sa;		// IPv4
struct sockaddr_in6 sa6;	// IPv6

inet_pton(AF_INET, "10.12.110.57", &(sa.sin_addr));
inet_pton(AF_INET6, "2001:db8:63b3:1::3490", &(sa6.sin6_addr));
// inet_pton() returns -1 on error and 0 if address is messed up
```

- `inet_ntop()`: ntop stands for Network to Presentation, to convert IPs from binary to digits-and-dots or hex-and-colons notations.
```text
// IPv4
char ip4[NET_ADDRSTRLEN];	// to hold IPv4 string
struct sockaddr_in sa;		// preloaded socket address struct
inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSRLEN);
printf("The IPv4 address is: %s\n", ip4);

// IPv6
char ip6[INET6_ADDRSTRLEN];
struct sockaddr_in6 sa6;
inet_ntop(AF_INET6, &(sa6.sin6_addr), ip6, INET6_ADDRSTRLEN);
printf("The IPv6 address is: %s\n", ip6);
```
`INET_ADDRSTRLEN` and `INET6_ADDRSTRLEN`: Macros for IP address sizes.


## 💬 The IRC Server (ft_irc in Action)

When you build your server, you’ll:

- **Create a socket** – “Hey OS, I want to talk on the network.”
- **Bind it** – “Reserve this address and port (e.g. 127.0.0.1:6667).”
- **Listen** – “I’m waiting for someone to connect.”
- **Accept** – “Got one! Let’s talk.”
- **Read / Write** – “NICK john” in, “Welcome john!” out.

That’s the essence of a network server.

**Example Conversation**
- Client → NICK john
- Client → USER john 0 * :John Doe
- Server → :irc.local 001 john :Welcome to the IRC network, john!

We need to handle all those commands, keep track of users and channels, and broadcast messages when people talk.

🧱 **Big Picture: ft_irc Architecture**

```text
+-------------------+
|      Clients      |
|  (HexChat, irssi) |
+---------+---------+
          |
          |   TCP messages (NICK, JOIN, PRIVMSG...)
          v
+-------------------+
|     ft_irc        |
|  (your server)    |
|-------------------|
| Socket handling   |
| Command parsing   |
| Channel mgmt      |
| Message routing   |
+-------------------+
          |
          |   Responses to clients
          v
+-------------------+
|      Network      |
|   (TCP/IP stack)  |
+-------------------+
```

**🔄 Socket Lifecycle (Server Side)**

```text
      ┌───────────────────────────────────────────────┐
      │                 Your ft_irc                   │
      └───────────────────────────────────────────────┘
                        │
                        ▼
              ┌────────────────┐
              │ socket()       │  → Create a socket (get fd)
              └────────────────┘
                        │
                        ▼
              ┌────────────────┐
              │ bind()         │  → Assign IP + port (e.g. 127.0.0.1:6667)
              └────────────────┘
                        │
                        ▼
              ┌────────────────┐
              │ listen()       │  → Tell OS you’re ready for clients
              └────────────────┘
                        │
                        ▼
              ┌────────────────┐
              │ accept()       │  → Wait for incoming connection
              └────────────────┘
                        │
                        ▼
              ┌────────────────┐
              │ send()/recv()  │  → Exchange messages (NICK, JOIN, PRIVMSG)
              └────────────────┘
                        │
                        ▼
              ┌────────────────┐
              │ close()        │  → Disconnect socket
              └────────────────┘
```

** Client Side for reference**

```text

      ┌────────────────┐
      │ socket()       │  → Create socket
      └────────────────┘
            │
            ▼
      ┌────────────────┐
      │ connect()      │  → Connect to server IP:port
      └────────────────┘
            │
            ▼
      ┌────────────────┐
      │ send()/recv()  │  → Talk to server
      └────────────────┘
            │
            ▼
      ┌────────────────┐
      │ close()        │  → Disconnect
      └────────────────┘
```

Quick Recap:
User types a message
- IRC client sends it over TCP
- Broken into packets with IP + port info
- Travels across routers
- IRC server receives and reassembles it
- Processes command and responds
