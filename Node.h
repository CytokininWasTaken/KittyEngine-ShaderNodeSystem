#pragma once

namespace KE
{
	class GameObject;
	class Component;
	class Collider;
	class ScriptRuntime;
	class Script;
	class NodeExecutor;

	constexpr int FLOW_OUT_MAX = 8;
	constexpr int PIN_STRING_MAX = 64;

	enum class NodeCategory
	{
		Math,
		Logic,
		Flow,
		Variable,
		Macro,

		AIFlow,
		AILeaf,
		AIDecorator,

		CodeLogic,
		CodeFlow,
		CodeMath,
		CodeEntryPoint,

		Count
	};
	enum class PinType
	{
		Flow, 
		Value,
		OutputValue,

		Count
	};
	enum class ValueType
	{
		FlowValue,
		Container,

		Bool,
		Int,
		Float,

		Vector2,
		Vector3,
		Vector4,
		Transform,
		String,

		GameObject,
		Component,

		CodeValue,

		Any,
		Count
	};
	enum class EntryPointType
	{
		Awake,
		Update,
		Function,
		Event,

		Count
	};

	enum class ValueAttributes
	{
		Numeric = 1 << 0,
	};
		
	struct ContainerValue
	{
		void* data = nullptr;
		int size = 0;
		ValueType type = ValueType::Count;
	};

	struct PinValue
	{
		ValueType type = ValueType::Count;
		union PinValueData
		{
			bool Bool;
			int Int;
			float Float;
			Vector2f Vector2;
			Vector3f Vector3;
			Vector4f Vector4;
			char String[PIN_STRING_MAX];
			ContainerValue Container;
			KE::GameObject* GameObject;
			KE::Component* Component;
			Transform Transform;
			void* Pointer;
			char CodeValue[PIN_STRING_MAX];

			PinValueData() { memset(this, 0, sizeof(*this)); }
			PinValueData(const auto& aValue = 0)
			{
				memset(this, 0, sizeof(*this));
				memcpy(this, &aValue, sizeof(aValue));
			}

		} value;

		bool operator==(const PinValue& aOther) const { return type == aOther.type && memcmp(&value, &aOther.value, sizeof(value)) == 0; }

		PinValue(ValueType aType = ValueType::Count) : type(aType) {}
		PinValue(ValueType aType = ValueType::Count, const auto& aValue = 0) : type(aType), value(aValue) {}
	};

	struct ScriptMemberID
	{
		union
		{
			struct
			{
				short nodeID;
				short pinID;
			} idParts;
			int combinedID = 0;
		};

		operator int() const { return combinedID; }
		ScriptMemberID(int aID = 0) : combinedID(aID) {}
		std::size_t operator()(const ScriptMemberID& id) const { return std::hash<int>()(id.combinedID); }
		ScriptMemberID GetNodeID() const { ScriptMemberID out; out.idParts.nodeID = idParts.nodeID; return out;}
		ScriptMemberID GetPinID() const { ScriptMemberID out; out.idParts.pinID = idParts.pinID; return out;}
	};

	struct Pin
	{
		ScriptMemberID ID;
		PinValue value;
		PinType type;
		char name[PIN_STRING_MAX] = "Pin";
		bool isOutput = false;

		void* codeData = nullptr;

		void SetName(const char* aName) { memcpy(name, aName, PIN_STRING_MAX); }
	};

	struct PinConnection
	{
		ScriptMemberID to;
	};

	//Base Node Class
	class ScriptNode
	{
	protected:
		std::vector<Pin> inputPins;
		std::vector<Pin> outputPins;

		Script* parentScript = nullptr;

	public:
		ScriptMemberID ID;

		ScriptNode() {};
		virtual ~ScriptNode() {}

		void SetID(ScriptMemberID anID);
		void AssignScript(Script* aScript) { parentScript = aScript; }

		virtual const char* GetName() const = 0;
		virtual NodeCategory GetCategory() const = 0;
		virtual const char* GetDescription() const = 0;
		virtual void Init() = 0;

		//custom rendering interface
		virtual void CustomRender() { __noop; }
		virtual bool DrawHeader() { return true; }
		virtual void BeginDrawNode() { __noop; }
		virtual void EndDrawNode() { __noop; }
		//

		virtual void ExtraSerialize(void* aOutJson, Script* aLoadingScript)	{ __noop; }
		virtual void ExtraDeserialize(void* aInJson, Script* aLoadingScript) { __noop; }

		virtual void Execute(NodeExecutor* anExecutor) { __noop; }
		virtual PinValue GetOutputPinValue(int aPinIndex, NodeExecutor* anExecutor) { return {outputPins[aPinIndex].value}; }

		virtual void OnConnectPin(Pin* aFromPin, Pin* aToPin) { __noop; };
		virtual bool TryAssignUnsupportedPinValue(int aPinIndex, PinValue aValue) { return false; }

		virtual void* GetCustomData() { return nullptr; }
		virtual void SetCustomData(void* data) { __noop; }

		virtual std::vector<Pin>& GetInputPins() { return inputPins; }
		virtual std::vector<Pin>& GetOutputPins() { return outputPins; }

		void AddInputPin(ValueType aValueType, PinType aType, const char* aName)
		{
			ScriptMemberID pinID = ID;
			pinID.idParts.pinID = (short)(inputPins.size() + outputPins.size());
			Pin pin = { pinID, PinValue{aValueType}, aType, "" };
			strcpy_s(pin.name, aName);
			pin.isOutput = false;
			inputPins.push_back(pin);
		}
		void AddOutputPin(ValueType aValueType, PinType aType, const char* aName)
		{
			ScriptMemberID pinID = ID;
			pinID.idParts.pinID = (short)(inputPins.size() + outputPins.size());
			Pin pin = { pinID, PinValue{aValueType}, aType, "" };
			strcpy_s(pin.name, aName);
			pin.isOutput = true;
			outputPins.push_back(pin);
		}
	};
}
