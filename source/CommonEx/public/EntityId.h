#pragma once
/**
 * @file EntityId.h
 *
 * @brief EntityId abstraction
 *			Note: This is the EntityId layout I use in my own projects, it lets for O(1) retrival of resources
 *			Hint: Add/remove types and layouts as needed
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx {
	enum class EEntityType : uint8_t {
		Connection,
		Object,

		//Add here more entities types
	};

	enum class EEntitySubType : uint8_t {
		Player,
		DiagnosticTool,
		AdminTool,
		WorldServerNode,

		//Add here more entities sub types
	};

	using EntityContextId = uint16_t;
	using EntityIndex = uint16_t;
	using EntitySubType = int16_t;

	struct EntityId {

#pragma region Layouts
		struct ObjectIdLayout {
			EEntityType				EntityType;
			uint8_t					Unused;

			union {
				EntityContextId		TLSId;			//Index in WolrdServer WorderCreature TLS array of each Worker thread
				EntityContextId		ObjectId;		//Treated as "Global Object UID"
			} TLSLocation;

			struct {
				union {
					EntityIndex		PoolIndex;		//Pool Index ECreatureType
					EntitySubType	SubType;
				} Outer;

				EntityIndex			Index;			//Index into the origin pool 
			} MemoryLocation;
		};
		struct ConnectionIdLayout {
			EEntityType				EntityType;
			EEntitySubType			ConnectionType;

			union {
				EntityContextId		TLSId;			//Index in WolrdServer WorderPlayer TLS array of each Worker thread
				EntityContextId		ConnectionId;	//Connection object Index
				EntityContextId		PlayerId;		//Treated as "Global Player UID"
			} Location;

			SGUID					SessionId;		//Session GUID
		};
#pragma endregion

		union {
			//Layouts
			ObjectIdLayout		ObjectId;
			ConnectionIdLayout	ConnectionId;

			//Whole
			uint64_t			Id;
		};

		EntityId() : Id(0) { }
		EntityId(uint64_t Id) : Id(Id) { }

		EntityId(const EntityId& Other) : Id(Other.Id) { }
		EntityId& operator=(const EntityId& Other) {
			Id = Other.Id;

			return *this;
		}

		inline static EntityId CreateConnection(EEntitySubType	ConnectionType, EntityContextId ConnectionId, SGUID SessionId) noexcept {
			EntityId NewId;

			NewId.ConnectionId.EntityType = EEntityType::Connection;
			NewId.ConnectionId.ConnectionType = ConnectionType;
			NewId.ConnectionId.Location.ConnectionId = ConnectionId;
			NewId.ConnectionId.SessionId = SessionId;

			return NewId;
		}

		bool IsPlayer() const noexcept {
			return false;
		}

		static EntityId None;

		static_assert(sizeof(ObjectIdLayout) == sizeof(uint64_t), "EntityId::Invalid ObjectIdLayout type size!");
		static_assert(sizeof(ConnectionIdLayout) == sizeof(uint64_t), "EntityId::Invalid ConnectionIdLayout type size!");
	};

	inline EntityId EntityId::None{ 0 };
}
