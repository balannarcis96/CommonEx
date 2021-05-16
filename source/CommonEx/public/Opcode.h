#pragma once
/**
 * @file Opcode.h
 *
 * @brief CommonEx Protocol opcode values
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx {
	enum Opcode : TPacketOpcode {
		//Default opcode for stream packets (packet containing more sub packets)
		Opcode_STREAM_PACKET = 1,

		//Default opcode for boardcast packets
		Opcode_BROADCAST_PACKET = 2,

		//Default opcode for boardcast packets
		Opcode_ROUTED_PACKET = 3,

		//Lobby
		C_LOGIN_ARBITER,
		S_LOGIN_ARBITER,
		S_USER_SETTINGS,
		C_GET_USER_SETTINGS,
		C_SAVE_USER_SETTINGS,
		C_GET_CHARACTER_LIST,
		S_CHARACTER_LIST,
		C_CAN_CREATE_CHARACTER,
		S_CAN_CREATE_CHARACTER,
		C_CREATE_CHARACTER,
		S_CREATE_CHARACTER,
		C_UPDATE_CHARACTER_LOBBY_INDEX,
		C_SELECT_CHARACTER,
		S_SELECT_CHARACTER,

		//Enter world process
		S_ENTER_WORLD_START,
		S_SPAWN_ME,
		S_LOAD_TOPO,
		C_ENTER_WORLD_FIN,
		S_ENTER_WORLD_FIN,
		
		//Spawn
		S_SPAWN_PLAYER,
		S_SPAWN_NPC,
		S_SPAWN_OBJECT,
		S_SPAWN_COLLECT_OBJECT,
		S_SPAWN_PET,
		S_SPAWN_VILLAGER, //Simmilar to S_SPAWN_NPC but for non interactive npcs (more optimized)


		//World
		C_PLAYER_LOCATION,
		C_UNEQUIP_ITEM,
		C_MOVE_ITEM,

		Opcode_MAX = COpcodeMaxValue
	};
}
