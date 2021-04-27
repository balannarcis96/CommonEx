# packets usage example

# TODO: write a node js module to generate the packet data and contexts from some decriptor files !
```
# protocol descriptor file format example
.ProtocolVersion=1.0
.Version=1.4

.opcodes ClientOpcodes
{
	Client_Login,
	Server_CharacterList
	# etc
}

.struct Character
{
	int32  				Id;
	wstring				Name;
	int32				GuildId;
	int32				ZoneId;
	int32				ContinentId;
	float				Height;
	# etc
}

.packet ServerUserList<Server_CharacterList> 
{
	bool				bIsVip
	Array<Character> 	Characters
}

.packet ClientLogin<ClientLogin> 
{
	int32 				Version
	wstring				Username
	wstring				PasswordHash
	uint64				ClientTime;
}

```

```cpp
//Usage 1
void Usage1() {
	P_ClientLogin Packet;

	Packet->Version = 23;
	Packet->Username = "MyUsername";
	Packet->Ticket = "My hashed password";
	Packet->ClientTime = (uint64_t)time(NULL);

	//Send(Packet); //the send function will actually allocate the stream and call BuildPacket to build the packet into the allocated stream
}

```
