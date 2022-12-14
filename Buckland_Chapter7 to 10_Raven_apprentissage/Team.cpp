#include "Team.h"
#include "Messaging/Telegram.h"
#include "Raven_Messages.h"
#include "Messaging/MessageDispatcher.h"
#include "raven_bot.h"


Team::Team()
{
}

Team::Team(Vector2D lootposition, std::string name, int teamId)
{
	members = std::list<Raven_Bot*>();
	name_team = name;
	lootingLocation = lootposition;
	target = 0;
	m_teamId = teamId;
}

void Team::UpdateNewTarget(Raven_Bot* new_target, int id_sender) {
	target = new_target; //Add target
	std::list<Raven_Bot*>::iterator curBot = members.begin();
	Raven_Bot* leader = *curBot;
	for (curBot; curBot != members.end(); ++curBot) { //dispatch message to all members
		Raven_Bot* bot_courant = *curBot;
		Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
			id_sender,
			bot_courant->ID(),
			Msg_UpdatingTarget,
			NO_ADDITIONAL_INFO);
	}
}

void Team::ProtectLeader(int id_sender)
{
	std::list<Raven_Bot*>::iterator curBot = members.begin();
	Raven_Bot* leader = *curBot;
	for (curBot; curBot != members.end(); ++curBot) { //dispatch message to all members
		Raven_Bot* bot_courant = *curBot;
		if (!bot_courant->isPossessed())
		{
			Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
				id_sender,
				bot_courant->ID(),
				Msg_ProtectLeader,
				NO_ADDITIONAL_INFO);
		}
	}
}

void Team::ClearTarget(int id_sender) {
	target = 0;
	std::list<Raven_Bot*>::iterator curBot = members.begin();
	Raven_Bot* leader = *curBot;
	for (curBot; curBot != members.end(); ++curBot) { //dispatch message to all members
		Raven_Bot* bot_courant = *curBot;
		Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
			id_sender,
			bot_courant->ID(),
			Msg_TargetKilled,
			NO_ADDITIONAL_INFO);
	}
}

void Team::AddSpawnPoint(Vector2D spawnPoint) {
	m_SpawnPoints.push_back(spawnPoint);
}


void Team::Addmember(Raven_Bot* new_memeber)
{
	members.push_back(new_memeber);
}

Team::~Team()
{
}