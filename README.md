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

---

### 🌍 Okay, But How Does the data travel?

Imagine the Internet as a gigantic *postal system for data*. Every computer has an **address** (called an IP), and when you send something, it’s broken into tiny envelopes called **packets**.

Your packet hops across routers, switches, and networks until it reaches the right destination.  
It’s a bit like your data playing “hot potato” around the globe.

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

When you send data — say, an IRC message like `PRIVMSG #42 :Hello!` — your computer doesn’t just blast that text straight onto the Internet.

Nope. It wraps it up in several layers of information. Each packet contains:
- The **payload** (your actual data)
- Metadata (addresses, sequence numbers, etc.)

Think of it like a bunch of envelopes inside each other, Russian-doll style:
- [Ethernet Header] - [IP Header] - [TCP Header] - PRIVMSG #42 :Hello!

Each layer adds its own info:
- **Ethernet/WiFi:** “Source and Destination MAC addresses”
- **IP:** “Source & Destination IPs”
- **TCP:** “Source & destination port, sequence number, flags”
- **Payload:** Your actual message.

The server unwraps the packet layer by layer, reads the message, and replies.

#### 🪜 The 5-Layer Model (aka The Great Stack)

Here’s how your data climbs down and back up the network stack:

| Layer | Example Protocols | What It Does |
|--------|------------------|---------------|
| **Application** | HTTP, FTP, IRC | What you actually care about |
| **Transport** | TCP, UDP | Splits and reassembles data |
| **Network** | IP | Routes data across networks |
| **Link** | Ethernet, Wi-Fi | Local delivery on your LAN |
| **Physical** | Copper, Fiber, Radio | Sends the actual bits (1s and 0s) through electrons / light / radio waves|

Each header just adds routing info to make sure the right data gets to the right place. Each layer only knows about its own job and can only talk to the layers above or below it.  
That’s what makes the Internet modular and beautiful.

### 💬 The IRC Server (ft_irc in Action)

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

Quick Recap:
User types a message
- IRC client sends it over TCP
- Broken into packets with IP + port info
- Travels across routers
- IRC server receives and reassembles it
- Processes command and responds

--
## Socket Programing Basics

### What is a Socket?
A socket is a way to speak to other programs using standard Unix file descriptors. When UNIX programs do any I/O, it is done through a file descriptor.
A file descriptor is an integer associated with an open file; which could also be a network connection, a pipe, a FIFO, a terminal, or a real disk file, etc.
So communicating with other programs on the internet also requires a file descriptor! 
*Socket()*: We open this connection by calling the socket() system routine.
*send() & recv()*: used to communicate through the socket.

#### Types of Sockets
Some socket types: DARPA Internet addresses (Internet Sockets), path names on a local node (Unix Sockets), CCITT X.25 addresses (X.25 Sockets), and many others. In ft_irc we will deal with internet sockets.

Types of Internet Sockets: Stream Sockets “SOCK_STREAM”, Datagram Sockets “SOCK_DGRAM”, Raw Sockets, and many others...

**Stream Sockets** are reliable two-way connected communication streams - outputs data consecutively as received, error-free. Telnet and ssh network protocols use stream sockets. Also, web browsers use the Hypertext Transfer Protocol (HTTP) which uses stream sockets to get pages.
To achieve this reliability, stream sockets use "The Transmission Control Protocol" aka "TCP" which makes sure data arrives sequentially and error-free. "TCP/IP" is a suite of protocols using TCP and IP (Internet Protocol) to connect network devices on the internet and on private networks. the IP part deals with internet routing but data integrity is TCP's responsibility.

**Datagram Sockets** aka connectionless sockets, are unreliable - if you send a datagram, it may or may not arrive; but if it does, it will be error-free.
They are also used for IP routing but not through TCP but through "User Datagram Protocol" or UDP. Here an open connection does not need to be maintained. A few dropped packets is not the end of the world and you want SPEED!!
e.g. multiplayer games, streaming audio, video conferencing, tftp (trivial file transfer protocol), dhcpcd (a DHCP client), etc.
In tftp and dhcpcd however realiability matters! But these programs also have their own protocols on top of UDP that sends out a confirmation once a paket is received (an "ACK" paket). If no confirmation was received, the sender re-tries after a few seconds until the paket receipt is acknowleged. Reliable SOCK_DGRAM applications require this acknowledgement procedure.
 
