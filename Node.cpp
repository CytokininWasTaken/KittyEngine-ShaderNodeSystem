#include "stdafx.h"
#include "Node.h"

//lol u thought there was code here
void KE::ScriptNode::SetID(ScriptMemberID anID)
{
	ID = anID;
	for (auto& pin : inputPins)
	{
		pin.ID.idParts.nodeID = anID.idParts.nodeID;
	}
	for (auto& pin : outputPins)
	{
		pin.ID.idParts.nodeID = anID.idParts.nodeID;
	}
}
