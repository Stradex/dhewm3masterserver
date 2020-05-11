/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "idlib/LangDict.h"
#include "framework/async/AsyncNetwork.h"

const int MIN_RECONNECT_TIME			= 2000;
const int EMPTY_RESEND_TIME				= 500;
const int PING_RESEND_TIME				= 500;
const int NOINPUT_IDLE_TIME				= 30000;

const int HEARTBEAT_MSEC				= 5*60*1000;

// must be kept in sync with authReplyMsg_t
const char* authReplyMsg[] = {
	//	"Waiting for authorization",
	"#str_07204",
	//	"Client unknown to auth",
	"#str_07205",
	//	"Access denied - CD Key in use",
	"#str_07206",
	//	"Auth custom message", // placeholder - we propagate a message from the master
	"#str_07207",
	//	"Authorize Server - Waiting for client"
	"#str_07208"
};

const char* authReplyStr[] = {
	"AUTH_NONE",
	"AUTH_OK",
	"AUTH_WAIT",
	"AUTH_DENY"
};

/*
==================
idAsyncServer::idAsyncServer
==================
*/
idAsyncServer::idAsyncServer( void ) {
	int i;

	active = false;
	realTime = 0;
	serverTime = 0;
	serverId = 0;
	serverDataChecksum = 0;
	localClientNum = -1;
	gameInitId = 0;
	gameFrame = 0;
	gameTime = 0;
	gameTimeResidual = 0;
	memset( challenges, 0, sizeof( challenges ) );
	memset( userCmds, 0, sizeof( userCmds ) );
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		ClearClient( i );
	}
	serverReloadingEngine = false;
	nextHeartbeatTime = 0;
	nextAsyncStatsTime = 0;
	noRconOutput = true;
	lastAuthTime = 0;

	memset( stats_outrate, 0, sizeof( stats_outrate ) );
	stats_current = 0;
	stats_average_sum = 0;
	stats_max = 0;
	stats_max_index = 0;
}

/*
==================
idAsyncServer::InitPort
==================
*/
bool idAsyncServer::InitPort( void ) {
	int lastPort;

	// if this is the first time we have spawned a server, open the UDP port
	if ( !serverPort.GetPort() ) {
		if ( cvarSystem->GetCVarInteger( "net_port" ) != 0 ) {
			if ( !serverPort.InitForPort( cvarSystem->GetCVarInteger( "net_port" ) ) ) {
				common->Printf( "Unable to open server on port %d (net_port)\n", cvarSystem->GetCVarInteger( "net_port" ) );
				return false;
			}
		} else {
			// scan for multiple ports, in case other servers are running on this IP already
			for ( lastPort = 0; lastPort < NUM_SERVER_PORTS; lastPort++ ) {
				if ( serverPort.InitForPort( PORT_SERVER + lastPort ) ) {
					break;
				}
			}
			if ( lastPort >= NUM_SERVER_PORTS ) {
				common->Printf( "Unable to open server network port.\n" );
				return false;
			}
		}
	}

	return true;
}

/*
==================
idAsyncServer::ClosePort
==================
*/
void idAsyncServer::ClosePort( void ) {
	int i;

	serverPort.Close();
	for ( i = 0; i < MAX_CHALLENGES; i++ ) {
		challenges[ i ].authReplyPrint.Clear();
	}
}

/*
==================
idAsyncServer::Spawn
==================
*/
void idAsyncServer::Spawn( void ) {
	int			i, size;
	byte		msgBuf[MAX_MESSAGE_SIZE];
	netadr_t	from;

	if ( active ) {
		return;
	}

	if ( !InitPort() ) {
		return;
	}

	// trash any currently pending packets
	while( serverPort.GetPacket( from, msgBuf, size, sizeof( msgBuf ) ) ) {
	}

	// reset cheats cvars
	if ( !idAsyncNetwork::allowCheats.GetBool() ) {
		cvarSystem->ResetFlaggedVariables( CVAR_CHEAT );
	}

	memset( challenges, 0, sizeof( challenges ) );
	memset( userCmds, 0, sizeof( userCmds ) );
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		ClearClient( i );
	}

	common->Printf( "Server spawned on port %i.\n", serverPort.GetPort() );

	// get a pseudo random server id, but don't use the id which is reserved for connectionless packets
	serverId = Sys_Milliseconds() & CONNECTIONLESS_MESSAGE_ID_MASK;

	active = true;

	nextHeartbeatTime = 0;
	nextAsyncStatsTime = 0;

	ExecuteMapChange();
}

/*
==================
idAsyncServer::Kill
==================
*/
void idAsyncServer::Kill( void ) {
	int i, j;

	if ( !active ) {
		return;
	}

	// drop all clients
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		DropClient( i, "#str_07135" );
	}

	// send some empty messages to the zombie clients to make sure they disconnect
	for ( j = 0; j < 4; j++ ) {
		for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
			if ( clients[i].clientState == SCS_ZOMBIE ) {
				if ( clients[i].channel.UnsentFragmentsLeft() ) {
					clients[i].channel.SendNextFragment( serverPort, serverTime );
				} else {
					SendEmptyToClient( i, true );
				}
			}
		}
		Sys_Sleep( 10 );
	}

	// reset any pureness
	fileSystem->ClearPureChecksums();

	active = false;

}

/*
==================
idAsyncServer::ExecuteMapChange
==================
*/
void idAsyncServer::ExecuteMapChange( void ) {

	// serverTime gets reset, force a heartbeat so timings restart
	MasterHeartbeat( true );
}

/*
==================
idAsyncServer::GetPort
==================
*/
int idAsyncServer::GetPort( void ) const {
	return serverPort.GetPort();
}

/*
===============
idAsyncServer::GetBoundAdr
===============
*/
netadr_t idAsyncServer::GetBoundAdr( void ) const {
	return serverPort.GetAdr();
}

/*
==================
idAsyncServer::GetOutgoingRate
==================
*/
int idAsyncServer::GetOutgoingRate( void ) const {
	int i, rate;

	rate = 0;
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		const serverClient_t &client = clients[i];

		if ( client.clientState >= SCS_CONNECTED ) {
			rate += client.channel.GetOutgoingRate();
		}
	}
	return rate;
}

/*
==================
idAsyncServer::GetIncomingRate
==================
*/
int idAsyncServer::GetIncomingRate( void ) const {
	int i, rate;

	rate = 0;
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		const serverClient_t &client = clients[i];

		if ( client.clientState >= SCS_CONNECTED ) {
			rate += client.channel.GetIncomingRate();
		}
	}
	return rate;
}

/*
==================
idAsyncServer::IsClientInGame
==================
*/
bool idAsyncServer::IsClientInGame( int clientNum ) const {
	return ( clients[clientNum].clientState >= SCS_INGAME );
}

/*
==================
idAsyncServer::GetClientPing
==================
*/
int idAsyncServer::GetClientPing( int clientNum ) const {
	const serverClient_t &client = clients[clientNum];

	if ( client.clientState < SCS_CONNECTED ) {
		return 99999;
	} else {
		return client.clientPing;
	}
}

/*
==================
idAsyncServer::GetClientPrediction
==================
*/
int idAsyncServer::GetClientPrediction( int clientNum ) const {
	const serverClient_t &client = clients[clientNum];

	if ( client.clientState < SCS_CONNECTED ) {
		return 99999;
	} else {
		return client.clientPrediction;
	}
}

/*
==================
idAsyncServer::GetClientTimeSinceLastPacket
==================
*/
int idAsyncServer::GetClientTimeSinceLastPacket( int clientNum ) const {
	const serverClient_t &client = clients[clientNum];

	if ( client.clientState < SCS_CONNECTED ) {
		return 99999;
	} else {
		return serverTime - client.lastPacketTime;
	}
}

/*
==================
idAsyncServer::GetClientTimeSinceLastInput
==================
*/
int idAsyncServer::GetClientTimeSinceLastInput( int clientNum ) const {
	const serverClient_t &client = clients[clientNum];

	if ( client.clientState < SCS_CONNECTED ) {
		return 99999;
	} else {
		return serverTime - client.lastInputTime;
	}
}

/*
==================
idAsyncServer::GetClientOutgoingRate
==================
*/
int idAsyncServer::GetClientOutgoingRate( int clientNum ) const {
	const serverClient_t &client = clients[clientNum];

	if ( client.clientState < SCS_CONNECTED ) {
		return -1;
	} else {
		return client.channel.GetOutgoingRate();
	}
}

/*
==================
idAsyncServer::GetClientIncomingRate
==================
*/
int idAsyncServer::GetClientIncomingRate( int clientNum ) const {
	const serverClient_t &client = clients[clientNum];

	if ( client.clientState < SCS_CONNECTED ) {
		return -1;
	} else {
		return client.channel.GetIncomingRate();
	}
}

/*
==================
idAsyncServer::GetClientOutgoingCompression
==================
*/
float idAsyncServer::GetClientOutgoingCompression( int clientNum ) const {
	const serverClient_t &client = clients[clientNum];

	if ( client.clientState < SCS_CONNECTED ) {
		return 0.0f;
	} else {
		return client.channel.GetOutgoingCompression();
	}
}

/*
==================
idAsyncServer::GetClientIncomingCompression
==================
*/
float idAsyncServer::GetClientIncomingCompression( int clientNum ) const {
	const serverClient_t &client = clients[clientNum];

	if ( client.clientState < SCS_CONNECTED ) {
		return 0.0f;
	} else {
		return client.channel.GetIncomingCompression();
	}
}

/*
==================
idAsyncServer::GetClientIncomingPacketLoss
==================
*/
float idAsyncServer::GetClientIncomingPacketLoss( int clientNum ) const {
	const serverClient_t &client = clients[clientNum];

	if ( client.clientState < SCS_CONNECTED ) {
		return 0.0f;
	} else {
		return client.channel.GetIncomingPacketLoss();
	}
}

/*
==================
idAsyncServer::GetNumClients
==================
*/
int idAsyncServer::GetNumClients( void ) const {
	int ret = 0;
	for ( int i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		if ( clients[ i ].clientState >= SCS_CONNECTED ) {
			ret++;
		}
	}
	return ret;
}

/*
==================
idAsyncServer::GetNumIdleClients
==================
*/
int idAsyncServer::GetNumIdleClients( void ) const {
	int ret = 0;
	for ( int i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		if ( clients[ i ].clientState >= SCS_CONNECTED ) {
			if ( serverTime - clients[ i ].lastInputTime > NOINPUT_IDLE_TIME ) {
				ret++;
			}
		}
	}
	return ret;
}

/*
==================
idAsyncServer::DuplicateUsercmds
==================
*/
void idAsyncServer::DuplicateUsercmds( int frame, int time ) {
	int i, previousIndex, currentIndex;

	previousIndex = ( frame - 1 ) & ( MAX_USERCMD_BACKUP - 1 );
	currentIndex = frame & ( MAX_USERCMD_BACKUP - 1 );

	// duplicate previous user commands if no new commands are available for a client
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		if ( clients[i].clientState == SCS_FREE ) {
			continue;
		}

		if ( idAsyncNetwork::DuplicateUsercmd( userCmds[previousIndex][i], userCmds[currentIndex][i], frame, time ) ) {
			clients[i].numDuplicatedUsercmds++;
		}
	}
}

/*
==================
idAsyncServer::ClearClient
==================
*/
void idAsyncServer::ClearClient( int clientNum ) {
	serverClient_t &client = clients[clientNum];
	client.clientId = 0;
	client.clientState = SCS_FREE;
	client.clientPrediction = 0;
	client.clientAheadTime = 0;
	client.clientRate = 0;
	client.clientPing = 0;
	client.gameInitSequence = 0;
	client.gameFrame = 0;
	client.gameTime = 0;
	client.channel.Shutdown();
	client.lastConnectTime = 0;
	client.lastEmptyTime = 0;
	client.lastPingTime = 0;
	client.lastSnapshotTime = 0;
	client.lastPacketTime = 0;
	client.lastInputTime = 0;
	client.snapshotSequence = 0;
	client.acknowledgeSnapshotSequence = 0;
	client.numDuplicatedUsercmds = 0;
}

/*
==================
idAsyncServer::InitClient
==================
*/
void idAsyncServer::InitClient( int clientNum, int clientId, int clientRate ) {
	int i;

	// clear the server client
	serverClient_t &client = clients[clientNum];
	client.clientId = clientId;
	client.clientState = SCS_CONNECTED;
	client.clientPrediction = 0;
	client.clientAheadTime = 0;
	client.gameInitSequence = -1;
	client.gameFrame = 0;
	client.gameTime = 0;
	client.channel.ResetRate();
	client.clientRate = clientRate ? clientRate : idAsyncNetwork::serverMaxClientRate.GetInteger();
	client.channel.SetMaxOutgoingRate( Min( idAsyncNetwork::serverMaxClientRate.GetInteger(), client.clientRate ) );
	client.clientPing = 0;
	client.lastConnectTime = serverTime;
	client.lastEmptyTime = serverTime;
	client.lastPingTime = serverTime;
	client.lastSnapshotTime = serverTime;
	client.lastPacketTime = serverTime;
	client.lastInputTime = serverTime;
	client.acknowledgeSnapshotSequence = 0;
	client.numDuplicatedUsercmds = 0;

	// clear the user commands
	for ( i = 0; i < MAX_USERCMD_BACKUP; i++ ) {
		memset( &userCmds[i][clientNum], 0, sizeof( userCmds[i][clientNum] ) );
	}

}

/*
==================
idAsyncServer::InitLocalClient
==================
*/
void idAsyncServer::InitLocalClient( int clientNum ) {
	netadr_t badAddress;

	localClientNum = clientNum;
	InitClient( clientNum, 0, 0 );
	memset( &badAddress, 0, sizeof( badAddress ) );
	badAddress.type = NA_BAD;
	clients[clientNum].channel.Init( badAddress, serverId );
	clients[clientNum].clientState = SCS_INGAME;
}

/*
==================
idAsyncServer::BeginLocalClient
==================
*/
void idAsyncServer::BeginLocalClient( void ) {

}

/*
==================
idAsyncServer::LocalClientInput
==================
*/
void idAsyncServer::LocalClientInput( void ) {
	int index;

	if ( localClientNum < 0 ) {
		return;
	}

	index = gameFrame & ( MAX_USERCMD_BACKUP - 1 );
	userCmds[index][localClientNum] = usercmdGen->GetDirectUsercmd();
	userCmds[index][localClientNum].gameFrame = gameFrame;
	userCmds[index][localClientNum].gameTime = gameTime;
	if ( idAsyncNetwork::UsercmdInputChanged( userCmds[( gameFrame - 1 ) & ( MAX_USERCMD_BACKUP - 1 )][localClientNum], userCmds[index][localClientNum] ) ) {
		clients[localClientNum].lastInputTime = serverTime;
	}
	clients[localClientNum].gameFrame = gameFrame;
	clients[localClientNum].gameTime = gameTime;
	clients[localClientNum].lastPacketTime = serverTime;
}

/*
==================
idAsyncServer::DropClient
==================
*/
void idAsyncServer::DropClient( int clientNum, const char *reason ) {
}

/*
==================
idAsyncServer::SendReliableMessage
==================
*/
void idAsyncServer::SendReliableMessage( int clientNum, const idBitMsg &msg ) {
	if ( clientNum == localClientNum ) {
		return;
	}
	if ( !clients[ clientNum ].channel.SendReliableMessage( msg ) ) {
		clients[ clientNum ].channel.ClearReliableMessages();
		DropClient( clientNum, "#str_07136" );
	}
}

/*
==================
idAsyncServer::CheckClientTimeouts
==================
*/
void idAsyncServer::CheckClientTimeouts( void ) {
	int i, zombieTimeout, clientTimeout;

	zombieTimeout = serverTime - idAsyncNetwork::serverZombieTimeout.GetInteger() * 1000;
	clientTimeout = serverTime - idAsyncNetwork::serverClientTimeout.GetInteger() * 1000;

	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		serverClient_t &client = clients[i];

		if ( i == localClientNum ) {
			continue;
		}

		if ( client.lastPacketTime > serverTime ) {
			client.lastPacketTime = serverTime;
			continue;
		}

		if ( client.clientState == SCS_ZOMBIE && client.lastPacketTime < zombieTimeout ) {
			client.channel.Shutdown();
			client.clientState = SCS_FREE;
			continue;
		}

		if ( client.clientState >= SCS_PUREWAIT && client.lastPacketTime < clientTimeout ) {
			DropClient( i, "#str_07137" );
			continue;
		}
	}
}

/*
==================
idAsyncServer::SendPrintBroadcast
==================
*/
void idAsyncServer::SendPrintBroadcast( const char *string ) {
	int			i;
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteByte( SERVER_RELIABLE_MESSAGE_PRINT );
	msg.WriteString( string );

	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		if ( clients[i].clientState >= SCS_CONNECTED ) {
			SendReliableMessage( i, msg );
		}
	}
}

/*
==================
idAsyncServer::SendPrintToClient
==================
*/
void idAsyncServer::SendPrintToClient( int clientNum, const char *string ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	serverClient_t &client = clients[clientNum];

	if ( client.clientState < SCS_CONNECTED ) {
		return;
	}

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteByte( SERVER_RELIABLE_MESSAGE_PRINT );
	msg.WriteString( string );

	SendReliableMessage( clientNum, msg );
}

/*
==================
idAsyncServer::SendUserInfoBroadcast
==================
*/
void idAsyncServer::SendUserInfoBroadcast( int userInfoNum, const idDict &info, bool sendToAll ) {
}

/*
==================
idAsyncServer::UpdateUI
if the game modifies userInfo, it will call this through command system
we then need to get the info from the game, and broadcast to clients
( using DeltaDict and our current mapSpawnData as a base )
==================
*/
void idAsyncServer::UpdateUI( int clientNum ) {
}

/*
==================
idAsyncServer::SendUserInfoToClient
==================
*/
void idAsyncServer::SendUserInfoToClient( int clientNum, int userInfoNum, const idDict &info ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	if ( clients[clientNum].clientState < SCS_CONNECTED ) {
		return;
	}

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteByte( SERVER_RELIABLE_MESSAGE_CLIENTINFO );
	msg.WriteByte( userInfoNum );
	msg.WriteBits( 0, 1 );

#if ID_CLIENTINFO_TAGS
	msg.WriteInt( 0 );
	common->DPrintf( "user info %d to client %d: NULL base\n", userInfoNum, clientNum );
#endif

	msg.WriteDeltaDict( info, NULL );

	SendReliableMessage( clientNum, msg );
}

/*
==================
idAsyncServer::SendSyncedCvarsBroadcast
==================
*/
void idAsyncServer::SendSyncedCvarsBroadcast( const idDict &cvars ) {
}

/*
==================
idAsyncServer::SendSyncedCvarsToClient
==================
*/
void idAsyncServer::SendSyncedCvarsToClient( int clientNum, const idDict &cvars ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	if ( clients[clientNum].clientState < SCS_CONNECTED ) {
		return;
	}

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteByte( SERVER_RELIABLE_MESSAGE_SYNCEDCVARS );
	msg.WriteDeltaDict( cvars, NULL );

	SendReliableMessage( clientNum, msg );
}

/*
==================
idAsyncServer::SendApplySnapshotToClient
==================
*/
void idAsyncServer::SendApplySnapshotToClient( int clientNum, int sequence ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteByte( SERVER_RELIABLE_MESSAGE_APPLYSNAPSHOT );
	msg.WriteInt( sequence );

	SendReliableMessage( clientNum, msg );
}

/*
==================
idAsyncServer::SendEmptyToClient
==================
*/
bool idAsyncServer::SendEmptyToClient( int clientNum, bool force ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	serverClient_t &client = clients[clientNum];

	if ( client.lastEmptyTime > realTime ) {
		client.lastEmptyTime = realTime;
	}

	if ( !force && ( realTime - client.lastEmptyTime < EMPTY_RESEND_TIME ) ) {
		return false;
	}

	if ( idAsyncNetwork::verbose.GetInteger() ) {
		common->Printf( "sending empty to client %d: gameInitId = %d, gameFrame = %d, gameTime = %d\n", clientNum, gameInitId, gameFrame, gameTime );
	}

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteInt( gameInitId );
	msg.WriteByte( SERVER_UNRELIABLE_MESSAGE_EMPTY );

	client.channel.SendMessage( serverPort, serverTime, msg );

	client.lastEmptyTime = realTime;

	return true;
}

/*
==================
idAsyncServer::SendPingToClient
==================
*/
bool idAsyncServer::SendPingToClient( int clientNum ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	serverClient_t &client = clients[clientNum];

	if ( client.lastPingTime > realTime ) {
		client.lastPingTime = realTime;
	}

	if ( realTime - client.lastPingTime < PING_RESEND_TIME ) {
		return false;
	}

	if ( idAsyncNetwork::verbose.GetInteger() == 2 ) {
		common->Printf( "pinging client %d: gameInitId = %d, gameFrame = %d, gameTime = %d\n", clientNum, gameInitId, gameFrame, gameTime );
	}

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteInt( gameInitId );
	msg.WriteByte( SERVER_UNRELIABLE_MESSAGE_PING );
	msg.WriteInt( realTime );

	client.channel.SendMessage( serverPort, serverTime, msg );

	client.lastPingTime = realTime;

	return true;
}

/*
==================
idAsyncServer::SendGameInitToClient
==================
*/
void idAsyncServer::SendGameInitToClient( int clientNum ) {
}

/*
==================
idAsyncServer::SendSnapshotToClient
==================
*/
bool idAsyncServer::SendSnapshotToClient( int clientNum ) {
	return true;
}

/*
==================
idAsyncServer::ProcessUnreliableClientMessage
==================
*/
void idAsyncServer::ProcessUnreliableClientMessage( int clientNum, const idBitMsg &msg ) {

}

/*
==================
idAsyncServer::ProcessReliableClientMessages
==================
*/
void idAsyncServer::ProcessReliableClientMessages( int clientNum ) {
}

/*
==================
idAsyncServer::ProcessAuthMessage
==================
*/
void idAsyncServer::ProcessAuthMessage( const idBitMsg &msg ) {
	netadr_t		client_from;
	char			client_guid[ 12 ], string[ MAX_STRING_CHARS ];
	int				i, clientId;
	authReply_t		reply;
	authReplyMsg_t	replyMsg = AUTH_REPLY_WAITING;
	idStr			replyPrintMsg;

	reply = (authReply_t)msg.ReadByte();
	if ( reply <= 0 || reply >= AUTH_MAXSTATES ) {
		common->DPrintf( "auth: invalid reply %d\n", reply );
		return;
	}
	clientId = msg.ReadShort( );
	msg.ReadNetadr( &client_from );
	msg.ReadString( client_guid, sizeof( client_guid ) );
	if ( reply != AUTH_OK ) {
		replyMsg = (authReplyMsg_t)msg.ReadByte();
		if ( replyMsg <= 0 || replyMsg >= AUTH_REPLY_MAXSTATES ) {
			common->DPrintf( "auth: invalid reply msg %d\n", replyMsg );
			return;
		}
		if ( replyMsg == AUTH_REPLY_PRINT ) {
			msg.ReadString( string, MAX_STRING_CHARS );
			replyPrintMsg = string;
		}
	}

	lastAuthTime = serverTime;

	// no message parsing below

	for ( i = 0; i < MAX_CHALLENGES; i++ ) {
		if ( !challenges[i].connected && challenges[ i ].clientId == clientId ) {
			// return if something is wrong
			// break if we have found a valid auth
			if ( !strlen( challenges[ i ].guid ) ) {
				common->DPrintf( "auth: client %s has no guid yet\n", Sys_NetAdrToString( challenges[ i ].address ) );
				return;
			}
			if ( idStr::Cmp( challenges[ i ].guid, client_guid ) ) {
				common->DPrintf( "auth: client %s %s not matched, auth server says guid %s\n", Sys_NetAdrToString( challenges[ i ].address ), challenges[i].guid, client_guid );
				return;
			}
			if ( !Sys_CompareNetAdrBase( client_from, challenges[i].address ) ) {
				// let auth work when server and master don't see the same IP
				common->DPrintf( "auth: matched guid '%s' for != IPs %s and %s\n", client_guid, Sys_NetAdrToString( client_from ), Sys_NetAdrToString( challenges[i].address ) );
			}
			break;
		}
	}
	if ( i >= MAX_CHALLENGES ) {
		common->DPrintf( "auth: failed client lookup %s %s\n", Sys_NetAdrToString( client_from ), client_guid );
		return;
	}

	if ( challenges[ i ].authState != CDK_WAIT ) {
		common->DWarning( "auth: challenge 0x%x %s authState %d != CDK_WAIT", challenges[ i ].challenge, Sys_NetAdrToString( challenges[ i ].address ), challenges[ i ].authState );
		return;
	}

	idStr::snPrintf( challenges[ i ].guid, 12, client_guid );
	if ( reply == AUTH_OK ) {
		challenges[ i ].authState = CDK_OK;
		common->Printf( "client %s %s is authed\n", Sys_NetAdrToString( client_from ), client_guid );
	} else {
		const char *msg;
		if ( replyMsg != AUTH_REPLY_PRINT ) {
			msg = authReplyMsg[ replyMsg ];
		} else {
			msg = replyPrintMsg.c_str();
		}
		// maybe localize it
		const char *l_msg = common->GetLanguageDict()->GetString( msg );
		common->DPrintf( "auth: client %s %s - %s %s\n", Sys_NetAdrToString( client_from ), client_guid, authReplyStr[ reply ], l_msg );
		challenges[ i ].authReply = reply;
		challenges[ i ].authReplyMsg = replyMsg;
		challenges[ i ].authReplyPrint = replyPrintMsg;
	}
}

/*
==================
idAsyncServer::ProcessChallengeMessage
==================
*/
void idAsyncServer::ProcessChallengeMessage( const netadr_t from, const idBitMsg &msg ) {
	int			i, clientId, oldest, oldestTime;
	idBitMsg	outMsg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	clientId = msg.ReadInt();

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for ( i = 0; i < MAX_CHALLENGES; i++ ) {
		if ( !challenges[i].connected && Sys_CompareNetAdrBase( from, challenges[i].address ) && clientId == challenges[i].clientId ) {
			break;
		}
		if ( challenges[i].time < oldestTime ) {
			oldestTime = challenges[i].time;
			oldest = i;
		}
	}

	if ( i >= MAX_CHALLENGES ) {
		// this is the first time this client has asked for a challenge
		i = oldest;
		challenges[i].address = from;
		challenges[i].clientId = clientId;
		challenges[i].challenge = ( (rand() << 16) ^ rand() ) ^ serverTime;
		challenges[i].time = serverTime;
		challenges[i].connected = false;
		challenges[i].authState = CDK_WAIT;
		challenges[i].authReply = AUTH_NONE;
		challenges[i].authReplyMsg = AUTH_REPLY_WAITING;
		challenges[i].authReplyPrint = "";
		challenges[i].guid[0] = '\0';
	}
	challenges[i].pingTime = serverTime;

	common->Printf( "sending challenge 0x%x to %s\n", challenges[i].challenge, Sys_NetAdrToString( from ) );

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	outMsg.WriteString( "challengeResponse" );
	outMsg.WriteInt( challenges[i].challenge );
	outMsg.WriteShort( serverId );
	outMsg.WriteString( cvarSystem->GetCVarString( "fs_game_base" ) );
	outMsg.WriteString( cvarSystem->GetCVarString( "fs_game" ) );

	serverPort.SendPacket( from, outMsg.GetData(), outMsg.GetSize() );

#if ID_ENFORCE_KEY_CLIENT
	if ( Sys_IsLANAddress( from ) ) {
		// no CD Key check for LAN clients
		challenges[i].authState = CDK_OK;
	} else {
		if ( idAsyncNetwork::LANServer.GetBool() ) {
			common->Printf( "net_LANServer is enabled. Client %s is not a LAN address, will be rejected\n", Sys_NetAdrToString( from ) );
			challenges[ i ].authState = CDK_ONLYLAN;
		} else {
			// emit a cd key confirmation request
			outMsg.BeginWriting();
			outMsg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
			outMsg.WriteString( "srvAuth" );
			outMsg.WriteInt( ASYNC_PROTOCOL_VERSION );
			outMsg.WriteNetadr( from );
			outMsg.WriteInt( -1 ); // this identifies "challenge" auth vs "connect" auth
			// protocol 1.37 addition
			outMsg.WriteByte( fileSystem->RunningD3XP() );
			serverPort.SendPacket( idAsyncNetwork::GetMasterAddress(), outMsg.GetData(), outMsg.GetSize() );
		}
	}
#else
	if (! Sys_IsLANAddress( from ) ) {
		common->Printf( "Build Does not have CD Key Enforcement enabled. Client %s is not a LAN address, but will be accepted\n", Sys_NetAdrToString( from ) );
	}
	challenges[i].authState = CDK_OK;
#endif
}

/*
==================
idAsyncServer::SendPureServerMessage
==================
*/
bool idAsyncServer::SendPureServerMessage( const netadr_t to ) {
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_MESSAGE_SIZE ];
	int			serverChecksums[ MAX_PURE_PAKS ];
	int			i;

	fileSystem->GetPureServerChecksums( serverChecksums );
	if ( !serverChecksums[ 0 ] ) {
		// happens if you run fully expanded assets with si_pure 1
		common->Warning( "pure server has no pak files referenced" );
		return false;
	}
	common->DPrintf( "client %s: sending pure pak list\n", Sys_NetAdrToString( to ) );

	// send our list of required paks
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	outMsg.WriteString( "pureServer" );

	i = 0;
	while ( serverChecksums[ i ] ) {
		outMsg.WriteInt( serverChecksums[ i++ ] );
	}
	outMsg.WriteInt( 0 );

	serverPort.SendPacket( to, outMsg.GetData(), outMsg.GetSize() );
	return true;
}

/*
==================
idAsyncServer::SendReliablePureToClient
==================
*/
bool idAsyncServer::SendReliablePureToClient( int clientNum ) {
	idBitMsg	msg;
	byte		msgBuf[ MAX_MESSAGE_SIZE ];
	int			serverChecksums[ MAX_PURE_PAKS ];
	int			i;

	fileSystem->GetPureServerChecksums( serverChecksums );
	if ( !serverChecksums[ 0 ] ) {
		// happens if you run fully expanded assets with si_pure 1
		common->Warning( "pure server has no pak files referenced" );
		return false;
	}

	common->DPrintf( "client %d: sending pure pak list (reliable channel) @ gameInitId %d\n", clientNum, gameInitId );

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteByte( SERVER_RELIABLE_MESSAGE_PURE );

	msg.WriteInt( gameInitId );

	i = 0;
	while ( serverChecksums[ i ] ) {
		msg.WriteInt( serverChecksums[ i++ ] );
	}
	msg.WriteInt( 0 );

	SendReliableMessage( clientNum, msg );

	return true;
}

/*
==================
idAsyncServer::ValidateChallenge
==================
*/
int idAsyncServer::ValidateChallenge( const netadr_t from, int challenge, int clientId ) {
	int i;
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		const serverClient_t &client = clients[i];

		if ( client.clientState == SCS_FREE ) {
			continue;
		}
		if ( Sys_CompareNetAdrBase( from, client.channel.GetRemoteAddress() ) &&
					( clientId == client.clientId || from.port == client.channel.GetRemoteAddress().port ) ) {
			if ( serverTime - client.lastConnectTime < MIN_RECONNECT_TIME ) {
				common->Printf( "%s: reconnect rejected : too soon\n", Sys_NetAdrToString( from ) );
				return -1;
			}
			break;
		}
	}

	for ( i = 0; i < MAX_CHALLENGES; i++ ) {
		if ( Sys_CompareNetAdrBase( from, challenges[i].address ) && from.port == challenges[i].address.port ) {
			if ( challenge == challenges[i].challenge ) {
				break;
			}
		}
	}
	if ( i == MAX_CHALLENGES ) {
		PrintOOB( from, SERVER_PRINT_BADCHALLENGE, "#str_04840" );
		return -1;
	}
	return i;
}

/*
==================
idAsyncServer::ProcessConnectMessage
==================
*/
void idAsyncServer::ProcessConnectMessage( const netadr_t from, const idBitMsg &msg ) {

}

/*
==================
idAsyncServer::VerifyChecksumMessage
==================
*/
bool idAsyncServer::VerifyChecksumMessage( int clientNum, const netadr_t *from, const idBitMsg &msg, idStr &reply ) {
	int		i, numChecksums;
	int		checksums[ MAX_PURE_PAKS ];
	int		serverChecksums[ MAX_PURE_PAKS ];

	// pak checksums, in a 0-terminated list
	numChecksums = 0;
	do {
		i = msg.ReadInt( );
		checksums[ numChecksums++ ] = i;
		// just to make sure a broken client doesn't crash us
		if ( numChecksums >= MAX_PURE_PAKS ) {
			common->Warning( "MAX_PURE_PAKS ( %d ) exceeded in idAsyncServer::ProcessPureMessage\n", MAX_PURE_PAKS );
			sprintf( reply, "#str_07144" );
			return false;
		}
	} while ( i );
	numChecksums--;

	fileSystem->GetPureServerChecksums( serverChecksums );
	assert( serverChecksums[ 0 ] );

	for ( i = 0; serverChecksums[ i ] != 0; i++ ) {
		if ( checksums[ i ] != serverChecksums[ i ] ) {
			common->DPrintf( "client %s: pak missing ( 0x%x )\n", from ? Sys_NetAdrToString( *from ) : va( "%d", clientNum ), serverChecksums[ i ] );
			sprintf( reply, "pak missing ( 0x%x )\n", serverChecksums[ i ] );
			return false;
		}
	}
	if ( checksums[ i ] != 0 ) {
		common->DPrintf( "client %s: extra pak file referenced ( 0x%x )\n", from ? Sys_NetAdrToString( *from ) : va( "%d", clientNum ), checksums[ i ] );
		sprintf( reply, "extra pak file referenced ( 0x%x )\n", checksums[ i ] );
		return false;
	}
	return true;
}

/*
==================
idAsyncServer::ProcessPureMessage
==================
*/
void idAsyncServer::ProcessPureMessage( const netadr_t from, const idBitMsg &msg ) {
	int		iclient, challenge, clientId;
	idStr	reply;

	challenge = msg.ReadInt();
	clientId = msg.ReadShort();

	if ( ( iclient = ValidateChallenge( from, challenge, clientId ) ) == -1 ) {
		return;
	}

	if ( challenges[ iclient ].authState != CDK_PUREWAIT ) {
		common->DPrintf( "client %s: got pure message, not in CDK_PUREWAIT\n", Sys_NetAdrToString( from ) );
		return;
	}

	if ( !VerifyChecksumMessage( iclient, &from, msg, reply ) ) {
		PrintOOB( from, SERVER_PRINT_MISC, reply );
		return;
	}

	common->DPrintf( "client %s: passed pure checks\n", Sys_NetAdrToString( from ) );
	challenges[ iclient ].authState = CDK_PUREOK; // next connect message will get the client through completely
}

/*
==================
idAsyncServer::ProcessReliablePure
==================
*/
void idAsyncServer::ProcessReliablePure( int clientNum, const idBitMsg &msg ) {
	idStr		reply;
	idBitMsg	outMsg;
	byte		msgBuf[MAX_MESSAGE_SIZE];
	int			clientGameInitId;

	clientGameInitId = msg.ReadInt();
	if ( clientGameInitId != gameInitId ) {
		common->DPrintf( "client %d: ignoring reliable pure from an old gameInit (%d)\n", clientNum, clientGameInitId );
		return;
	}

	if ( clients[ clientNum ].clientState != SCS_PUREWAIT ) {
		// should not happen unless something is very wrong. still, don't let this crash us, just get rid of the client
		common->DPrintf( "client %d: got reliable pure while != SCS_PUREWAIT, sending a reload\n", clientNum );
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( SERVER_RELIABLE_MESSAGE_RELOAD );
		SendReliableMessage( clientNum, msg );
		// go back to SCS_CONNECTED to sleep on the client until it goes away for a reconnect
		clients[ clientNum ].clientState = SCS_CONNECTED;
		return;
	}

	if ( !VerifyChecksumMessage( clientNum, NULL, msg, reply ) ) {
		DropClient( clientNum, reply );
		return;
	}
	common->DPrintf( "client %d: passed pure checks (reliable channel)\n", clientNum );
	clients[ clientNum ].clientState = SCS_CONNECTED;
}

/*
==================
idAsyncServer::RemoteConsoleOutput
==================
*/
void idAsyncServer::RemoteConsoleOutput( const char *string ) {
	noRconOutput = false;
	PrintOOB( rconAddress, SERVER_PRINT_RCON, string );
}

/*
==================
RConRedirect
==================
*/
void RConRedirect( const char *string ) {
	idAsyncNetwork::server.RemoteConsoleOutput( string );
}

/*
==================
idAsyncServer::ProcessRemoteConsoleMessage
==================
*/
void idAsyncServer::ProcessRemoteConsoleMessage( const netadr_t from, const idBitMsg &msg ) {
	idBitMsg	outMsg;
	byte		msgBuf[952];
	char		string[MAX_STRING_CHARS];

	if ( idAsyncNetwork::serverRemoteConsolePassword.GetString()[0] == '\0' ) {
		PrintOOB( from, SERVER_PRINT_MISC, "#str_04846" );
		return;
	}

	msg.ReadString( string, sizeof( string ) );

	if ( idStr::Icmp( string, idAsyncNetwork::serverRemoteConsolePassword.GetString() ) != 0 ) {
		PrintOOB( from, SERVER_PRINT_MISC, "#str_04847" );
		return;
	}

	msg.ReadString( string, sizeof( string ) );

	common->Printf( "rcon from %s: %s\n", Sys_NetAdrToString( from ), string );

	rconAddress = from;
	noRconOutput = true;
	common->BeginRedirect( (char *)msgBuf, sizeof( msgBuf ), RConRedirect );

	cmdSystem->BufferCommandText( CMD_EXEC_NOW, string );

	common->EndRedirect();

	if ( noRconOutput ) {
		PrintOOB( rconAddress, SERVER_PRINT_RCON, "#str_04848" );
	}
}

/*
==================
idAsyncServer::ProcessGetInfoMessage
==================
*/
void idAsyncServer::ProcessGetInfoMessage( const netadr_t from, const idBitMsg &msg ) {

}

/*
===============
idAsyncServer::PrintLocalServerInfo
see (client) "getInfo" -> (server) "infoResponse" -> (client)ProcessGetInfoMessage
===============
*/
void idAsyncServer::PrintLocalServerInfo( void ) {

}

/*
==================
idAsyncServer::ConnectionlessMessage
==================
*/
bool idAsyncServer::ConnectionlessMessage( const netadr_t from, const idBitMsg &msg ) {
	char		string[MAX_STRING_CHARS*2];  // M. Quinn - Even Balance - PB Packets need more than 1024

	msg.ReadString( string, sizeof( string ) );

	// info request
	if ( idStr::Icmp( string, "getInfo" ) == 0 ) {
		ProcessGetInfoMessage( from, msg );
		return false;
	}

	// remote console
	if ( idStr::Icmp( string, "rcon" ) == 0 ) {
		ProcessRemoteConsoleMessage( from, msg );
		return true;
	}

	if ( !active ) {
		PrintOOB( from, SERVER_PRINT_MISC, "#str_04849" );
		return false;
	}

	// challenge from a client
	if ( idStr::Icmp( string, "challenge" ) == 0 ) {
		ProcessChallengeMessage( from, msg );
		return false;
	}

	// connect from a client
	if ( idStr::Icmp( string, "connect" ) == 0 ) {
		ProcessConnectMessage( from, msg );
		return false;
	}

	// pure mesasge from a client
	if ( idStr::Icmp( string, "pureClient" ) == 0 ) {
		ProcessPureMessage( from, msg );
		return false;
	}

	// download request
	if ( idStr::Icmp( string, "downloadRequest" ) == 0 ) {
		ProcessDownloadRequestMessage( from, msg );
	}

	// auth server
	if ( idStr::Icmp( string, "auth" ) == 0 ) {
		if ( !Sys_CompareNetAdrBase( from, idAsyncNetwork::GetMasterAddress() ) ) {
			common->Printf( "auth: bad source %s\n", Sys_NetAdrToString( from ) );
			return false;
		}
		if ( idAsyncNetwork::LANServer.GetBool() ) {
			common->Printf( "auth message from master. net_LANServer is enabled, ignored.\n" );
		}
		ProcessAuthMessage( msg );
		return false;
	}

	return false;
}

/*
==================
idAsyncServer::ProcessMessage
==================
*/
bool idAsyncServer::ProcessMessage( const netadr_t from, idBitMsg &msg ) {
	int			i, id, sequence;
	idBitMsg	outMsg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	id = msg.ReadShort();

	// check for a connectionless message
	if ( id == CONNECTIONLESS_MESSAGE_ID ) {
		return ConnectionlessMessage( from, msg );
	}

	if ( msg.GetRemaingData() < 4 ) {
		common->DPrintf( "%s: tiny packet\n", Sys_NetAdrToString( from ) );
		return false;
	}

	// find out which client the message is from
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		serverClient_t &client = clients[i];

		if ( client.clientState == SCS_FREE ) {
			continue;
		}

		// This does not compare the UDP port, because some address translating
		// routers will change that at arbitrary times.
		if ( !Sys_CompareNetAdrBase( from, client.channel.GetRemoteAddress() ) || id != client.clientId ) {
			continue;
		}

		// make sure it is a valid, in sequence packet
		if ( !client.channel.Process( from, serverTime, msg, sequence ) ) {
			return false;		// out of order, duplicated, fragment, etc.
		}

		// zombie clients still need to do the channel processing to make sure they don't
		// need to retransmit the final reliable message, but they don't do any other processing
		if ( client.clientState == SCS_ZOMBIE ) {
			return false;
		}

		client.lastPacketTime = serverTime;

		ProcessReliableClientMessages( i );
		ProcessUnreliableClientMessage( i, msg );

		return false;
	}

	// if we received a sequenced packet from an address we don't recognize,
	// send an out of band disconnect packet to it
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	outMsg.WriteString( "disconnect" );
	serverPort.SendPacket( from, outMsg.GetData(), outMsg.GetSize() );

	return false;
}

/*
==================
idAsyncServer::SendReliableGameMessage
==================
*/
void idAsyncServer::SendReliableGameMessage( int clientNum, const idBitMsg &msg ) {
	int			i;
	idBitMsg	outMsg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( SERVER_RELIABLE_MESSAGE_GAME );
	outMsg.WriteData( msg.GetData(), msg.GetSize() );

	if ( clientNum >= 0 && clientNum < MAX_ASYNC_CLIENTS ) {
		if ( clients[clientNum].clientState == SCS_INGAME ) {
			SendReliableMessage( clientNum, outMsg );
		}
		return;
	}

	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		if ( clients[i].clientState != SCS_INGAME ) {
			continue;
		}
		SendReliableMessage( i, outMsg );
	}
}

/*
==================
idAsyncServer::LocalClientSendReliableMessageExcluding
==================
*/
void idAsyncServer::SendReliableGameMessageExcluding( int clientNum, const idBitMsg &msg ) {
	int			i;
	idBitMsg	outMsg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	assert( clientNum >= 0 && clientNum < MAX_ASYNC_CLIENTS );

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( SERVER_RELIABLE_MESSAGE_GAME );
	outMsg.WriteData( msg.GetData(), msg.GetSize() );

	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		if ( i == clientNum ) {
			continue;
		}
		if ( clients[i].clientState != SCS_INGAME ) {
			continue;
		}
		SendReliableMessage( i, outMsg );
	}
}

/*
==================
idAsyncServer::LocalClientSendReliableMessage
==================
*/
void idAsyncServer::LocalClientSendReliableMessage( const idBitMsg &msg ) {
}

/*
==================
idAsyncServer::ProcessConnectionLessMessages
==================
*/
void idAsyncServer::ProcessConnectionLessMessages( void ) {
	int			size, id;
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];
	netadr_t	from;

	if ( !serverPort.GetPort() ) {
		return;
	}

	while( serverPort.GetPacket( from, msgBuf, size, sizeof( msgBuf ) ) ) {
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.SetSize( size );
		msg.BeginReading();
		id = msg.ReadShort();
		if ( id == CONNECTIONLESS_MESSAGE_ID ) {
			ConnectionlessMessage( from, msg );
		}
	}
}

/*
==================
idAsyncServer::UpdateTime
==================
*/
int idAsyncServer::UpdateTime( int clamp ) {
	int time, msec;

	time = Sys_Milliseconds();
	msec = idMath::ClampInt( 0, clamp, time - realTime );
	realTime = time;
	serverTime += msec;
	return msec;
}

/*
==================
idAsyncServer::RunFrame
==================
*/
void idAsyncServer::RunFrame( void ) {
	int			i, msec, size;
	bool		newPacket;
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];
	netadr_t	from;
	int			outgoingRate, incomingRate;
	float		outgoingCompression, incomingCompression;

	msec = UpdateTime( 100 );

	if ( !serverPort.GetPort() ) {
		return;
	}

	if ( !active ) {
		ProcessConnectionLessMessages();
		return;
	}

	gameTimeResidual += msec;

	// spin in place processing incoming packets until enough time lapsed to run a new game frame
	do {

		do {

			// blocking read with game time residual timeout
			newPacket = serverPort.GetPacketBlocking( from, msgBuf, size, sizeof( msgBuf ), USERCMD_MSEC - gameTimeResidual - 1 );
			if ( newPacket ) {
				msg.Init( msgBuf, sizeof( msgBuf ) );
				msg.SetSize( size );
				msg.BeginReading();
				if ( ProcessMessage( from, msg ) ) {
					return;	// return because rcon was used
				}
			}

			msec = UpdateTime( 100 );
			gameTimeResidual += msec;

		} while( newPacket );

	} while( gameTimeResidual < USERCMD_MSEC );

	// send heart beat to master servers
	MasterHeartbeat();

	// check for clients that timed out
	CheckClientTimeouts();


	// make sure the time doesn't wrap
	if ( serverTime > 0x70000000 ) {
		ExecuteMapChange();
		return;
	}

	// check for synchronized cvar changes
	if ( cvarSystem->GetModifiedFlags() & CVAR_NETWORKSYNC ) {
		idDict newCvars;
		newCvars = *cvarSystem->MoveCVarsToDict( CVAR_NETWORKSYNC );
		SendSyncedCvarsBroadcast( newCvars );
		cvarSystem->ClearModifiedFlags( CVAR_NETWORKSYNC );
	}



	// duplicate usercmds so there is always at least one available to send with snapshots
	DuplicateUsercmds( gameFrame, gameTime );

	// send snapshots to connected clients
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		serverClient_t &client = clients[i];

		if ( client.clientState == SCS_FREE || i == localClientNum ) {
			continue;
		}

		// modify maximum rate if necesary
		if ( idAsyncNetwork::serverMaxClientRate.IsModified() ) {
			client.channel.SetMaxOutgoingRate( Min( client.clientRate, idAsyncNetwork::serverMaxClientRate.GetInteger() ) );
		}

		// if the channel is not yet ready to send new data
		if ( !client.channel.ReadyToSend( serverTime ) ) {
			continue;
		}

		// send additional message fragments if the last message was too large to send at once
		if ( client.channel.UnsentFragmentsLeft() ) {
			client.channel.SendNextFragment( serverPort, serverTime );
			continue;
		}

		if ( client.clientState == SCS_INGAME ) {
			if ( !SendSnapshotToClient( i ) ) {
				SendPingToClient( i );
			}
		} else {
			SendEmptyToClient( i );
		}
	}

	if ( com_showAsyncStats.GetBool() ) {

		UpdateAsyncStatsAvg();

		// dedicated will verbose to console
		if ( idAsyncNetwork::serverDedicated.GetBool() && serverTime >= nextAsyncStatsTime ) {
			common->Printf( "delay = %d msec, total outgoing rate = %d KB/s, total incoming rate = %d KB/s\n", GetDelay(),
							GetOutgoingRate() >> 10, GetIncomingRate() >> 10 );

			for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {

				outgoingRate = GetClientOutgoingRate( i );
				incomingRate = GetClientIncomingRate( i );
				outgoingCompression = GetClientOutgoingCompression( i );
				incomingCompression = GetClientIncomingCompression( i );

				if ( outgoingRate != -1 && incomingRate != -1 ) {
					common->Printf( "client %d: out rate = %d B/s (% -2.1f%%), in rate = %d B/s (% -2.1f%%)\n",
									i, outgoingRate, outgoingCompression, incomingRate, incomingCompression );
				}
			}

			idStr msg;
			GetAsyncStatsAvgMsg( msg );
			common->Printf( va( "%s\n", msg.c_str() ) );

			nextAsyncStatsTime = serverTime + 1000;
		}
	}

	idAsyncNetwork::serverMaxClientRate.ClearModified();
}

/*
==================
idAsyncServer::PacifierUpdate
==================
*/
void idAsyncServer::PacifierUpdate( void ) {
	int i;

	if ( !IsActive() ) {
		return;
	}
	realTime = Sys_Milliseconds();
	ProcessConnectionLessMessages();
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		if ( clients[i].clientState >= SCS_PUREWAIT ) {
			if ( clients[i].channel.UnsentFragmentsLeft() ) {
				clients[i].channel.SendNextFragment( serverPort, serverTime );
			} else {
				SendEmptyToClient( i );
			}
		}
	}
}

/*
==================
idAsyncServer::PrintOOB
==================
*/
void idAsyncServer::PrintOOB( const netadr_t to, int opcode, const char *string ) {
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_MESSAGE_SIZE ];

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	outMsg.WriteString( "print" );
	outMsg.WriteInt( opcode );
	outMsg.WriteString( string );
	serverPort.SendPacket( to, outMsg.GetData(), outMsg.GetSize() );
}

/*
==================
idAsyncServer::MasterHeartbeat
==================
*/
void idAsyncServer::MasterHeartbeat( bool force ) {
	if ( idAsyncNetwork::LANServer.GetBool() ) {
		if ( force ) {
			common->Printf( "net_LANServer is enabled. Not sending heartbeats\n" );
		}
		return;
	}
	if ( force ) {
		nextHeartbeatTime = 0;
	}
	// not yet
	if ( serverTime < nextHeartbeatTime ) {
		return;
	}
	nextHeartbeatTime = serverTime + HEARTBEAT_MSEC;
	for ( int i = 0 ; i < MAX_MASTER_SERVERS ; i++ ) {
		netadr_t adr;
		if ( idAsyncNetwork::GetMasterAddress( i, adr ) ) {
			common->Printf( "Sending heartbeat to %s\n", Sys_NetAdrToString( adr ) );
			idBitMsg outMsg;
			byte msgBuf[ MAX_MESSAGE_SIZE ];
			outMsg.Init( msgBuf, sizeof( msgBuf ) );
			outMsg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
			outMsg.WriteString( "heartbeat" );
			serverPort.SendPacket( adr, outMsg.GetData(), outMsg.GetSize() );
		}
	}
}

/*
===============
idAsyncServer::SendEnterGameToClient
===============
*/
void idAsyncServer::SendEnterGameToClient( int clientNum ) {
	idBitMsg	msg;
	byte		msgBuf[ MAX_MESSAGE_SIZE ];

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteByte( SERVER_RELIABLE_MESSAGE_ENTERGAME );
	SendReliableMessage( clientNum, msg );
}

/*
===============
idAsyncServer::UpdateAsyncStatsAvg
===============
*/
void idAsyncServer::UpdateAsyncStatsAvg( void ) {
	stats_average_sum -= stats_outrate[ stats_current ];
	stats_outrate[ stats_current ] = idAsyncNetwork::server.GetOutgoingRate();
	if ( stats_outrate[ stats_current ] > stats_max ) {
		stats_max = stats_outrate[ stats_current ];
		stats_max_index = stats_current;
	} else if ( stats_current == stats_max_index ) {
		// find the new max
		int i;
		stats_max = 0;
		for ( i = 0; i < stats_numsamples ; i++ ) {
			if ( stats_outrate[ i ] > stats_max ) {
				stats_max = stats_outrate[ i ];
				stats_max_index = i;
			}
		}
	}
	stats_average_sum += stats_outrate[ stats_current ];
	stats_current++; stats_current %= stats_numsamples;
}

/*
===============
idAsyncServer::GetAsyncStatsAvgMsg
===============
*/
void idAsyncServer::GetAsyncStatsAvgMsg( idStr &msg ) {
	sprintf( msg, "avrg out: %d B/s - max %d B/s ( over %d ms )", stats_average_sum / stats_numsamples, stats_max, idAsyncNetwork::serverSnapshotDelay.GetInteger() * stats_numsamples );
}

/*
===============
idAsyncServer::ProcessDownloadRequestMessage
===============
*/
void idAsyncServer::ProcessDownloadRequestMessage( const netadr_t from, const idBitMsg &msg ) {

}
