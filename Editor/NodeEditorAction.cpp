#include  "stdafx.h"
#ifndef KITTYENGINE_NO_EDITOR

#include "Script/Node.h"
#include "NodeEditorAction.h"
#include "../EditorWindows/NodeEditor.h"
#include "Script/Script.h"



namespace KE_EDITOR
{
	void EditorActionStack::Init(NodeEditor* anOwner, KE::Script* aScript)
	{
		owner = anOwner;
		script = aScript;
	}

	//--//--//--//--//--//--// 

	void AddNodeAction::Do()
	{
		addedNode->Init();
		addedNode->SetID(myScript->IncrementNodeID());
		myScript->BehaviourlessInsertNode(addedNode);
		myEditor->AddConnections(connections);
	}

	void AddNodeAction::Undo()
	{
		nodePosition = myEditor->GetNodePosition(addedNode);
		connections.clear();
		myEditor->GetConnections(addedNode->ID, connections);
		myScript->RemoveNode(addedNode->ID);
	}

	void AddNodeAction::Redo()
	{
		myScript->BehaviourlessInsertNode(addedNode);
		myEditor->SetNodePosition(addedNode, nodePosition);
		myEditor->AddConnections(connections);

	}

	//--//--//--//--//--//--// 

	void RemoveNodeAction::Do()
	{
		nodePosition = myEditor->GetNodePosition(removedNode);
		connections.clear();
		myEditor->GetConnections(removedNode->ID, connections);
		myEditor->DeselectNode(removedNode->ID);
		myScript->RemoveNode(removedNode->ID);
	}

	void RemoveNodeAction::Undo()
	{
		myScript->BehaviourlessInsertNode(removedNode);
		myEditor->SetNodePosition(removedNode, nodePosition);
		myEditor->AddConnections(connections);
	}

	void RemoveNodeAction::Redo()
	{
		Do();
	}

	//--//--//--//--//--//--//

	void AddLinkAction::Do()
	{
		myScript->AddConnection(from, to);
	}

	void AddLinkAction::Undo()
	{
		myScript->RemoveConnection(from, to);
	}

	void AddLinkAction::Redo()
	{
		myScript->AddConnection(from, to);
	}

	//--//--//--//--//--//--//

	void RemoveLinkAction::Do()
	{
		myScript->RemoveConnection(from, to);
	}

	void RemoveLinkAction::Undo()
	{
		myScript->AddConnection(from, to);

	}

	void RemoveLinkAction::Redo()
	{
		myScript->RemoveConnection(from, to);
	}

	//--//--//--//--//--//--//


}
#endif