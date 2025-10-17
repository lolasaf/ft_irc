# ft_irc

## 🌐 Some Networking basics on Servers (and the Internet)

Hey there!  
If you’re jumping into **ft_irc** and you’ve never touched networking before, this guide is for you.

It’s not a deep academic dive — it’s a friendly walk through *what’s actually going on* when your IRC server talks to the world.

---

### 🧠 So… What’s a Server, Anyway?

A **server** is just a program that *listens* for incoming connections and *responds* when someone asks for something.

In other words, a server “serves.”

If you open your browser and hit `https://www.google.com`, you (the **client**) send a request, and Google’s **server** sends a response.

Same deal in IRC:  
Your IRC **client** says `NICK John`, your **server** hears it and says, “Cool, welcome John!”

### Common Kinds of Servers

| Type | What It Does | Example |
|------|---------------|----------|
| Web Server | Serves web pages | Apache, Nginx |
| File Server | Shares files | FTP |
| Mail Server | Sends/receives email | Postfix |
| Database Server | Stores data | MySQL |
| DNS Server | Translates names → IPs | Google DNS |
| IRC Server | Handles chat messages | InspIRCd |
| Game Server | Runs online matches | Minecraft |
| Proxy Server | Forwards requests | Squid |

In **ft_irc**, you’ll be writing your very own **IRC server** — a small program that listens for client connections and speaks the IRC protocol (RFC 1459).

---

### 🌍 Okay, But How Does the Internet Work?

Imagine the Internet as a gigantic *postal system for data*.  
Every computer has an **address** (called an IP), and when you send something, it’s broken into tiny envelopes called **packets**.

Your packet hops across routers, switches, and networks until it reaches the right destination.  
It’s a bit like your data playing “hot potato” around the globe.

#### Basic Flow

Client --> Request --> Server
Client <-- Response <-- Server

Example:
- You: “GET /index.html”
- Server: “Here’s the file!”

Simple, right?

---

### 🧩 What’s Inside Those Packets?

When you send data — say, an IRC message like `PRIVMSG #42 :Hello!` — your computer doesn’t just blast that text straight onto the Internet.

Nope. It wraps it up in several layers of information.
Think of it like a bunch of envelopes inside each other, Russian-doll style:
[Ethernet Header]
[IP Header]
[TCP Header]
PRIVMSG #42 :Hello!


Each layer adds its own info:
- **Ethernet/WiFi:** “This goes to my router.”
- **IP:** “Destination = 142.250.184.206.”
- **TCP:** “This is part 3 of 5, don’t lose it!”
- **Payload:** Your actual message.

---

### 🪜 The 5-Layer Model (aka The Great Stack)

Here’s how your data climbs down and back up the network stack:

| Layer | Example Protocols | What It Does |
|--------|------------------|---------------|
| **Application** | HTTP, FTP, IRC | What you actually care about |
| **Transport** | TCP, UDP | Splits and reassembles data |
| **Network** | IP | Routes data across networks |
| **Link** | Ethernet, Wi-Fi | Local delivery on your LAN |
| **Physical** | Copper, Fiber, Radio | Sends the actual bits (1s and 0s) |

### Data Flow

Application: "PRIVMSG #42 :Hello!"
↓
Transport: TCP adds sequence + reliability
↓
Network: IP adds source/destination addresses
↓
Link: Ethernet adds MAC addresses
↓
Physical: Shoots electrons / light / radio waves

ach header just adds routing info to make sure the right data gets to the right place.
Each layer only knows about its own job and can only talk to the layers above or below it.  
That’s what makes the Internet modular and beautiful.

---

### 🛰️ A Packet’s Wild Ride
+-----------+     +-----------+     +-----------+     +-----------+
|  Laptop   | --> |   Router  | --> |    ISP    | --> |  IRC Host |
|192.168.1.5|     |           |     | Backbone  |     | 142.250.x |
+----------+      +-----------+     +-----------+     +-----------+

At each stop:
- Routers peek at the **destination IP**
- Forward the packet closer to its goal
- Until it finally hits the server

The server unwraps the packet layer by layer, reads your message, and replies.

---







