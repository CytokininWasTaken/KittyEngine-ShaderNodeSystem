#pragma once


namespace KE
{
	class Script;
	class ScriptNode;
	struct ScriptMemberID;
}

namespace KE_EDITOR
{
	class NodeEditor;

	struct EditorAction
	{
		virtual void Do() = 0;
		virtual void Undo() = 0;
		virtual void Redo() = 0;

		KE::Script* myScript = nullptr;
		NodeEditor* myEditor = nullptr;
	};


	struct EditorActionStack
	{
		NodeEditor* owner;
		KE::Script* script;
		std::vector<EditorAction*> actions;
		std::vector<bool> multiActionStack;

		size_t currentIndex = 0;

		bool multiActionMode = false;

		void Init(NodeEditor* anOwner, KE::Script* aScript);

		void Undo()
		{
			if (currentIndex > 0)
			{
				currentIndex--;
				actions[currentIndex]->Undo();
				if (multiActionStack[currentIndex])
				{
					Undo();
				}
			}
		}

		void Redo()
		{
			if (currentIndex < actions.size())
			{
				actions[currentIndex]->Redo();
				currentIndex++;
				if (multiActionStack[currentIndex])
				{
					Redo();
				}
			}
		}

		void BeginMultiAction()
		{
			multiActionMode = true;
		}

		void PerformAction(EditorAction* anAction)
		{
			anAction->myEditor = owner;
			anAction->myScript = script;

			//clear anything after current index
			actions.erase(actions.begin() + currentIndex, actions.end());

			actions.push_back(anAction);
			multiActionStack.push_back(multiActionMode);

			currentIndex++;
			anAction->Do();
		}

		void EndMultiAction()
		{
			multiActionMode	= false;
		}
	};

	//

	struct AddNodeAction : public EditorAction
	{
		std::vector<std::pair<KE::ScriptMemberID, KE::ScriptMemberID>> connections;
		KE::ScriptNode* addedNode = nullptr;
		Vector2f nodePosition;

		AddNodeAction(KE::ScriptNode* aNode) : addedNode(aNode) {}
		void Do() override;
		void Undo() override;
		void Redo() override;
	};

	struct RemoveNodeAction : public EditorAction
	{
		std::vector<std::pair<KE::ScriptMemberID, KE::ScriptMemberID>> connections;
		KE::ScriptNode* removedNode = nullptr;
		Vector2f nodePosition;

		RemoveNodeAction(KE::ScriptNode* aNode) : removedNode(aNode) {}
		void Do() override;
		void Undo() override;
		void Redo() override;
	};

	struct AddLinkAction : public EditorAction
	{
		KE::ScriptMemberID from, to;

		AddLinkAction(KE::ScriptMemberID aFrom, KE::ScriptMemberID aTo) : from(aFrom), to(aTo) {}
		void Do() override;
		void Undo() override;
		void Redo() override;
	};

	struct RemoveLinkAction : public EditorAction
	{
		KE::ScriptMemberID from, to;

		RemoveLinkAction(KE::ScriptMemberID aFrom, KE::ScriptMemberID aTo) : from(aFrom), to(aTo) {}
		void Do() override;
		void Undo() override;
		void Redo() override;
	};

}