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

		//Add here custom opcodes

		Opcode_MAX = COpcodeMaxValue
	};
}
