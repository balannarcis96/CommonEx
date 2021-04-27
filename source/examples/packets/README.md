# packets usage example

```cpp
//Usage 1
void Usage1() {
	P_ClientLogin Packet;

	Packet->ClientVersion = 23;
	Packet->Username = "MyUsername";
	Packet->Ticket = "My hashed password";

	//Send(Packet); //the send function will actually allocate the stream and call BuildPacket to build the packet into the allocated stream
}

//Usage 2
void Usage2() {

	constexpr auto SizeDiff = sizeof(P_ServerSelectCharacter) - sizeof(P_ServerSelectCharacterData);
	//8bytes - the contexts adds just an additional 8bytes to the stack space :D

	P_ServerSelectCharacter Packet;

	Packet->CharacterId = 2;
	Packet->bDoSomething = TRUE;

	//Send(Packet); //the send function will actually allocate the stream and call BuildPacket to build the packet into the allocated stream
}
```
