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
	serverReloadingEngine = false;
	nextHeartbeatTime = 0;
	nextAsyncStatsTime = 0;
	noRconOutput = true;
	lastAuthTime = 0;
	servers.Clear();
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
idAsyncServer::Kill
==================
*/
void idAsyncServer::Kill( void ) {
	if ( !active ) {
		return;
	}

	Sys_Sleep( 10 );

	// reset any pureness
	fileSystem->ClearPureChecksums();

	active = false;

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
===============
idAsyncServer::PrintLocalServerInfo
see (client) "getInfo" -> (server) "infoResponse" -> (client)ProcessGetInfoMessage
===============
*/
void idAsyncServer::PrintLocalServerInfo( void ) {

}

/*
==================
idAsyncServer::ProcessMessage
==================
*/
bool idAsyncServer::ProcessMessage( const netadr_t from, idBitMsg &msg ) {
	int			id;
	idBitMsg	outMsg;

	id = msg.ReadShort();

	if ( msg.GetRemaingData() < 4 ) {
		common->DPrintf( "%s: tiny packet\n", Sys_NetAdrToString( from ) );
		return false;
	}

	// check for a connectionless message
	if ( id == CONNECTIONLESS_MESSAGE_ID ) {
		return ConnectionlessMessage( from, msg );
	}

	common->Printf("packet received from %s\n", Sys_NetAdrToString(from));

	return true;
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
	int			msec, size;
	idBitMsg	msg;
	byte		msgBuf[MAX_MESSAGE_SIZE];
	netadr_t	from;
	bool		newPacket;
	msec = UpdateTime( 100 );

	if ( !serverPort.GetPort() ) {
		return;
	}

	if (!active) {
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
					return;
				}
			}

			msec = UpdateTime( 100 );
			gameTimeResidual += msec;

		} while( newPacket );

	} while( gameTimeResidual < USERCMD_MSEC );

	idAsyncNetwork::serverMaxClientRate.ClearModified();
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
==================
idAsyncServer::ConnectionlessMessage
==================
*/
bool idAsyncServer::ConnectionlessMessage( const netadr_t from, const idBitMsg &msg ) {
	char		string[MAX_STRING_CHARS*2];  // M. Quinn - Even Balance - PB Packets need more than 1024

	msg.ReadString( string, sizeof( string ) );

	// receiving heartbeat
	if ( idStr::Icmp( string, "heartbeat" ) == 0 ) {
		ProcessHeartbeatMessage(from);
		return false;
	}
	if (idStr::Icmp(string, "getServers") == 0) {
		ProcessRequestServersMessage(from, msg);
		return false;
	}
	if (idStr::Icmp(string, "srvAuth") == 0) {
		ProcessAuthRequestMessage(from, msg);
		return false;
	}

	common->Printf("Receiving unknown packet from %s\n", Sys_NetAdrToString(from));
	return false;
}


bool idAsyncServer::ProcessHeartbeatMessage( const netadr_t from ){
	AddServerToMaster(from);
	common->Printf("Receiving heartbeat from %s\n", Sys_NetAdrToString(from));
	return true;
}

void idAsyncServer::ProcessRequestServersMessage( const netadr_t from, const idBitMsg &msg ) {
	common->Printf("Receiving getServers from %s\n", Sys_NetAdrToString(from));

	idBitMsg	outMsg;
	byte		msgBuf[MAX_MESSAGE_SIZE];

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteShort( CONNECTIONLESS_MESSAGE_ID );
	outMsg.WriteString( "servers" );
	for (int i=0; i < servers.Num(); i++) {
		outMsg.WriteByte(servers[i].address.ip[0]);
		outMsg.WriteByte(servers[i].address.ip[1]);
		outMsg.WriteByte(servers[i].address.ip[2]);
		outMsg.WriteByte(servers[i].address.ip[3]);
		outMsg.WriteShort(servers[i].address.port);
		common->Printf("Sending data... %s\n", Sys_NetAdrToString(from));
	}
	serverPort.SendPacket( from, outMsg.GetData(), outMsg.GetSize() );
	//a.ip[0], a.ip[1], a.ip[2], a.ip[3], a.port );
}

void idAsyncServer::ProcessAuthRequestMessage( const netadr_t from, const idBitMsg &msg ) {
	common->Printf("Receiving srvAuth from %s\n", Sys_NetAdrToString(from));
}

bool idAsyncServer::AddServerToMaster(const netadr_t from) {
	serverData_t sv;
	sv.address = from;
	sv.filterGameType = 0;
	sv.filterPassword = 0;
	sv.filterPlayers = 0;
	strcpy(sv.fsGame, "base"); //is all this necessary?
	if (servers.FindIndex(sv) != -1) {
		common->Printf("Server %s already in list\n", Sys_NetAdrToString(from));
	} else {
		common->Printf("Server %s added to list\n", Sys_NetAdrToString(from));
		servers.AddUnique(sv);
	}
	return true;
}