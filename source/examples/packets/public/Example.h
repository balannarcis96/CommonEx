#pragma once

#include <CommonEx.h>

#include <iostream>

using namespace CommonEx;

enum ClientOpcodes : TPacketOpcode {
	ClientOpcodes_ClientLogin,
	ClientOpcodes_ClientGetCharacters,
	ClientOpcodes_ClientSelectCharacter,
	ClientOpcodes_ServerSelectCharacter,
};

//We name the packet %Client% so we may use it in the client source code, as it is a client->server packet(its only built by the client to be sent to the server, so a "Client" packet)
//We name the packet %Server% ------------------------server-------------------------server->client------------------------------server-------------------client-------"Server"-------)

//Dynamic size packet 
struct P_ClientLoginData : ExternalPacket<ClientOpcodes_ClientLogin> {
	uint32_t			ClientVersion{ 0 };
	uint64_t			ServerTime{ 0 };
	std::string_view	Username;
	std::string_view	Ticket;
};

struct P_ClientLogin : PacketContext<P_ClientLogin, P_ClientLoginData>
{
	//If we have a big packet we might want to precalculate the size of the packet so the system can allocate the right size buffer
	// [default returns  -1]
	//inline int64_t GetPacketSize() const noexcept {
	//	
	//	//if we return -1 here the default size buffer will be allocate, so this packet should fit inside it otherwise it will fail
	//	return -1; 
	//}

	RStatus BuildPacket(TStream& Stream) const noexcept 
	{
		//########## Fixed part of the packet body
		//write the packet body
		Stream.Write(Data.ClientVersion);

		//write the offsets to the strings
		auto& UsernameRef = Stream.WriteStringRef();
		auto& TicketRef = Stream.WriteStringRef();

		//write some other data before we write the dynamic part
		Stream.Write(Data.ServerTime);
		//################################

		//########## Dynamic part of the packet body [strings, arrays etc]

		//wr "try" here so the system may catch the failure 
		R_TRY(Stream.WriteString(UsernameRef, Data.Username.data())) {}
		R_TRY(Stream.WriteString(TicketRef, Data.Ticket.data())) {}

		//################################

		return RSuccess;
	}
};

//Usage 1
void Usage1() {

	constexpr auto SizeDiff = sizeof(P_ClientLogin) - sizeof(P_ClientLoginData);
	//8bytes - the contexts adds just an additional 8bytes to the stack space :D

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