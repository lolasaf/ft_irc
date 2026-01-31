# FT_IRC — Evaluation Scale

> **Project:** ft_irc  
> **Context:** 42 School — Peer Evaluation  

---

## General Instructions

- Remain polite, courteous, respectful, and constructive.
- Identify possible dysfunctions and discuss them openly.
- Keep an open mind — interpretations of the subject may vary.
- Peer evaluation must be done seriously and honestly.

---

## Evaluation Guidelines

- Only grade the work present in the submitted Git repository.
- Verify repository ownership and correctness.
- Ensure `git clone` works in an empty directory.
- Check for malicious aliases or misleading setups.
- Review any grading or automation scripts together.

---

## Automatic Failure Conditions

Evaluation ends immediately with grade **0** (or **–42 for cheating**) if:
- Repository is empty
- Program does not compile
- Norm errors are present
- Cheating is detected

---

## Runtime Rules

During defense:
- No segfaults
- No unexpected crashes
- No uncontrolled termination

If any occur → **final grade = 0**

---

## Memory Management

- All heap allocations must be freed
- No memory leaks allowed
- Tools: valgrind, leaks, e_fence

---

## Mandatory Part

### Basic Checks

- Makefile exists
- Compiles correctly
- Written in C++
- Executable name: `ircserv`

### poll() Rules

- Only one poll() (or equivalent)
- Must be called before accept/read/write
- `errno` must not be used for logic

### fcntl()

```c
fcntl(fd, F_SETFL, O_NONBLOCK);
```

Any other usage is forbidden.

---

## Networking

- Server listens on all interfaces
- Port from command-line argument
- nc connectivity works
- Reference IRC client works
- Multiple simultaneous clients supported
- Channels broadcast messages correctly

---

## Networking Edge Cases

- Partial commands via nc
- Unexpected client termination
- Half-sent commands
- Suspended client flood test
- No server hang, no leaks

---

## Client Commands

- Authentication
- Nickname / username
- Join channel
- PRIVMSG
- NOTICE

---

## Finish Evaluation

- User permissions respected
- Operator commands work
- Deduct 1 point per broken feature
- Final rating: 0–5

---

## Bonus

- File transfer works
- IRC bot present

---

## Final Flags

- Ok
- Outstanding
- Empty work
- Invalid compilation
- Crash
- Cheat
- Incomplete group
- Leaks
- Forbidden function

---

## Conclusion

Leave a final evaluator comment.
