# ft_irc — Internet Relay Chat  
**Subject (Version 9.1)**

> Converted from the official ft_irc PDF subject into Markdown for easier reading, searching, and versioning.

---

## Summary

This project is about creating your own **IRC server**.  
You will use a real IRC client to connect to your server and test it.

The Internet is governed by standard protocols; understanding them is essential. fileciteturn1file0

---

## Table of Contents

1. Introduction  
2. General Rules  
3. AI Instructions  
4. Mandatory Part  
   - Requirements  
   - macOS-specific rules  
   - Test example  
5. Bonus Part  
6. Submission & Peer Evaluation  

---

## 1. Introduction

Internet Relay Chat (IRC) is a text-based communication protocol that allows real-time messaging.

- Public and private messages  
- Group channels  
- Clients connect to servers  
- Servers form IRC networks  

---

## 2. General Rules

- Your program **must never crash**, even on memory exhaustion  
- Unexpected quit = **grade 0**
- Provide a Makefile that avoids unnecessary relinking
- Required Makefile rules:
  - `$(NAME)`, `all`, `clean`, `fclean`, `re`
- Compile with:
  - `-Wall -Wextra -Werror`
- Code must comply with **C++98**
- Prefer C++ headers (`<cstring>` over `<string.h>`)
- **No external libraries**, no Boost

---

## 3. AI Instructions

### Context
AI tools can help but must be used critically. Generated code or explanations may be wrong or incomplete.

### Main Message
- Use AI to reduce repetitive work
- Improve prompting skills
- Understand AI risks and biases
- Work with peers
- Only use AI output you fully understand

### Learner Rules
- Explore AI tools ethically
- Think before prompting
- Review and test AI output
- Always seek peer review

**Bad practice:** copying AI-generated code without understanding  
**Good practice:** using AI as support, then reviewing with peers

---

## 4. Mandatory Part

### Program

- **Executable name:** `ircserv`
- **Language:** C++98
- **Client:** you must NOT implement an IRC client
- **Server-to-server communication:** forbidden

### Usage
```bash
./ircserv <port> <password>
```

- `port` — listening port
- `password` — required for client connections

### Authorized Functions

Networking & system calls:
- `socket`, `close`, `setsockopt`, `bind`, `listen`, `accept`
- `send`, `recv`, `poll` (or equivalent)
- `fcntl`, `lseek`, `fstat`
- `signal`, `sigaction`, signal helpers
- Address helpers: `getaddrinfo`, `inet_ntoa`, etc.

Everything must remain **C++98 compliant**.

---

## 4.1 Requirements

- Handle **multiple clients simultaneously**
- **Forking is prohibited**
- All I/O must be **non-blocking**
- Only **ONE `poll()` (or equivalent)** is allowed
- Any read/write without poll readiness = **grade 0**
- TCP/IP only (IPv4 or IPv6)
- Choose a **reference IRC client** for evaluation

### Mandatory Features

Clients must be able to:
- Authenticate
- Set nickname and username
- Join channels
- Send and receive private messages

Server must support:
- Channel message broadcasting
- Operators and regular users

### Operator Commands

- `KICK` — remove a user from channel
- `INVITE` — invite user to channel
- `TOPIC` — view or change channel topic
- `MODE` — channel modes:
  - `i` invite-only
  - `t` topic restricted to operators
  - `k` channel key (password)
  - `o` give/remove operator
  - `l` user limit

Clean code is expected.

---

## 4.2 macOS Only

macOS handles `write()` differently.

- You **must** use non-blocking file descriptors
- Allowed usage:
```c
fcntl(fd, F_SETFL, O_NONBLOCK);
```
- Any other flag is forbidden

---

## 4.3 Test Example

You must correctly handle:
- Partial packets
- Low bandwidth
- Fragmented commands

Example:
```bash
nc -C 127.0.0.1 6667
com^Dman^Dd
```

Commands may arrive split across packets and must be reassembled before parsing.

---

## 5. Bonus Part

Optional features (only evaluated if mandatory is PERFECT):
- File transfer
- A bot

If **any** mandatory requirement is missing or broken, the bonus is **ignored**.

---

## 6. Submission & Peer Evaluation

- Submit via your Git repository
- Only repository content is evaluated
- You are encouraged to write local tests
- Reference client will be used during evaluation
- Evaluators may ask you to:
  - Modify behavior
  - Add a small feature
  - Explain part of your code

You must demonstrate **real understanding** of your implementation.

---

## Notes

- Evaluation details may vary
- Be prepared to explain:
  - poll usage
  - buffering
  - command handling
  - cleanup logic

---

**End of subject**
