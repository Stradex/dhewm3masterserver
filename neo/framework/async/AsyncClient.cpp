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
#include "framework/Licensee.h"

#include "framework/async/AsyncClient.h"

const int SETUP_CONNECTION_RESEND_TIME	= 1000;
const int EMPTY_RESEND_TIME				= 500;
const int PREDICTION_FAST_ADJUST		= 4;

/*
==================
idAsyncClient::idAsyncClient
==================
*/
idAsyncClient::idAsyncClient( void ) {
	updateState = UPDATE_NONE;
	Clear();
}

/*
==================
idAsyncClient::Clear
==================
*/
void idAsyncClient::Clear( void ) {
	active = false;
	realTime = 0;
	clientTime = 0;
	clientId = 0;
	clientDataChecksum = 0;
	clientNum = 0;
	clientState = CS_DISCONNECTED;
	clientPrediction = 0;
	clientPredictTime = 0;
	serverId = 0;
	serverChallenge = 0;
	serverMessageSequence = 0;
	lastConnectTime = -9999;
	lastEmptyTime = -9999;
	lastPacketTime = -9999;
	lastSnapshotTime = -9999;
	snapshotGameFrame = 0;
	snapshotGameTime = 0;
	snapshotSequence = 0;
	gameInitId = GAME_INIT_ID_INVALID;
	gameFrame = 0;
	gameTimeResidual = 0;
	gameTime = 0;
	memset( userCmds, 0, sizeof( userCmds ) );
	backgroundDownload.completed = true;
	lastRconTime = 0;
	showUpdateMessage = false;
	lastFrameDelta = 0;

	dlRequest = -1;
	dlCount = -1;
	memset( dlChecksums, 0, sizeof( int ) * MAX_PURE_PAKS );
	currentDlSize = 0;
	totalDlSize = 0;
}

/*
==================
idAsyncClient::Shutdown
==================
*/
void idAsyncClient::Shutdown( void ) {
	updateMSG.Clear();
	updateURL.Clear();
	updateFile.Clear();
	updateFallback.Clear();
	backgroundDownload.url.url.Clear();
	dlList.Clear();
}

/*
==================
idAsyncClient::InitPort
==================
*/
bool idAsyncClient::InitPort( void ) {
	// if this is the first time we connect to a server, open the UDP port
	if ( !clientPort.GetPort() ) {
		if ( !clientPort.InitForPort( PORT_ANY ) ) {
			common->Printf( "Couldn't open client network port.\n" );
			return false;
		}
	}

	return true;
}

/*
==================
idAsyncClient::ClosePort
==================
*/
void idAsyncClient::ClosePort( void ) {
	clientPort.Close();
}

/*
==================
idAsyncClient::ClearPendingPackets
==================
*/
void idAsyncClient::ClearPendingPackets( void ) {
	int			size;
	byte		msgBuf[MAX_MESSAGE_SIZE];
	netadr_t	from;

	while( clientPort.GetPacket( from, msgBuf, size, sizeof( msgBuf ) ) ) {
	}
}

/*
==================
idAsyncClient::HandleGuiCommandInternal
==================
*/
const char* idAsyncClient::HandleGuiCommandInternal( const char *cmd ) {
	if ( !idStr::Cmp( cmd, "abort" ) || !idStr::Cmp( cmd, "pure_abort" ) ) {
		common->DPrintf( "connection aborted\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
		return "";
	} else {
		common->DWarning( "idAsyncClient::HandleGuiCommand: unknown cmd %s", cmd );
	}
	return NULL;
}

/*
==================
idAsyncClient::HandleGuiCommand
==================
*/
const char* idAsyncClient::HandleGuiCommand( const char *cmd ) {
	return idAsyncNetwork::client.HandleGuiCommandInternal( cmd );
}

/*
==================
idAsyncClient::ConnectToServer
==================
*/
void idAsyncClient::ConnectToServer( const netadr_t adr ) {

	if ( !InitPort() ) {
		return;
	}

	if ( cvarSystem->GetCVarBool( "net_serverDedicated" ) ) {
		common->Printf( "Can't connect to a server as dedicated\n" );
		return;
	}

	// trash any currently pending packets
	ClearPendingPackets();

	serverAddress = adr;

	// clear the client state
	Clear();

	// get a pseudo random client id, but don't use the id which is reserved for connectionless packets
	clientId = Sys_Milliseconds() & CONNECTIONLESS_MESSAGE_ID_MASK;


	// start challenging the server
	clientState = CS_CHALLENGING;

	active = true;
}

/*
==================
idAsyncClient::Reconnect
==================
*/
void idAsyncClient::Reconnect( void ) {
	ConnectToServer( serverAddress );
}

/*
==================
idAsyncClient::ConnectToServer
==================
*/
void idAsyncClient::ConnectToServer( const char *address ) {
	int serverNum;
	netadr_t adr;

	if ( idStr::IsNumeric( address ) ) {
		serverNum = atoi( address );
		if ( serverNum < 0 || serverNum >= serverList.Num() ) {
			return;
		}
		adr = serverList[ serverNum ].adr;
	} else {
		if ( !Sys_StringToNetAdr( address, &adr, true ) ) {
			return;
		}
	}
	if ( !adr.port ) {
		adr.port = PORT_SERVER;
	}

	common->Printf( "\"%s\" resolved to %s\n", address, Sys_NetAdrToString( adr ) );

	ConnectToServer( adr );
}

/*
==================
idAsyncClient::DisconnectFromServer
==================
*/
void idAsyncClient::DisconnectFromServer( void ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	if ( clientState >= CS_CONNECTED ) {
		// if we were actually connected, clear the pure list
		fileSystem->ClearPureChecksums();

		// send reliable disconnect to server
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.WriteByte( CLIENT_RELIABLE_MESSAGE_DISCONNECT );
		msg.WriteString( "disconnect" );

		if ( !channel.SendReliableMessage( msg ) ) {
			common->Error( "client->server reliable messages overflow\n" );
		}

		SendEmptyToServer( true );
		SendEmptyToServer( true );
		SendEmptyToServer( true );
	}

	if ( clientState != CS_PURERESTART ) {
		channel.Shutdown();
		clientState = CS_DISCONNECTED;
	}

	active = false;
}

/*
==================
idAsyncClient::GetServerInfo
==================
*/
void idAsyncClient::GetServerInfo( const netadr_t adr ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	if ( !InitPort() ) {
		return;
	}

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	msg.WriteString( "getInfo" );
	msg.WriteInt( serverList.GetChallenge() );	// challenge

	clientPort.SendPacket( adr, msg.GetData(), msg.GetSize() );
}

/*
==================
idAsyncClient::GetServerInfo
==================
*/
void idAsyncClient::GetServerInfo( const char *address ) {
	netadr_t	adr;

	if ( address && *address != '\0' ) {
		if ( !Sys_StringToNetAdr( address, &adr, true ) ) {
			common->Printf( "Couldn't get server address for \"%s\"\n", address );
			return;
		}
	} else if ( active ) {
		adr = serverAddress;
	} else if ( idAsyncNetwork::server.IsActive() ) {
		// used to be a Sys_StringToNetAdr( "localhost", &adr, true ); and send a packet over loopback
		// but this breaks with net_ip ( typically, for multi-homed servers )
		idAsyncNetwork::server.PrintLocalServerInfo();
		return;
	} else {
		common->Printf( "no server found\n" );
		return;
	}

	if ( !adr.port ) {
		adr.port = PORT_SERVER;
	}

	GetServerInfo( adr );
}

/*
==================
idAsyncClient::GetLANServers
==================
*/
void idAsyncClient::GetLANServers( void ) {
	int			i;
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];
	netadr_t	broadcastAddress;

	if ( !InitPort() ) {
		return;
	}

	idAsyncNetwork::LANServer.SetBool( true );

	serverList.SetupLANScan();

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	msg.WriteString( "getInfo" );
	msg.WriteInt( serverList.GetChallenge() );

	broadcastAddress.type = NA_BROADCAST;
	for ( i = 0; i < MAX_SERVER_PORTS; i++ ) {
		broadcastAddress.port = PORT_SERVER + i;
		clientPort.SendPacket( broadcastAddress, msg.GetData(), msg.GetSize() );
	}
}

/*
==================
idAsyncClient::GetNETServers
==================
*/
void idAsyncClient::GetNETServers( void ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	idAsyncNetwork::LANServer.SetBool( false );

	// NetScan only clears GUI and results, not the stored list
	serverList.Clear( );
	serverList.NetScan( );
	serverList.StartServers( true );

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	msg.WriteString( "getServers" );
	msg.WriteInt( ASYNC_PROTOCOL_VERSION );
	msg.WriteString( cvarSystem->GetCVarString( "fs_game" ) );
	msg.WriteBits( cvarSystem->GetCVarInteger( "gui_filter_password" ), 2 );
	msg.WriteBits( cvarSystem->GetCVarInteger( "gui_filter_players" ), 2 );
	msg.WriteBits( cvarSystem->GetCVarInteger( "gui_filter_gameType" ), 2 );

	netadr_t adr;
	if ( idAsyncNetwork::GetMasterAddress( 0, adr ) ) {
		clientPort.SendPacket( adr, msg.GetData(), msg.GetSize() );
	}
}

/*
==================
idAsyncClient::ListServers
==================
*/
void idAsyncClient::ListServers( void ) {
	int i;

	for ( i = 0; i < serverList.Num(); i++ ) {
		common->Printf( "%3d: %s %dms (%s)\n", i, serverList[i].serverInfo.GetString( "si_name" ), serverList[ i ].ping, Sys_NetAdrToString( serverList[i].adr ) );
	}
}

/*
==================
idAsyncClient::ClearServers
==================
*/
void idAsyncClient::ClearServers( void ) {
	serverList.Clear();
}

/*
==================
idAsyncClient::RemoteConsole
==================
*/
void idAsyncClient::RemoteConsole( const char *command ) {
	netadr_t	adr;
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	if ( !InitPort() ) {
		return;
	}

	if ( active ) {
		adr = serverAddress;
	} else {
		Sys_StringToNetAdr( idAsyncNetwork::clientRemoteConsoleAddress.GetString(), &adr, true );
	}

	if ( !adr.port ) {
		adr.port = PORT_SERVER;
	}

	lastRconAddress = adr;
	lastRconTime = realTime;

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	msg.WriteString( "rcon" );
	msg.WriteString( idAsyncNetwork::clientRemoteConsolePassword.GetString() );
	msg.WriteString( command );

	clientPort.SendPacket( adr, msg.GetData(), msg.GetSize() );
}

/*
==================
idAsyncClient::GetPrediction
==================
*/
int idAsyncClient::GetPrediction( void ) const {
	if ( clientState < CS_CONNECTED ) {
		return -1;
	} else {
		return clientPrediction;
	}
}

/*
==================
idAsyncClient::GetTimeSinceLastPacket
==================
*/
int idAsyncClient::GetTimeSinceLastPacket( void ) const {
	if ( clientState < CS_CONNECTED ) {
		return -1;
	} else {
		return clientTime - lastPacketTime;
	}
}

/*
==================
idAsyncClient::GetOutgoingRate
==================
*/
int idAsyncClient::GetOutgoingRate( void ) const {
	if ( clientState < CS_CONNECTED ) {
		return -1;
	} else {
		return channel.GetOutgoingRate();
	}
}

/*
==================
idAsyncClient::GetIncomingRate
==================
*/
int idAsyncClient::GetIncomingRate( void ) const {
	if ( clientState < CS_CONNECTED ) {
		return -1;
	} else {
		return channel.GetIncomingRate();
	}
}

/*
==================
idAsyncClient::GetOutgoingCompression
==================
*/
float idAsyncClient::GetOutgoingCompression( void ) const {
	if ( clientState < CS_CONNECTED ) {
		return 0.0f;
	} else {
		return channel.GetOutgoingCompression();
	}
}

/*
==================
idAsyncClient::GetIncomingCompression
==================
*/
float idAsyncClient::GetIncomingCompression( void ) const {
	if ( clientState < CS_CONNECTED ) {
		return 0.0f;
	} else {
		return channel.GetIncomingCompression();
	}
}

/*
==================
idAsyncClient::GetIncomingPacketLoss
==================
*/
float idAsyncClient::GetIncomingPacketLoss( void ) const {
	if ( clientState < CS_CONNECTED ) {
		return 0.0f;
	} else {
		return channel.GetIncomingPacketLoss();
	}
}

/*
==================
idAsyncClient::DuplicateUsercmds
==================
*/
void idAsyncClient::DuplicateUsercmds( int frame, int time ) {
	int i, previousIndex, currentIndex;

	previousIndex = ( frame - 1 ) & ( MAX_USERCMD_BACKUP - 1 );
	currentIndex = frame & ( MAX_USERCMD_BACKUP - 1 );

	// duplicate previous user commands if no new commands are available for a client
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		idAsyncNetwork::DuplicateUsercmd( userCmds[previousIndex][i], userCmds[currentIndex][i], frame, time );
	}
}

/*
==================
idAsyncClient::SendUserInfoToServer
==================
*/
void idAsyncClient::SendUserInfoToServer( void ) {
}

/*
==================
idAsyncClient::SendEmptyToServer
==================
*/
void idAsyncClient::SendEmptyToServer( bool force, bool mapLoad ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	if ( lastEmptyTime > realTime ) {
		lastEmptyTime = realTime;
	}

	if ( !force && ( realTime - lastEmptyTime < EMPTY_RESEND_TIME ) ) {
		return;
	}

	if ( idAsyncNetwork::verbose.GetInteger() ) {
		common->Printf( "sending empty to server, gameInitId = %d\n", mapLoad ? GAME_INIT_ID_MAP_LOAD : gameInitId );
	}

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteInt( serverMessageSequence );
	msg.WriteInt( mapLoad ? GAME_INIT_ID_MAP_LOAD : gameInitId );
	msg.WriteInt( snapshotSequence );
	msg.WriteByte( CLIENT_UNRELIABLE_MESSAGE_EMPTY );

	channel.SendMessage( clientPort, clientTime, msg );

	while( channel.UnsentFragmentsLeft() ) {
		channel.SendNextFragment( clientPort, clientTime );
	}

	lastEmptyTime = realTime;
}

/*
==================
idAsyncClient::SendPingResponseToServer
==================
*/
void idAsyncClient::SendPingResponseToServer( int time ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	if ( idAsyncNetwork::verbose.GetInteger() == 2 ) {
		common->Printf( "sending ping response to server, gameInitId = %d\n", gameInitId );
	}

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteInt( serverMessageSequence );
	msg.WriteInt( gameInitId );
	msg.WriteInt( snapshotSequence );
	msg.WriteByte( CLIENT_UNRELIABLE_MESSAGE_PINGRESPONSE );
	msg.WriteInt( time );

	channel.SendMessage( clientPort, clientTime, msg );
	while( channel.UnsentFragmentsLeft() ) {
		channel.SendNextFragment( clientPort, clientTime );
	}
}

/*
==================
idAsyncClient::SendUsercmdsToServer
==================
*/
void idAsyncClient::SendUsercmdsToServer( void ) {
	int			i, numUsercmds, index;
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];
	usercmd_t *	last;

	if ( idAsyncNetwork::verbose.GetInteger() == 2 ) {
		common->Printf( "sending usercmd to server: gameInitId = %d, gameFrame = %d, gameTime = %d\n", gameInitId, gameFrame, gameTime );
	}

	// generate user command for this client
	index = gameFrame & ( MAX_USERCMD_BACKUP - 1 );
	userCmds[index][clientNum] = usercmdGen->GetDirectUsercmd();
	userCmds[index][clientNum].gameFrame = gameFrame;
	userCmds[index][clientNum].gameTime = gameTime;

	// send the user commands to the server
	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteInt( serverMessageSequence );
	msg.WriteInt( gameInitId );
	msg.WriteInt( snapshotSequence );
	msg.WriteByte( CLIENT_UNRELIABLE_MESSAGE_USERCMD );
	msg.WriteShort( clientPrediction );

	numUsercmds = idMath::ClampInt( 0, 10, idAsyncNetwork::clientUsercmdBackup.GetInteger() ) + 1;

	// write the user commands
	msg.WriteInt( gameFrame );
	msg.WriteByte( numUsercmds );
	for ( last = NULL, i = gameFrame - numUsercmds + 1; i <= gameFrame; i++ ) {
		index = i & ( MAX_USERCMD_BACKUP - 1 );
		idAsyncNetwork::WriteUserCmdDelta( msg, userCmds[index][clientNum], last );
		last = &userCmds[index][clientNum];
	}

	channel.SendMessage( clientPort, clientTime, msg );
	while( channel.UnsentFragmentsLeft() ) {
		channel.SendNextFragment( clientPort, clientTime );
	}
}

/*
==================
idAsyncClient::InitGame
==================
*/
void idAsyncClient::InitGame( int serverGameInitId, int serverGameFrame, int serverGameTime, const idDict &serverSI ) {

}

/*
==================
idAsyncClient::ProcessUnreliableServerMessage
==================
*/
void idAsyncClient::ProcessUnreliableServerMessage( const idBitMsg &msg ) {

}

/*
==================
idAsyncClient::ProcessReliableMessagePure
==================
*/
void idAsyncClient::ProcessReliableMessagePure( const idBitMsg &msg ) {

}

/*
===============
idAsyncClient::ReadLocalizedServerString
===============
*/
void idAsyncClient::ReadLocalizedServerString( const idBitMsg &msg, char *out, int maxLen ) {
	msg.ReadString( out, maxLen );
	// look up localized string. if the message is not an #str_ format, we'll just get it back unchanged
	idStr::snPrintf( out, maxLen - 1, "%s", common->GetLanguageDict()->GetString( out ) );
}

/*
==================
idAsyncClient::ProcessReliableServerMessages
==================
*/
void idAsyncClient::ProcessReliableServerMessages( void ) {

}

/*
==================
idAsyncClient::ProcessChallengeResponseMessage
==================
*/
void idAsyncClient::ProcessChallengeResponseMessage( const netadr_t from, const idBitMsg &msg ) {
	char serverGame[ MAX_STRING_CHARS ], serverGameBase[ MAX_STRING_CHARS ];

	if ( clientState != CS_CHALLENGING ) {
		common->Printf( "Unwanted challenge response received.\n" );
		return;
	}

	serverChallenge = msg.ReadInt();
	serverId = msg.ReadShort();
	msg.ReadString( serverGameBase, MAX_STRING_CHARS );
	msg.ReadString( serverGame, MAX_STRING_CHARS );

	// the server is running a different game... we need to reload in the correct fs_game
	// even pure pak checks would fail if we didn't, as there are files we may not even see atm
	// NOTE: we could read the pure list from the server at the same time and set it up for the restart
	// ( if the client can restart directly with the right pak order, then we avoid an extra reloadEngine later.. )
	if ( idStr::Icmp( cvarSystem->GetCVarString( "fs_game_base" ), serverGameBase ) ||
		idStr::Icmp( cvarSystem->GetCVarString( "fs_game" ), serverGame ) ) {
		// bug #189 - if the server is running ROE and ROE is not locally installed, refuse to connect or we might crash
		if ( !fileSystem->HasD3XP() && ( !idStr::Icmp( serverGameBase, "d3xp" ) || !idStr::Icmp( serverGame, "d3xp" ) ) ) {
			common->Printf( "The server is running Doom3: Resurrection of Evil expansion pack. RoE is not installed on this client. Aborting the connection..\n" );
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "disconnect\n" );
			return;
		}
		common->Printf( "The server is running a different mod (%s-%s). Restarting..\n", serverGameBase, serverGame );
		cvarSystem->SetCVarString( "fs_game_base", serverGameBase );
		cvarSystem->SetCVarString( "fs_game", serverGame );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "reloadEngine" );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "reconnect\n" );
		return;
	}

	common->Printf( "received challenge response 0x%x from %s\n", serverChallenge, Sys_NetAdrToString( from ) );

	// start sending connect packets instead of challenge request packets
	clientState = CS_CONNECTING;
	lastConnectTime = -9999;

	// take this address as the new server address.  This allows
	// a server proxy to hand off connections to multiple servers
	serverAddress = from;
}

/*
==================
idAsyncClient::ProcessConnectResponseMessage
==================
*/
void idAsyncClient::ProcessConnectResponseMessage( const netadr_t from, const idBitMsg &msg ) {

}

/*
==================
idAsyncClient::ProcessDisconnectMessage
==================
*/
void idAsyncClient::ProcessDisconnectMessage( const netadr_t from, const idBitMsg &msg ) {
	if ( clientState == CS_DISCONNECTED ) {
		common->Printf( "Disconnect packet while not connected.\n" );
		return;
	}
	if ( !Sys_CompareNetAdrBase( from, serverAddress ) ) {
		common->Printf( "Disconnect packet from unknown server.\n" );
		return;
	}
}

/*
==================
idAsyncClient::ProcessInfoResponseMessage
==================
*/
void idAsyncClient::ProcessInfoResponseMessage( const netadr_t from, const idBitMsg &msg ) {
	int i, protocol, index;
	networkServer_t serverInfo;
	bool verbose = false;

	if ( from.type == NA_LOOPBACK || cvarSystem->GetCVarBool( "developer" ) ) {
		verbose = true;
	}

	serverInfo.clients = 0;
	serverInfo.adr = from;
	serverInfo.challenge = msg.ReadInt();			// challenge
	protocol = msg.ReadInt();
	if ( protocol != ASYNC_PROTOCOL_VERSION ) {
		common->Printf( "server %s ignored - protocol %d.%d, expected %d.%d\n", Sys_NetAdrToString( serverInfo.adr ), protocol >> 16, protocol & 0xffff, ASYNC_PROTOCOL_MAJOR, ASYNC_PROTOCOL_MINOR );
		return;
	}
	msg.ReadDeltaDict( serverInfo.serverInfo, NULL );

	if ( verbose ) {
		common->Printf( "server IP = %s\n", Sys_NetAdrToString( serverInfo.adr ) );
		serverInfo.serverInfo.Print();
	}
	for ( i = msg.ReadByte(); i < MAX_ASYNC_CLIENTS; i = msg.ReadByte() ) {
		serverInfo.pings[ serverInfo.clients ] = msg.ReadShort();
		serverInfo.rate[ serverInfo.clients ] = msg.ReadInt();
		msg.ReadString( serverInfo.nickname[ serverInfo.clients ], MAX_NICKLEN );
		if ( verbose ) {
			common->Printf( "client %2d: %s, ping = %d, rate = %d\n", i, serverInfo.nickname[ serverInfo.clients ], serverInfo.pings[ serverInfo.clients ], serverInfo.rate[ serverInfo.clients ] );
		}
		serverInfo.clients++;
	}
	index = serverList.InfoResponse( serverInfo );

	common->Printf( "%d: server %s - protocol %d.%d - %s\n", index, Sys_NetAdrToString( serverInfo.adr ), protocol >> 16, protocol & 0xffff, serverInfo.serverInfo.GetString( "si_name" ) );
}

/*
==================
idAsyncClient::ProcessPrintMessage
==================
*/
void idAsyncClient::ProcessPrintMessage( const netadr_t from, const idBitMsg &msg ) {

}

/*
==================
idAsyncClient::ProcessServersListMessage
==================
*/
void idAsyncClient::ProcessServersListMessage( const netadr_t from, const idBitMsg &msg ) {
	if ( !Sys_CompareNetAdrBase( idAsyncNetwork::GetMasterAddress(), from ) ) {
		common->DPrintf( "received a server list from %s - not a valid master\n", Sys_NetAdrToString( from ) );
		return;
	}
	while ( msg.GetRemaingData() ) {
		int a,b,c,d;
		a = msg.ReadByte(); b = msg.ReadByte(); c = msg.ReadByte(); d = msg.ReadByte();
		serverList.AddServer( serverList.Num(), va( "%i.%i.%i.%i:%i", a, b, c, d, msg.ReadShort() ) );
	}
}

/*
==================
idAsyncClient::ProcessAuthKeyMessage
==================
*/
void idAsyncClient::ProcessAuthKeyMessage( const netadr_t from, const idBitMsg &msg ) {

}

/*
==================
idAsyncClient::ProcessVersionMessage
==================
*/
void idAsyncClient::ProcessVersionMessage( const netadr_t from, const idBitMsg &msg ) {
	char string[ MAX_STRING_CHARS ];

	if ( updateState != UPDATE_SENT ) {
		common->Printf( "ProcessVersionMessage: version reply, != UPDATE_SENT\n" );
		return;
	}

	common->Printf( "A new version is available\n" );
	msg.ReadString( string, MAX_STRING_CHARS );
	updateMSG = string;
	updateDirectDownload = ( msg.ReadByte() != 0 );
	msg.ReadString( string, MAX_STRING_CHARS );
	updateURL = string;
	updateMime = (dlMime_t)msg.ReadByte();
	msg.ReadString( string, MAX_STRING_CHARS );
	updateFallback = string;
	updateState = UPDATE_READY;
}

/*
==================
idAsyncClient::ValidatePureServerChecksums
==================
*/
bool idAsyncClient::ValidatePureServerChecksums( const netadr_t from, const idBitMsg &msg ) {
	int			i, numChecksums, numMissingChecksums;
	int			inChecksums[ MAX_PURE_PAKS ];
	int			missingChecksums[ MAX_PURE_PAKS ];
	idBitMsg	dlmsg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	// read checksums
	// pak checksums, in a 0-terminated list
	numChecksums = 0;
	do {
		i = msg.ReadInt( );
		inChecksums[ numChecksums++ ] = i;
		// just to make sure a broken message doesn't crash us
		if ( numChecksums >= MAX_PURE_PAKS ) {
			common->Warning( "MAX_PURE_PAKS ( %d ) exceeded in idAsyncClient::ProcessPureMessage\n", MAX_PURE_PAKS );
			return false;
		}
	} while ( i );
	inChecksums[ numChecksums ] = 0;

	fsPureReply_t reply = fileSystem->SetPureServerChecksums( inChecksums, missingChecksums );
	switch ( reply ) {
		case PURE_RESTART:
			// need to restart the filesystem with a different pure configuration
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
			// restart with the right FS configuration and get back to the server
			clientState = CS_PURERESTART;
			fileSystem->SetRestartChecksums( inChecksums );
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "reloadEngine" );
			return false;
		case PURE_MISSING: {

			idStr checksums;

			i = 0;
			while ( missingChecksums[ i ] ) {
				checksums += va( "0x%x ", missingChecksums[ i++ ] );
			}
			numMissingChecksums = i;

			if ( idAsyncNetwork::clientDownload.GetInteger() == 0 ) {
				// never any downloads
				idStr message = va( common->GetLanguageDict()->GetString( "#str_07210" ), Sys_NetAdrToString( from ) );

				if ( numMissingChecksums > 0 ) {
					message += va( common->GetLanguageDict()->GetString( "#str_06751" ), numMissingChecksums, checksums.c_str() );
				}

				common->Printf( message );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
			} else {
				if ( clientState >= CS_CONNECTED ) {
					// we are already connected, reconnect to negociate the paks in connectionless mode
					cmdSystem->BufferCommandText( CMD_EXEC_NOW, "reconnect" );
					return false;
				}
				// ask the server to send back download info
				common->DPrintf( "missing %d paks: %s\n", numMissingChecksums, checksums.c_str() );
				// store the requested downloads
				GetDownloadRequest( missingChecksums, numMissingChecksums );
				// build the download request message
				// NOTE: in a specific function?
				dlmsg.Init( msgBuf, sizeof( msgBuf ) );
				dlmsg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
				dlmsg.WriteString( "downloadRequest" );
				dlmsg.WriteInt( serverChallenge );
				dlmsg.WriteShort( clientId );
				// used to make sure the server replies to the same download request
				dlmsg.WriteInt( dlRequest );
				// special case the code pak - if we have a 0 checksum then we don't need to download it
				// 0-terminated list of missing paks
				i = 0;
				while ( missingChecksums[ i ] ) {
					dlmsg.WriteInt( missingChecksums[ i++ ] );
				}
				dlmsg.WriteInt( 0 );
				clientPort.SendPacket( from, dlmsg.GetData(), dlmsg.GetSize() );
			}

			return false;
		}
		default:
			break;
	}

	return true;
}

/*
==================
idAsyncClient::ProcessPureMessage
==================
*/
void idAsyncClient::ProcessPureMessage( const netadr_t from, const idBitMsg &msg ) {
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_MESSAGE_SIZE ];
	int			i;
	int			inChecksums[ MAX_PURE_PAKS ];

	if ( clientState != CS_CONNECTING ) {
		common->Printf( "clientState != CS_CONNECTING, pure msg ignored\n" );
		return;
	}

	if ( !ValidatePureServerChecksums( from, msg ) ) {
		return;
	}

	fileSystem->GetPureServerChecksums( inChecksums );
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	outMsg.WriteString( "pureClient" );
	outMsg.WriteInt( serverChallenge );
	outMsg.WriteShort( clientId );
	i = 0;
	while ( inChecksums[ i ] ) {
		outMsg.WriteInt( inChecksums[ i++ ] );
	}
	outMsg.WriteInt( 0 );

	clientPort.SendPacket( from, outMsg.GetData(), outMsg.GetSize() );
}

/*
==================
idAsyncClient::ConnectionlessMessage
==================
*/
void idAsyncClient::ConnectionlessMessage( const netadr_t from, const idBitMsg &msg ) {
	char string[MAX_STRING_CHARS*2];  // M. Quinn - Even Balance - PB packets can go beyond 1024

	msg.ReadString( string, sizeof( string ) );

	// info response from a server, are accepted from any source
	if ( idStr::Icmp( string, "infoResponse" ) == 0 ) {
		ProcessInfoResponseMessage( from, msg );
		return;
	}

	// from master server:
	if ( Sys_CompareNetAdrBase( from, idAsyncNetwork::GetMasterAddress( ) ) ) {
		// server list
		if ( idStr::Icmp( string, "servers" ) == 0 ) {
			ProcessServersListMessage( from, msg );
			return;
		}

		if ( idStr::Icmp( string, "authKey" ) == 0 ) {
			ProcessAuthKeyMessage( from, msg );
			return;
		}

		if ( idStr::Icmp( string, "newVersion" ) == 0 ) {
			ProcessVersionMessage( from, msg );
			return;
		}
	}

	// ignore if not from the current/last server
	if ( !Sys_CompareNetAdrBase( from, serverAddress ) && ( lastRconTime + 10000 < realTime || !Sys_CompareNetAdrBase( from, lastRconAddress ) ) ) {
		common->DPrintf( "got message '%s' from bad source: %s\n", string, Sys_NetAdrToString( from ) );
		return;
	}

	// challenge response from the server we are connecting to
	if ( idStr::Icmp( string, "challengeResponse" ) == 0 ) {
		ProcessChallengeResponseMessage( from, msg );
		return;
	}

	// connect response from the server we are connecting to
	if ( idStr::Icmp( string, "connectResponse" ) == 0 ) {
		ProcessConnectResponseMessage( from, msg );
		return;
	}

	// a disconnect message from the server, which will happen if the server
	// dropped the connection but is still getting packets from this client
	if ( idStr::Icmp( string, "disconnect" ) == 0 ) {
		ProcessDisconnectMessage( from, msg );
		return;
	}

	// print request from server
	if ( idStr::Icmp( string, "print" ) == 0 ) {
		ProcessPrintMessage( from, msg );
		return;
	}

	// server pure list
	if ( idStr::Icmp( string, "pureServer" ) == 0 ) {
		ProcessPureMessage( from, msg );
		return;
	}

	if ( idStr::Icmp( string, "downloadInfo" ) == 0 ) {
		ProcessDownloadInfoMessage( from, msg );
	}

	if ( idStr::Icmp( string, "authrequired" ) == 0 ) {
		// server telling us that he's expecting an auth mode connect, just in case we're trying to connect in LAN mode
		if ( idAsyncNetwork::LANServer.GetBool() ) {
			common->Warning( "server %s requests master authorization for this client. Turning off LAN mode\n", Sys_NetAdrToString( from ) );
			idAsyncNetwork::LANServer.SetBool( false );
		}
	}

	common->DPrintf( "ignored message from %s: %s\n", Sys_NetAdrToString( from ), string );
}

/*
=================
idAsyncClient::ProcessMessage
=================
*/
void idAsyncClient::ProcessMessage( const netadr_t from, idBitMsg &msg ) {
	int id;

	id = msg.ReadShort();

	// check for a connectionless packet
	if ( id == CONNECTIONLESS_MESSAGE_ID ) {
		ConnectionlessMessage( from, msg );
		return;
	}

	if ( clientState < CS_CONNECTED ) {
		return;		// can't be a valid sequenced packet
	}

	if ( msg.GetRemaingData() < 4 ) {
		common->DPrintf( "%s: tiny packet\n", Sys_NetAdrToString( from ) );
		return;
	}

	// is this a packet from the server
	if ( !Sys_CompareNetAdrBase( from, channel.GetRemoteAddress() ) || id != serverId ) {
		common->DPrintf( "%s: sequenced server packet without connection\n", Sys_NetAdrToString( from ) );
		return;
	}

	if ( !channel.Process( from, clientTime, msg, serverMessageSequence ) ) {
		return;		// out of order, duplicated, fragment, etc.
	}

	lastPacketTime = clientTime;
	ProcessReliableServerMessages();
	ProcessUnreliableServerMessage( msg );
}

/*
==================
idAsyncClient::SetupConnection
==================
*/
void idAsyncClient::SetupConnection( void ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	if ( clientTime - lastConnectTime < SETUP_CONNECTION_RESEND_TIME ) {
		return;
	}

	if ( clientState == CS_CHALLENGING ) {
		common->Printf( "sending challenge to %s\n", Sys_NetAdrToString( serverAddress ) );
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
		msg.WriteString( "challenge" );
		msg.WriteInt( clientId );
		clientPort.SendPacket( serverAddress, msg.GetData(), msg.GetSize() );
	} else if ( clientState == CS_CONNECTING ) {
		common->Printf( "sending connect to %s with challenge 0x%x\n", Sys_NetAdrToString( serverAddress ), serverChallenge );
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
		msg.WriteString( "connect" );
		msg.WriteInt( ASYNC_PROTOCOL_VERSION );
		msg.WriteInt( clientDataChecksum );
		msg.WriteInt( serverChallenge );
		msg.WriteShort( clientId );
		msg.WriteInt( cvarSystem->GetCVarInteger( "net_clientMaxRate" ) );
		msg.WriteString( cvarSystem->GetCVarString( "com_guid" ) );
		msg.WriteString( cvarSystem->GetCVarString( "password" ), -1, false );
		// do not make the protocol depend on PB
		msg.WriteShort( 0 );
		clientPort.SendPacket( serverAddress, msg.GetData(), msg.GetSize() );
#ifdef ID_ENFORCE_KEY_CLIENT
		if ( idAsyncNetwork::LANServer.GetBool() ) {
			common->Printf( "net_LANServer is set, connecting in LAN mode\n" );
		} else {
			// emit a cd key authorization request
			// modified at protocol 1.37 for XP key addition
			msg.BeginWriting();
			msg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
			msg.WriteString( "clAuth" );
			msg.WriteInt( ASYNC_PROTOCOL_VERSION );
			msg.WriteNetadr( serverAddress );
			// if we don't have a com_guid, this will request a direct reply from auth with it
			msg.WriteByte( cvarSystem->GetCVarString( "com_guid" )[0] ? 1 : 0 );
			// send the main key, and flag an extra byte to add XP key
			msg.WriteString( "000000" );
			const char *xpkey = "00000";
			msg.WriteByte( xpkey ? 1 : 0 );
			if ( xpkey ) {
				msg.WriteString( xpkey );
			}
			clientPort.SendPacket( idAsyncNetwork::GetMasterAddress(), msg.GetData(), msg.GetSize() );
		}
#else
		if (! Sys_IsLANAddress( serverAddress ) ) {
			common->Printf( "Build Does not have CD Key Enforcement enabled. The Server ( %s ) is not within the lan addresses. Attemting to connect.\n", Sys_NetAdrToString( serverAddress ) );
		}
		common->Printf( "Not Testing key.\n" );
#endif
	} else {
		return;
	}

	lastConnectTime = clientTime;
}

/*
==================
idAsyncClient::SendReliableGameMessage
==================
*/
void idAsyncClient::SendReliableGameMessage( const idBitMsg &msg ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	if ( clientState < CS_INGAME ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( CLIENT_RELIABLE_MESSAGE_GAME );
	outMsg.WriteData( msg.GetData(), msg.GetSize() );
	if ( !channel.SendReliableMessage( outMsg ) ) {
		common->Error( "client->server reliable messages overflow\n" );
	}
}

/*
==================
idAsyncClient::Idle
==================
*/
void idAsyncClient::Idle( void ) {
	// also need to read mouse for the connecting guis
	usercmdGen->GetDirectUsercmd();

	SendEmptyToServer();
}

/*
==================
idAsyncClient::UpdateTime
==================
*/
int idAsyncClient::UpdateTime( int clamp ) {
	int time, msec;

	time = Sys_Milliseconds();
	msec = idMath::ClampInt( 0, clamp, time - realTime );
	realTime = time;
	clientTime += msec;
	return msec;
}

/*
==================
idAsyncClient::RunFrame
==================
*/
void idAsyncClient::RunFrame( void ) {
	int			msec, size;
	bool		newPacket;
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];
	netadr_t	from;

	msec = UpdateTime( 100 );

	if ( !clientPort.GetPort() ) {
		return;
	}

	// handle ongoing pk4 downloads and patch downloads
	HandleDownloads();

	gameTimeResidual += msec;

	// spin in place processing incoming packets until enough time lapsed to run a new game frame
	do {

		do {

			// blocking read with game time residual timeout
			newPacket = clientPort.GetPacketBlocking( from, msgBuf, size, sizeof( msgBuf ), USERCMD_MSEC - ( gameTimeResidual + clientPredictTime ) - 1 );
			if ( newPacket ) {
				msg.Init( msgBuf, sizeof( msgBuf ) );
				msg.SetSize( size );
				msg.BeginReading();
				ProcessMessage( from, msg );
			}

			msec = UpdateTime( 100 );
			gameTimeResidual += msec;

		} while( newPacket );

	} while( gameTimeResidual + clientPredictTime < USERCMD_MSEC );

	// update server list
	serverList.RunFrame();

	if ( clientState == CS_DISCONNECTED ) {
		usercmdGen->GetDirectUsercmd();
		gameTimeResidual = USERCMD_MSEC - 1;
		clientPredictTime = 0;
		return;
	}

	if ( clientState == CS_PURERESTART ) {
		clientState = CS_DISCONNECTED;
		Reconnect();
		gameTimeResidual = USERCMD_MSEC - 1;
		clientPredictTime = 0;
		return;
	}

	// if not connected setup a connection
	if ( clientState < CS_CONNECTED ) {
		// also need to read mouse for the connecting guis
		usercmdGen->GetDirectUsercmd();
		SetupConnection();
		gameTimeResidual = USERCMD_MSEC - 1;
		clientPredictTime = 0;
		return;
	}

	if ( CheckTimeout() ) {
		return;
	}

	// if not yet in the game send empty messages to keep data flowing through the channel
	if ( clientState < CS_INGAME ) {
		Idle();
		gameTimeResidual = 0;
		return;
	}

	// check for user info changes
	if ( cvarSystem->GetModifiedFlags() & CVAR_USERINFO ) {
		SendUserInfoToServer( );
		cvarSystem->ClearModifiedFlags( CVAR_USERINFO );
	}

	if ( gameTimeResidual + clientPredictTime >= USERCMD_MSEC ) {
		lastFrameDelta = 0;
	}

	// generate user commands for the predicted time
	while ( gameTimeResidual + clientPredictTime >= USERCMD_MSEC ) {

		// send the user commands of this client to the server
		SendUsercmdsToServer();

		// update time
		gameFrame++;
		gameTime += USERCMD_MSEC;
		gameTimeResidual -= USERCMD_MSEC;

		// run from the snapshot up to the local game frame
		while ( snapshotGameFrame < gameFrame ) {

			lastFrameDelta++;

			// duplicate usercmds for clients if no new ones are available
			DuplicateUsercmds( snapshotGameFrame, snapshotGameTime );

			// indicate the last prediction frame before a render
			bool lastPredictFrame = ( snapshotGameFrame + 1 >= gameFrame && gameTimeResidual + clientPredictTime < USERCMD_MSEC );

			snapshotGameFrame++;
			snapshotGameTime += USERCMD_MSEC;
		}
	}
}

/*
==================
idAsyncClient::PacifierUpdate
==================
*/
void idAsyncClient::PacifierUpdate( void ) {
	if ( !IsActive() ) {
		return;
	}
	realTime = Sys_Milliseconds();
	SendEmptyToServer( false, true );
}

/*
==================
idAsyncClient::SendVersionCheck
==================
*/
void idAsyncClient::SendVersionCheck( bool fromMenu ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	if ( updateState != UPDATE_NONE && !fromMenu ) {
		common->DPrintf( "up-to-date check was already performed\n" );
		return;
	}

	InitPort();
	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	msg.WriteString( "versionCheck" );
	msg.WriteInt( ASYNC_PROTOCOL_VERSION );
	msg.WriteString( cvarSystem->GetCVarString( "si_version" ) );
	msg.WriteString( cvarSystem->GetCVarString( "com_guid" ) );
	clientPort.SendPacket( idAsyncNetwork::GetMasterAddress(), msg.GetData(), msg.GetSize() );

	common->DPrintf( "sent a version check request\n" );

	updateState = UPDATE_SENT;
	updateSentTime = clientTime;
	showUpdateMessage = fromMenu;
}

/*
==================
idAsyncClient::SendVersionDLUpdate

sending those packets is not strictly necessary. just a way to tell the update server
about what is going on. allows the update server to have a more precise view of the overall
network load for the updates
==================
*/
void idAsyncClient::SendVersionDLUpdate( int state ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	msg.WriteString( "versionDL" );
	msg.WriteInt( ASYNC_PROTOCOL_VERSION );
	msg.WriteShort( state );
	clientPort.SendPacket( idAsyncNetwork::GetMasterAddress(), msg.GetData(), msg.GetSize() );
}

/*
==================
idAsyncClient::HandleDownloads
==================
*/
void idAsyncClient::HandleDownloads( void ) {

}

/*
===============
idAsyncClient::SendAuthCheck
===============
*/
bool idAsyncClient::SendAuthCheck( const char *cdkey, const char *xpkey ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	msg.WriteString( "gameAuth" );
	msg.WriteInt( ASYNC_PROTOCOL_VERSION );
	msg.WriteByte( cdkey ? 1 : 0 );
	msg.WriteString( cdkey ? cdkey : "" );
	msg.WriteByte( xpkey ? 1 : 0 );
	msg.WriteString( xpkey ? xpkey : "" );
	InitPort();
	clientPort.SendPacket( idAsyncNetwork::GetMasterAddress(), msg.GetData(), msg.GetSize() );
	return true;
}

/*
===============
idAsyncClient::CheckTimeout
===============
*/
bool idAsyncClient::CheckTimeout( void ) {
	if ( lastPacketTime > 0 && ( lastPacketTime + idAsyncNetwork::clientServerTimeout.GetInteger()*1000 < clientTime ) ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
		return true;
	}
	return false;
}

/*
===============
idAsyncClient::ProcessDownloadInfoMessage
===============
*/
void idAsyncClient::ProcessDownloadInfoMessage( const netadr_t from, const idBitMsg &msg ) {
	char			buf[ MAX_STRING_CHARS ];
	int				srvDlRequest = msg.ReadInt();
	int				infoType = msg.ReadByte();
	int				pakDl;
	int				pakIndex;

	pakDlEntry_t	entry;
	bool			gotAllFiles = true;
	idStr			sizeStr;
	bool			gotGame = false;

	if ( dlRequest == -1 || srvDlRequest != dlRequest ) {
		common->Warning( "bad download id from server, ignored" );
		return;
	}
	// mark the dlRequest as dead now whatever how we process it
	dlRequest = -1;

	if ( infoType == SERVER_DL_REDIRECT ) {
		msg.ReadString( buf, MAX_STRING_CHARS );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
	} else if ( infoType == SERVER_DL_LIST ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
		if ( dlList.Num() ) {
			common->Warning( "tried to process a download list while already busy downloading things" );
			return;
		}
		// read the URLs, check against what we requested, prompt for download
		pakIndex = -1;
		totalDlSize = 0;
		do {
			pakIndex++;
			pakDl = msg.ReadByte();
			if ( pakDl == SERVER_PAK_YES ) {
				if ( pakIndex == 0 ) {
					gotGame = true;
				}
				msg.ReadString( buf, MAX_STRING_CHARS );
				entry.filename = buf;
				msg.ReadString( buf, MAX_STRING_CHARS );
				entry.url = buf;
				entry.size = msg.ReadInt();
				// checksums are not transmitted, we read them from the dl request we sent
				entry.checksum = dlChecksums[ pakIndex ];
				totalDlSize += entry.size;
				dlList.Append( entry );
				common->Printf( "download %s from %s ( 0x%x )\n", entry.filename.c_str(), entry.url.c_str(), entry.checksum );
			} else if ( pakDl == SERVER_PAK_NO ) {
				msg.ReadString( buf, MAX_STRING_CHARS );
				entry.filename = buf;
				entry.url = "";
				entry.size = 0;
				entry.checksum = 0;
				dlList.Append( entry );
				// first pak is game pak, only fail it if we actually requested it
				if ( pakIndex != 0 || dlChecksums[ 0 ] != 0 ) {
					common->Printf( "no download offered for %s ( 0x%x )\n", entry.filename.c_str(), dlChecksums[ pakIndex ] );
					gotAllFiles = false;
				}
			} else {
				assert( pakDl == SERVER_PAK_END );
			}
		} while ( pakDl != SERVER_PAK_END );
		if ( dlList.Num() < dlCount ) {
			common->Printf( "%d files were ignored by the server\n", dlCount - dlList.Num() );
			gotAllFiles = false;
		}
		sizeStr.BestUnit( "%.2f", totalDlSize, MEASURE_SIZE );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
		if ( totalDlSize == 0 ) {
			// was no downloadable stuff for us
			// "Can't connect to the pure server: no downloads offered"
			// "Missing required files"
			dlList.Clear();
			return;
		}
		bool asked = false;
		if ( gotGame ) {
			asked = true;
			// "You need to download game code to connect to this server. Are you sure? You should only answer yes if you trust the server administrators."
			// "Missing game binaries"
		}
		if ( !gotAllFiles ) {
			asked = true;
			// "The server only offers to download some of the files required to connect ( %s ). Download anyway?"
			// "Missing required files"
		}
		if ( !asked && idAsyncNetwork::clientDownload.GetInteger() == 1 ) {
			// "You need to download some files to connect to this server ( %s ), proceed?"
			// "Missing required files"
		}
	} else {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
		// "You are missing some files to connect to this server, and the server doesn't provide downloads."
		// "Missing required files"
	}
}

/*
===============
idAsyncClient::GetDownloadRequest
===============
*/
int idAsyncClient::GetDownloadRequest( const int checksums[ MAX_PURE_PAKS ], int count ) {
	assert( !checksums[ count ] ); // 0-terminated
	if ( memcmp( dlChecksums, checksums, sizeof( int ) * count ) ) {
		idRandom newreq;

		memcpy( dlChecksums, checksums, sizeof( int ) * MAX_PURE_PAKS );

		newreq.SetSeed( Sys_Milliseconds() );
		dlRequest = newreq.RandomInt();
		dlCount = count;
		return dlRequest;
	}
	// this is the same dlRequest, we haven't heard from the server. keep the same id
	return dlRequest;
}
