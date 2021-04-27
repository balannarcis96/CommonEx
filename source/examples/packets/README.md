# packets usage example

# TODO: write a node js module to generate the packet data and contexts from some decriptor files !

```cpp
//Usage 1
void Usage1() {
	P_ClientLogin Packet;

	Packet->ClientVersion = 23;
	Packet->Username = "MyUsername";
	Packet->Ticket = "My hashed password";

	//Send(Packet); //the send function will actually allocate the stream and call BuildPacket to build the packet into the allocated stream
}

//Fixed size packet
struct P_ServerSelectCharacterData: ExternalPacket<ClientOpcodes_ServerSelectCharacter> {
	EntityId		CharacterId{ 0 };
	uint32_t		bDoSomething{ 0 };
};
//FixedBodyPacketContext knows how to build the packet, no need for our input here
struct P_ServerSelectCharacter : FixedBodyPacketContext<P_ServerSelectCharacterData> {};

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
