#### Example

> ./newudpl -v -p 20002:20003 -i 127.0.0.1:20001  -o 127.0.0.1:20000 -L 1 -d 2 -B 1 -O 1

> ./sender newudpl-1.4.tar.gz 127.0.0.1 20002 20001 sendlog.txt 7

> ./receiver file.gz 20000 127.0.0.1 20001 recvlog.txt

```
Delivery completed successfully
Total bytes sent = 119658
Segments sent = 210
Segments retransmitted = 44.76 %
```

_Compiled successfully under gcc version 4.8.3_.

#### Description


1. TCP segment structure

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Source Port          |       Destination Port        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Acknowledgment Number                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Data |           |U|A|P|R|S|F|                               |
   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
   |       |           |G|K|H|T|N|N|                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Checksum            |         Urgent Pointer        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             data                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

 > [RFC 793 - Transmission Control Protocol](https://tools.ietf.org/html/rfc793#section-3.1)



2. Sender states

 See the FSM in lecture slide `Transport Layer 3-68`.

 The sender has only one state, where it waits for three events, `SENDDATA`, `ACK`, `TIMEOUT`.
 Since this is a sending-file program, the FSM can "asks for" instead of "waiting for" data, and it also has knowledge about whether there is window available.
 Hence, I restrict events sequence to patterns below (check comments in `sender.cpp` for more details):

 > TIMEOUT -> WAIT
 > SENDDATA -> WAIT
 > WAIT -> ACK / TIMEOUT
 > ACK -> (WAIT) -> SENDDATA
 > ACK -> WAIT

3. Receiver states

 See the FSM in lecture slide `Transport Layer 3-50`.

 The receiver is ACK-only. It only send ACK for correctly-received pkt with highest in-order sequence number.


4. The loss recovery mechanism

 Receiver uses accumulative ACKs. It has no buffer, so it simply drops all out-of-order and corrupted packets, and reACK the packet.



#### Issues

1. Since the receiver has no buffer, and each TCP packet has a size of 576bytes (defined by `TCP_DATA_SIZE` in `tcpstate.h`), it can take a good amount of time to tranfer the file under poor network conditions.

2. The computation of RTT follows [RFC 6298 - Computing TCP's Retransmission Timer](https://tools.ietf.org/html/rfc6298). Retransmission packet is not used in calculating estimated RTT. Therefore, under poor network conditions, it's likely that RTT is never estimated and remains default, which is 0.

3. Closing the connection is a 2-way handshake.

4. "Connected: Hello, receiver!" is output to confirm whether the TCP path is set up.

4. `pack.h` from [Beej's Guide to Network Programming](http://beej.us/guide/bgnet/) is included for `pack()` and `unpack()`.

5. Sequence number is not bounded. It corresponds to the index of file chunks.

6. Restart pattern of timer is not the same as that in lecture slide.






