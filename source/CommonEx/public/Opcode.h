#pragma once

namespace CommonEx {
	enum Opcode : TPacketOpcode {
		//Default opcode for stream packets (packet containing more sub packets)
		Opcode_STREAM_PACKET = 1,

		//Default opcode for boardcast packets
		Opcode_BROADCAST_PACKET = 2,

		//Default opcode for boardcast packets
		Opcode_ROUTED_PACKET = 3,

		Opcode_MAX = COpcodeMaxValue
	};
}
