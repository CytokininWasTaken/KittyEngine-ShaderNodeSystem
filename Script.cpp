#include "stdafx.h"
#include "Script.h"

#include "NodeDatabase.h"
#include "nlohmann/json.hpp"
#include "Utility/BinaryIO.h"

namespace KE 
{
	//custom json conversion for PinValue
	void to_json(nlohmann::json& j, const KE::PinValue& p)
	{
		j["type"] = p.type;
		j["value"] = nlohmann::json::array();
		for (int i = 0; i < sizeof(KE::PinValue::value); i++)
		{
			j["value"].push_back(((char*)&p.value)[i]);
		}
	}

	void from_json(const nlohmann::json& j, KE::PinValue& p)
	{
		p.type = j["type"];
		for (int i = 0; i < sizeof(KE::PinValue::value); i++)
		{
			((char*)&p.value)[i] = j["value"][i].get<char>();
		}
	}
}

KE::ScriptMemberID KE::Script::IncrementNodeID()
{
	ScriptMemberID id;
	id.idParts.nodeID = nextNodeID++;
	return id;
}

KE::ScriptMemberID KE::Script::AddNode(ScriptNode* aNode)
{
	auto id = IncrementNodeID();
	aNode->ID = id;
	//nodes.push_back(aNode);
	nodes[aNode->ID] = aNode;
	aNode->Init();

	EvaluateEntryPoints();

	return aNode->ID;
}

void KE::Script::BehaviourlessInsertNode(ScriptNode* aNode)
{
	nodes[aNode->ID] = aNode;
	EvaluateEntryPoints();
}

void KE::Script::InsertNode(ScriptNode* aNode)
{
	nodes[aNode->ID] = aNode;
	aNode->Init();

	EvaluateEntryPoints();
}

void KE::Script::RemoveNode(ScriptMemberID anID)
{
	if (!nodes.contains(anID)) { return; }
	for (auto& inputPin : nodes[anID]->GetInputPins())
	{
		for (auto& connection : connections[inputPin.ID])
		{
			RemoveConnection(inputPin.ID, connection.to);
		}
	}

	for (auto& outputPin : nodes[anID]->GetOutputPins())
	{
		for (auto& connection : connections[outputPin.ID])
		{
			RemoveConnection(outputPin.ID, connection.to);
		}
	}

	nodes.erase(anID);

	EvaluateEntryPoints();
}

bool KE::Script::HasNodeOfType(const std::string& anInternalName) const
{
	for (auto& node : nodes)
	{
		if (node.second->GetName() == anInternalName)
		{
			return true;
		}
	}
	return false;
}

void KE::Script::AddConnection(ScriptMemberID from, ScriptMemberID to, bool aTwoWay)
{
	ScriptMemberID fromNodeID = from;
	fromNodeID.idParts.pinID = 0;

	ScriptMemberID toNodeID = to;
	toNodeID.idParts.pinID = 0;

	ScriptNode* fromNode = nodes[fromNodeID];
	ScriptNode* toNode = nodes[toNodeID];

	connections[from].push_back({to });
	if (aTwoWay)
	{
		connections[to].push_back({from });
	}

	KE::Pin* fromPin = GetPin(from);
	KE::Pin* toPin = GetPin(to);

	fromNode->OnConnectPin(fromPin, toPin);
	toNode->OnConnectPin(fromPin, toPin);
}

void KE::Script::RemoveConnection(ScriptMemberID from, ScriptMemberID to)
{
	auto& connectionsFrom = connections[from];
	for (size_t i = 0; i < connectionsFrom.size(); i++)
	{
		if (connectionsFrom[i].to == to)
		{
			connectionsFrom.erase(connectionsFrom.begin() + i);
			break;
		}
	}
	auto& connectionsTo = connections[to];
	for (size_t i = 0; i < connectionsTo.size(); i++)
	{
		if (connectionsTo[i].to == from)
		{
			connectionsTo.erase(connectionsTo.begin() + i);
			break;
		}
	}
}

KE::Pin* KE::Script::GetPin(ScriptMemberID anID) const
{
	ScriptMemberID nodeID = anID;
	nodeID.idParts.pinID = 0;
	ScriptNode* node = nodes.at(nodeID);

	if (!node) { return nullptr; }
	const int inputCount = static_cast<int>(node->GetInputPins().size());
	if (anID.idParts.pinID < inputCount)
	{
		return &node->GetInputPins()[anID.idParts.pinID];
	}
	else
	{
		return &node->GetOutputPins()[anID.idParts.pinID - inputCount];
	}
}

void KE::Script::SaveToFile(const std::string& aPath)
{
	//
	nlohmann::json scriptJson;
	
	scriptJson["nodes"] = nlohmann::json::array();
	for (auto& node : nodes)
	{
		nlohmann::json nodeJson;

		node.second->ExtraSerialize(&nodeJson,this);

		nodeJson["name"] = node.second->GetName();
		nodeJson["id"] = node.second->ID.combinedID;
		nodeJson["position"] = {nodePositions[node.second->ID].x, nodePositions[node.second->ID].y};
		nodeJson["inPins"] = nlohmann::json::array();
		nodeJson["outPins"] = nlohmann::json::array();

		for (auto& inputPin : node.second->GetInputPins())
		{
			nlohmann::json pinJson;
			pinJson["pinValue"] = inputPin.value;
			pinJson["pinName"] = inputPin.name;
			pinJson["pinType"] = inputPin.type;
			nodeJson["inPins"].push_back(pinJson);

		}

		for (auto& outputPin : node.second->GetOutputPins())
		{
			nlohmann::json pinJson;
			pinJson["pinValue"] = outputPin.value;
			pinJson["pinName"] = outputPin.name;
			pinJson["pinType"] = outputPin.type;
			nodeJson["outPins"].push_back(pinJson);
		}


		scriptJson["nodes"].push_back(nodeJson);

	}

	scriptJson["connections"] = nlohmann::json::array();
	for (auto& connection : connections)
	{
		for (auto& connectionTo : connection.second)
		{
			nlohmann::json connectionJson;
			connectionJson["from"] = connection.first.combinedID;
			connectionJson["to"] = connectionTo.to.combinedID;
			scriptJson["connections"].push_back(connectionJson);
		}
	}

	scriptJson["variables"] = nlohmann::json::array();
	for (auto& variable : scriptVariables)
	{
		nlohmann::json variableJson;
		variableJson["name"] = variable.first;
		variableJson["value"] = variable.second;
		scriptJson["variables"].push_back(variableJson);
	}

	scriptJson["comments"] = nlohmann::json::array();
	for (auto& comment : scriptComments)
	{
		nlohmann::json commentJson;
		commentJson["text"] = comment.text;
		commentJson["position"] = { comment.position.x, comment.position.y };
		commentJson["size"] = { comment.size.x, comment.size.y };
		commentJson["colour"] = { comment.colour.x, comment.colour.y, comment.colour.z, comment.colour.w };
		scriptJson["comments"].push_back(commentJson);
	}

	scriptJson["macros"] = nlohmann::json::array();
	for (auto& macro : macros)
	{
		nlohmann::json macroJson;
		macroJson["name"] = macro.first;
		macroJson["inputValues"] = nlohmann::json::array();
		macroJson["outputValues"] = nlohmann::json::array();
		macroJson["macroStartID"] = macro.second.macroStartID.combinedID;
		macroJson["macroEndID"] = macro.second.macroEndID.combinedID;

		for (auto& inputPin : macro.second.inputValues)
		{
			nlohmann::json pinJson;
			pinJson["pinValue"] = inputPin.value;
			pinJson["pinName"] = inputPin.name;
			pinJson["pinType"] = inputPin.type;
			macroJson["inputValues"].push_back(pinJson);
		}
		for (auto& outputPin : macro.second.outputValues)
		{
			nlohmann::json pinJson;
			pinJson["pinValue"] = outputPin.value;
			pinJson["pinName"] = outputPin.name;
			pinJson["pinType"] = outputPin.type;
			macroJson["outputValues"].push_back(pinJson);
		}
		scriptJson["macros"].push_back(macroJson);
	}

	//check if file is read only
	if (std::filesystem::exists(aPath) && 
		std::filesystem::is_regular_file(aPath) && 
		std::filesystem::status(aPath).permissions() == std::filesystem::perms::_File_attribute_readonly)
	{
		std::filesystem::permissions(aPath, std::filesystem::perms::owner_write, std::filesystem::perm_options::replace);
	}

	std::ofstream file(aPath);
	file << scriptJson.dump();
	file.close();
}

void KE::Script::LoadFromFile(const std::string& aPath)
{
	nextNodeID = 0;
	//delete all nodes
	for (auto& node : nodes)
	{
		delete node.second;
	}

	nodes.clear();
	connections.clear();
	macros.clear();
	scriptComments.clear();

	nlohmann::json scriptJson;
	std::ifstream file(aPath);

	try //try parsing it, if it fails, save the file and try again. If it fails again, we have a problem.
	{
		file >> scriptJson;
	}
	catch (const std::exception& )
	{
		file.close();
		SaveToFile(aPath);
		LoadFromFile(aPath); //cursed AF!
							 //if the script is empty, save it, then load it again, so that we have a valid script file.
		return;
	}


	for (auto& macro : scriptJson["macros"])
	{
		ScriptMacro readMacro;
		readMacro.inputValues.clear();

		readMacro.macroStartID.combinedID = macro["macroStartID"];
		readMacro.macroEndID.combinedID = macro["macroEndID"];

		for (auto& inputPin : macro["inputValues"])
		{
			Pin pin;
			pin.value = inputPin["pinValue"];
			pin.type = inputPin["pinType"];
			std::string pinName = inputPin["pinName"];
			strcpy_s(pin.name, pinName.c_str());
			readMacro.inputValues.push_back(pin);
		}

		for (auto& outputPin : macro["outputValues"])
		{
			Pin pin;
			pin.value = outputPin["pinValue"];
			pin.type = outputPin["pinType"];
			std::string pinName = outputPin["pinName"];
			strcpy_s(pin.name, pinName.c_str());
			readMacro.outputValues.push_back(pin);
		}

		readMacro.macroScript = this;

		macros[macro["name"]] = readMacro;
	}

	for (auto& variable : scriptJson["variables"])
	{
		scriptVariables[variable["name"]] = variable["value"];
	}

	for (auto& comment : scriptJson["comments"])
	{
		ScriptComment commentData;
		const std::string commentText = comment["text"];
		strcpy_s(commentData.text, commentText.c_str());
		commentData.position = { comment["position"][0], comment["position"][1] };
		commentData.size = { comment["size"][0], comment["size"][1] };
		commentData.colour = { comment["colour"][0], comment["colour"][1], comment["colour"][2], comment["colour"][3] };
		scriptComments.push_back(commentData);
	}

	for (auto& nodeJson : scriptJson["nodes"])
	{
		ScriptMemberID id;
		id.combinedID = nodeJson["id"];
		Vector2f position = { nodeJson["position"][0], nodeJson["position"][1] };
		const std::string& jsonName = nodeJson["name"];
		const std::string& internalName = KE::nodeTypeDatabase.nodeNameMap[jsonName];

		if (!KE::nodeTypeDatabase.nodeTypes.contains(internalName)) { continue; }
		ScriptNode* node = KE::nodeTypeDatabase.nodeTypes[internalName].createFunction(this);
		node->ID = id;
		InsertNode(node);

		node->ExtraDeserialize(&nodeJson, this);
		
		for (size_t i = 0; i < nodeJson["inPins"].size(); i++)
		{
			node->GetInputPins()[i].type = nodeJson["inPins"][i]["pinType"];
			node->GetInputPins()[i].value = nodeJson["inPins"][i]["pinValue"];
			std::string pinName = nodeJson["inPins"][i]["pinName"];
			strcpy_s(node->GetInputPins()[i].name, pinName.c_str());

		}
		
		for (size_t i = 0; i < nodeJson["outPins"].size(); i++)
		{
			node->GetOutputPins()[i].type = nodeJson["outPins"][i]["pinType"];
			node->GetOutputPins()[i].value = nodeJson["outPins"][i]["pinValue"];
			std::string pinName = nodeJson["outPins"][i]["pinName"];
			strcpy_s(node->GetOutputPins()[i].name, pinName.c_str());
		}
		

		nodePositions[id] = position;

		if (nextNodeID <= id.idParts.nodeID) // make sure we don't try making existing nodes later.
		{
			nextNodeID = id.idParts.nodeID + 1;
		}

	}

	for (auto& connectionJson : scriptJson["connections"])
	{
		ScriptMemberID from;
		from.combinedID = connectionJson["from"];
		ScriptMemberID to;
		to.combinedID = connectionJson["to"];

		if (!(nodes.contains(from.GetNodeID()) && nodes.contains(to.GetNodeID()))) { continue; }

		AddConnection(from, to,	false); //one way, since the other way will be added later!
	}


}

void KE::Script::EvaluateMacros()
{
	for (auto& node : nodes)
	{
		const void* customData = node.second->GetCustomData();
		if (strcmp(node.second->GetName(), "Macro Input") == 0)
		{
			const char* macroName = (const char*)customData;
			macros[macroName].macroStartID = node.second->ID;
			macros[macroName].inputValues.clear();
			for (auto& pin : node.second->GetOutputPins())
			{
				if (&pin == &node.second->GetOutputPins().back()) { break; } //skip the last pin, which is the macro end pin
				macros[macroName].inputValues.push_back(pin);
			}
		}
		else if (strcmp(node.second->GetName(), "Macro Output") == 0)
		{
			const char* macroName = (const char*)customData;
			macros[macroName].macroEndID = node.second->ID;
			macros[macroName].outputValues.clear();
			for (auto& pin : node.second->GetInputPins())
			{
				if (&pin == &node.second->GetInputPins().back()) { break; } //skip the last pin, which is the macro end pin
				macros[macroName].outputValues.push_back(pin);
			}
		}

	}
}

bool KE::Script::PinIsOutput(Pin* aPin) const
{
	return aPin->isOutput;
}

void KE::Script::EvaluateEntryPoints()
{
	for (int i = 0; i < static_cast<int>(EntryPointType::Count); i++)
	{
		entryPoints[i].clear();
	}

	for (auto& node : nodes)
	{
		if (strcmp(node.second->GetName(), "Awake") == 0)
		{
			entryPoints[static_cast<int>(EntryPointType::Awake)].push_back(node.second->ID);
		}
		else if (strcmp(node.second->GetName(), "Update") == 0)
		{
			entryPoints[static_cast<int>(EntryPointType::Update)].push_back(node.second->ID);
		}
	}
}
