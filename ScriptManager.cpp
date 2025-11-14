#include "stdafx.h"
#include "Script.h"
#include "ScriptManager.h"

#include "CodeNode.h"
#include "NodeDatabase.h"
#include "Graphics/Graphics.h"

KE::ScriptManager::~ScriptManager()
{
	for (auto& script : myScripts)
	{
		delete script.second;
	}
}

void KE::ScriptManager::Init()
{
	KE_GLOBAL::blackboard.Register("scriptManager", this);

	nodeTypeDatabase.RegisterLanguage("Data/EngineAssets/LanguageDefinitions/hlsl.json");
}

KE::Script* KE::ScriptManager::CreateScript(const std::string& aName)
{
	Script* script = new Script();
	script->SetLanguageDefinition(nodeTypeDatabase.myLanguageDefinitions["hlsl"]);
	auto* context = new CodeRenderingContext();
	context->graphics = KE_GLOBAL::blackboard.Get<KE::Graphics>("graphics");
	script->SetCodeRenderingContext(context);
	myScripts[aName] = script;
	return script;
}

KE::Script* KE::ScriptManager::GetScript(const std::string& aName) const
{
	return myScripts.contains(aName) ? myScripts.at(aName) : nullptr;
}

KE::Script* KE::ScriptManager::GetOrLoadScript(const std::string& aName)
{
	if (myScripts.contains(aName))
	{
		return myScripts.at(aName);
	}
	else
	{
		LoadScript(aName, SCRIPT_LOAD_PATH + aName + ".scritch");
		return myScripts.at(aName);
	}
}

void KE::ScriptManager::LoadScript(const std::string& aName, const std::string& aPath)
{
	CreateScript(aName)->LoadFromFile(aPath);
	myScripts[aName]->SetScriptFileInfo(aName, aPath);
}

void KE::ScriptManager::SaveScript(const std::string& aName, const std::string& aPath)
{
	myScripts[aName]->SaveToFile(aPath);
}
