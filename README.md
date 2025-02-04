# 📌 Interactive Forum in C

This project recreates an **interactive forum** based on **Operating Systems** principles, where users can create topics, interact with each other, and an administrator manages the platform.

## 📂 Project Structure
- **feed.c** → Client-side logic, allowing users to send messages and interact with topics.
- **manager.c** → Server-side logic, responsible for managing users and topics.
- **util.h** → Definitions and data structures used in the project.

## 🛠️ Technologies Used
- **C Programming** 🖥️
- **FIFOs and Inter-Process Communication (IPC)** 🔄
- **Multi-threading for parallel processing** ⚡
- **`select()` mechanism for I/O multiplexing** 📡

## 🚀 Features
### 📌 Client (feed)
- 📩 **msg** → Sends a message to a topic.
- 📌 **subscribe** → Subscribes to an existing topic.
- ❌ **unsubscribe** → Unsubscribes from a topic.
- 📜 **topics** → Lists all available topics.
- 🚪 **exit** → Exits the platform.

### 🔧 Administrator (manager)
- 📜 **topics** → Lists all topics.
- 🔒 **lock** → Blocks a topic (prevents message sending).
- 🔓 **unlock** → Unblocks a previously locked topic.
- 👀 **show** → Displays detailed information about a topic.
- 🧹 **remove** → Removes a user from the platform.
- 🧑‍💻 **users** → Displays the list of connected users.
- 🛑 **close** → Shuts down the server, removing all users and saving persistent messages.


