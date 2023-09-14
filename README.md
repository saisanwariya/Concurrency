# Concurrency

---

# Academic Integrity Statement

Please note that all work included in this project is the original work of the author, and any external sources or references have been properly cited and credited. It is strictly prohibited to copy, reproduce, or use any part of this work without permission from the author.

If you choose to use any part of this work as a reference or resource, you are responsible for ensuring that you do not plagiarize or violate any academic integrity policies or guidelines. The author of this work cannot be held liable for any legal or academic consequences resulting from the misuse or misappropriation of this work.

Any unauthorized copying or use of this work may result in serious consequences, including but not limited to academic penalties, legal action, and damage to personal and professional reputation. Therefore, please use this work only as a reference and always ensure that you properly cite and attribute any sources or references used.

--

## Overview

This project implements a channel in C, which is a synchronization mechanism for concurrent programming using message passing. The channel acts as a queue or buffer for messages with a fixed maximum size. Threads can send messages (senders) to the channel, and other threads can receive messages (receivers) from the channel. The channel supports both blocking and non-blocking modes for sending and receiving messages.

## Program Functionality

The program consists of several functions that allow the creation, manipulation, and destruction of channels. The main functions provided are:

- `channel_t* channel_create(size_t size)`: Creates a new channel with the specified size.
- `enum channel_status channel_send(channel_t* channel, void* data)`: Sends a message to the channel (blocking).
- `enum channel_status channel_receive(channel_t* channel, void** data)`: Receives a message from the channel (blocking).
- `enum channel_status channel_non_blocking_send(channel_t* channel, void* data)`: Sends a message to the channel (non-blocking).
- `enum channel_status channel_non_blocking_receive(channel_t* channel, void** data)`: Receives a message from the channel (non-blocking).
- `enum channel_status channel_close(channel_t* channel)`: Closes the channel for further sends.
- `enum channel_status channel_destroy(channel_t* channel)`: Destroys the channel.
- `enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index)`: Performs a select operation on multiple channels.

## Software Setup

To set up and run this program on a Linux system, follow these steps:

1. Clone the GitHub repository containing the project:
   ```
   git clone https://github.com/your-username/concurrencylab.git
   ```

2. Navigate to the project directory:
   ```
   cd concurrencylab
   ```

3. Compile the program by running the following command:
   ```
   make
   ```

4. Run the provided tests to ensure your code's correctness:
   ```
   make test
   ```

   This will compile the code in release mode and run the test suite to evaluate your implementation.

## Running the Program

To run the program, you can use the following command:

```
./channel test_case_name iters
```

- Replace `test_case_name` with the name of the test case you want to run (or omit it to run all tests).
- Replace `iters` with the number of times you want to run the test (default is 1).

For example, to run a specific test called "my_test" five times, use the following command:

```
./channel my_test 5
```

## Notes

- Please ensure that you adhere to the academic integrity statement provided at the beginning of this document.
- You are encouraged to explore the code and understand the implementation details to better grasp the concepts of concurrent programming and channels.