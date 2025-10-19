# ft_irc

## 🧠 Some Networking basics first..

### So, what's a server anyway?

A **server** is just a program that *listens* for incoming connections and *responds* when someone asks for something.
In other words, a server “serves.”

If you open your browser and hit `https://www.google.com`, you (the **client**) send a request, and Google’s **server** sends a response.

Same deal in IRC:  
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

🖥️ Hardware server: A physical machine running continuously (e.g., a datacenter computer)

⚙️ Software server: A program running on that machine (e.g., Nginx, ft_irc executable)

In **ft_irc**, we build a software server: **IRC server** — a small program that listens for client connections and speaks the IRC protocol (RFC 1459).


### 🌍 Okay, But How Does the data travel?

Imagine the Internet as a gigantic *postal system for data*. Every computer has an **address** (called an IP), and when you send something, it’s broken into tiny envelopes called **packets**. Your packet hops across routers, switches, and networks until it reaches the right destination. It’s a bit like your data playing “hot potato” around the globe.

**Basic Flow**
- Client --> Request --> Server
- Client <-- Response <-- Server

Example:
- You: “GET /index.html”
- Server: “Here’s the file!”

#### Key Components
- **IP Address:** Unique identifier for a device (e.g. `142.250.184.206`)
- **DNS:** Translates domain names to IPs
- **Ports:** Virtual “doors” into a machine (e.g. port 80 = HTTP, 6667 = IRC)
- **Protocols:** Standardized rules for communication (HTTP, TCP, IP, etc.)

#### 🧩 What’s Inside Those Packets?

When you send data — say, an IRC message like `PRIVMSG #42 :Hello!` — your computer doesn’t just blast that text straight onto the Internet. It wraps it up in several layers of information. Each packet contains:
- The **payload** (your actual data)
- Metadata (addresses, sequence numbers, etc.)

Think of it like a bunch of envelopes inside each other, Russian-doll style:
- [Ethernet Header] - [IP Header] - [TCP Header] - PRIVMSG #42 :Hello!

This is called Data Encapsulation. Each layer adds its own info:
- **Ethernet/WiFi:** “Source and Destination MAC addresses”
- **IP:** “Source & Destination IPs”
- **TCP:** “Source & destination port, sequence number, flags”
- **Payload:** Your actual message.

The server unwraps the packet layer by layer, reads the message, and replies.

#### Data Encapsulation Notes

A packet is born then  wrapped (encapsulated) in a header by the first protocol (eg. TFTP), then the whole thing inc. TFTP header is encapsulated again by the next protocol (e.g. UDP), then again by the next (IP), then again by the final protocol on the hardware (physical) layer (say, Ethernet). When another computer receives the packet, the hardware strips the Ethernet header, the kernel strips the IP and UDP headers, the TFTP program strips the TFTP header, and it finally has the data. [Beej’s Guide to Network Programming]
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

In the early days of the internet, there was a great network routing system called Internet Protocol Version 4 (IPv4) - written in number-decimal format - which provided about 4 billion unique addresses, but we ran out! Especially since a lot of companies took millions of addresses back then. To solve the shortage, IPv6 was born. It uses 128-bit addresses, giving us 340 undecillion possible combinations for practically unlimited use.
IPv6 addresses are written in hexadecimal and separated by colons, e.g. 2001:0db8:c9d2:aee5:73e3:934a:a5ae:9551. They can be shortened by omitting leading zeros or replacing consecutive zeros with ::, e.g. 2001:db8:c9d2:12::51.
::1 is the loopback address (equivalent to 127.0.0.1 in IPv4) aka this machine I am on, and ::ffff:192.0.2.33 represents an IPv4 address embedded inside an IPv6 address. IPv4 is still widely used today, but we’re slowly moving to IPv6 because IPv4 space is limited.

So, an **IP address** identifies a device on a network.
- **IPv4:** 32 bits → around **4 billion** addresses (`192.0.2.12`)
- **IPv6:** 128 bits → around **340 trillion trillion trillion** addresses (`2001:db8::1`)

The Structure of an IP Address each IP address has **two parts**:
- **Network part** 🏘️ — identifies the network
- **Host part** 🏠 — identifies the device (host) on that network
`192.0.2.12` with a **netmask** of `255.255.255.0` →  
Network: `192.0.2.0`, Host: `.12`

A **netmask** (or “subnet mask”) tells you which bits belong to the network and which belong to hosts. It’s often written in either full or **CIDR** (slash) notation:

| Form | Example | Meaning |
|------|----------|----------|
| Dotted decimal | `255.255.255.0` | Network = first 24 bits |
| CIDR notation | `192.0.2.12/24` | “/24” means 24 bits for network, 8 bits for host |

The network address is found by performing a **bitwise AND** between the IP and the mask.

**The Old System (Classful Networks)**: originally, IPs were divided into fixed-size “classes”:

| Class | Network Bits | Host Bits | Example | Hosts |
|--------|---------------|------------|----------|--------|
| A | 8 | 24 | 10.0.0.0 | ~16 million |
| B | 16 | 16 | 172.16.0.0 | ~65,000 |
| C | 24 | 8 | 192.168.0.0 | ~256 |

This system was simple but wasteful — networks were either too large or too small.

**Modern System (CIDR – Classless Inter-Domain Routing)** replaced classes with a flexible format that lets you choose *any number* of bits for the network part.
Examples:
- `192.0.2.12/30` → 30 bits for network, 2 bits for host
- `192.0.2.12/24` → 24 bits for network, 8 bits for host
- `2001:db8::/64` → IPv6 example, 64 bits for network

CIDR makes it possible to divide a large network into smaller **subnets**, optimizing address usage.

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

## 🧩 Socket Programming Basics

### 🔌 What Is a Socket?
A **socket** is just a way for programs to **talk to each other** — locally or over a network — using **file descriptors** (just like reading/writing a file!).

Every open connection in Unix gets a file descriptor (an integer).  
So, network I/O = file I/O.  
You open this connection with `socket()` system call, send data with `send()`, and receive it with `recv()`.

### 🧠 Socket Types
There are several socket “families”: DARPA Internet addresses (Internet Sockets), path names on a local node (Unix Sockets), CCITT X.25 addresses (X.25 Sockets), and many others.
For **ft_irc**, we only care about **Internet sockets** (IPv4/IPv6).

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
- Used for: games 🎮, streaming 🎧, video calls 📹, DHCP, and TFTP.

When reliability *does* matter (like in TFTP), programs add their own small acknowledgment system (sending “ACK” packets and retrying).

### ⚙️ Summary
| Feature | Stream (TCP) | Datagram (UDP) |
|----------|---------------|----------------|
| Connection | Yes 🔗 | No 🚫 |
| Reliability | High ✅ | Low ⚠️ |
| Speed | Slower 🐢 | Faster ⚡ |
| Example | IRC, HTTP | Games, VoIP |

For **ft_irc**, we’ll use **TCP stream sockets** — because chat needs reliable, ordered communication between multiple clients and our server.

---

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
