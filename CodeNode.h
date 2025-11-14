#pragma once
#include <format>

#include "LanguageData.h"
#include "Node.h"

struct ID3D11ShaderResourceView;

namespace KE
{
	class Graphics;
	class ScriptNode;
	enum class NodeCategory;

	enum class CodeVariableMutability
	{
		Constant,
		Variable,

		Count
	};

	inline const char* EnumToString(CodeVariableMutability mutability)
	{
		switch (mutability)
		{
		case CodeVariableMutability::Constant: return "const";
		case CodeVariableMutability::Variable: return "";
		default: return "unknown";
		}
	}


	enum class CodePinType
	{
		Flow,
		Value
	};
	
	struct CodePinValue
	{
		DataType type;
	};

	struct CodePin
	{
		CodePinType type;
		CodePinValue value;
	};

#pragma region CodeVariableTypes
	inline const char* MutabilityToString(CodeVariableMutability mutability)
	{
		switch (mutability)
		{
		case CodeVariableMutability::Constant: return "const";
		case CodeVariableMutability::Variable: return "";
		default: return "unknown";
		}
	}

	enum class CodeVariableLevel
	{
		Register,
		Declare,
		Define,

		Count
	};

	inline const char* EnumToString(CodeVariableLevel level)
	{
		switch (level)
		{
		case CodeVariableLevel::Register: return "register";
		case CodeVariableLevel::Declare:  return "declare";
		case CodeVariableLevel::Define:   return "define";
		default: return "unknown";
		}
	}

#pragma endregion

#pragma region CodeOperation

	enum class CodeOperation
	{
		None,

		CallFunction,

		Add,
		Subtract,
		Multiply,
		Divide,

		Count
	};

	inline const char* CodeMathOperationToString(CodeOperation operation)
	{
		switch (operation)
		{
		case CodeOperation::Add:	  return "+";
		case CodeOperation::Subtract: return "-";
		case CodeOperation::Multiply: return "*";
		case CodeOperation::Divide:	  return "/";

		default: return "";
		}
	}

	inline CodeOperation StringToCodeMathOperation(const std::string& operation)
	{
		if (operation == "+") return CodeOperation::Add;
		if (operation == "-") return CodeOperation::Subtract;
		if (operation == "*") return CodeOperation::Multiply;
		if (operation == "/") return CodeOperation::Divide;

		return CodeOperation::None;
	}

#pragma endregion

	inline std::string CreateVariableName(ScriptMemberID id)
	{
		return "var_" + std::to_string(id.idParts.nodeID) + "_" + std::to_string(id.idParts.pinID);
	}

	inline std::string GetReplaceableVariableName(ScriptMemberID id)
	{
		return std::format("${}$", id.combinedID);
	}

	inline std::string GetReplaceableVariableName(const std::vector<ScriptMemberID>& ids)
	{
		std::string out;
		for (const auto& id : ids)
		{
			if (!out.empty())
			{
				out += ", ";
			}
			out += std::format("${}$", id.combinedID);
		}

		return out;
	}


	struct ScopePinList
	{
		std::vector<ScriptMemberID> pins = {};
		bool Contains(ScriptMemberID anID) const { return std::find(pins.begin(), pins.end(), anID) != pins.end(); }
	};

	struct CodeScope
	{

		ScriptMemberID scopeOwnerID;
		ScopePinList scopePins;

		std::vector<ScriptMemberID> scopeMembers;

		std::string scopeStart;
		std::string scopeEnd;

		CodeScope* parentScope = nullptr;
		std::vector<CodeScope*> childScopes;

		bool hasClosed = false;
		//

		size_t GetScopeDepth() const
		{
			return parentScope ? parentScope->GetScopeDepth() + 1 : 0;
		}

		std::string Open()
		{
			return scopeStart;
		}

		std::string Close()
		{
			hasClosed = true;
			return scopeEnd;
		}
	};

	struct CodeReturn
	{
		ScriptMemberID originID;

		std::string parsedReturn;
	};

	struct CodeVariable
	{
		//CodeVariableType type;
		DataType type;
		CodeVariableMutability mutability;

		ScriptMemberID originID; //i *think* this will always be a Pin ID, but I can not see the future

		CodeVariableLevel level;

		std::string parsedMutability;
		std::string parsedType;
		std::string parsedName;
		std::string parsedAssignment;
	};

	struct CodeAssign
	{
		ScriptMemberID assignFrom;
		std::string assignTo;

		std::string parsedAssignment;
	};

	struct CodeText
	{
		std::string text;
	};

#pragma region GenerationCommands

	/////////////////////
	//Predeclarations: //
	/////////////////////

	/////////////////////
	//Command Output:  //
	/////////////////////
	struct ParseResult
	{
		enum class ResultType
		{
			GenerateVariable,
			CreateScope,
			ReturnValue,
			AssignValue,
			AddText,

			Count
		} type = ResultType::Count;

		union
		{
			CodeVariable* variable;
			CodeScope*	  scope;
			CodeReturn*	  returnValue;
			CodeAssign*  assign;
			CodeText*	  text;
		};

		ParseResult() { type = ResultType::Count; variable = nullptr; }
		ParseResult(CodeVariable* aVariable ) { type = ResultType::GenerateVariable; variable = aVariable;  }
		ParseResult(CodeScope*	  aCodeScope) { type = ResultType::CreateScope;		 scope = aCodeScope;    }
		ParseResult(CodeReturn*	  aReturn)	  { type = ResultType::ReturnValue;		 returnValue = aReturn; }
		ParseResult(CodeAssign*	  aAssign)	  { type = ResultType::AssignValue;		 assign = aAssign; }
		ParseResult(CodeText*	  aText)	  { type = ResultType::AddText;			 text = aText; }
		~ParseResult()
		{
			switch(type)
			{
				case ResultType::GenerateVariable: delete variable; break;
				case ResultType::CreateScope:	   delete scope; break;
				case ResultType::ReturnValue:	   delete returnValue; break;
				case ResultType::AssignValue:	   delete assign; break;
				case ResultType::AddText:	   delete text; break;
			}
		}
	};

	/////////////////////
	//Command Base:    //
	/////////////////////
	struct GenerationCommand
	{
		virtual ParseResult Parse() { return {}; };
		virtual ~GenerationCommand() = default;
	};

	//////////////////
	//Command Types://
	//////////////////
	struct AssignmentReference
	{
		std::vector<ScriptMemberID> referenceIDs;

		AssignmentReference(const std::vector<ScriptMemberID>& aReferenceIDs) : referenceIDs(aReferenceIDs) {}
		AssignmentReference(ScriptMemberID aReferenceID) : referenceIDs({aReferenceID}) {}
	};

	struct GenerateVariableCommand : public GenerationCommand
	{
		struct Data
		{
			CodeVariableLevel level = CodeVariableLevel::Define;

			std::vector<std::pair<AssignmentReference, CodeOperation>> assignmentInputs;
			std::string optionalData;
		};

		//

		ScriptMemberID origin;


		DataType type;
		CodeVariableMutability mutability;

		Data data = {};

		std::string EvaluateAssignment()
		{
			std::string assignment = "";

			
			for (auto& input : data.assignmentInputs)
			{
				if (input.second == CodeOperation::CallFunction)
				{
					assignment += std::format(" {}(", data.optionalData);
					for (const auto& id : input.first.referenceIDs)
					{
						if (id != input.first.referenceIDs.front()) { assignment += ", "; }
						assignment += GetReplaceableVariableName(id);
					}
					assignment += ")";


				}
				else
				{
					assignment += std::format(" {} {}", GetReplaceableVariableName(input.first.referenceIDs[0]), CodeMathOperationToString(input.second));
				}
			}

			return std::format("={}", assignment);
		}

		ParseResult Parse() override
		{
			std::string outName; 
			std::string outMutability; 
			std::string outType; 
			std::string outAssignment; 


			if (data.level == CodeVariableLevel::Register && !data.optionalData.empty())
			{
				outName = data.optionalData;
				outMutability = "";
				outAssignment = "";
			}
			else
			{
				outName = "var_" + std::to_string(origin.idParts.nodeID) + "_" + std::to_string(origin.idParts.pinID);
				outMutability = MutabilityToString(mutability);
				outAssignment = EvaluateAssignment();
			}

			outType = type.typeName;

			return ParseResult(
				new CodeVariable(
					type, mutability, origin,
					//std::format(
						//"{} {} {} {};\n",
						data.level,
						outMutability,
						outType,
						//CodeVariableTypeToString(type),
						outName,
						outAssignment
					//)
				)
			);
		}
	};

	struct CreateScopeCommand : public GenerationCommand
	{
		ScriptMemberID scopeOwnerID = {}; //the pin from which this scope originates

		ScopePinList pinList = {};

		std::string scopeStart;
		std::string scopeEnd;

		CreateScopeCommand()
		{
			scopeStart = "";
			scopeEnd = "";
		}

		ParseResult Parse() override
		{
			CodeScope* newScope = new CodeScope(scopeOwnerID, {});
			newScope->scopeStart = scopeStart;
			newScope->scopeEnd = scopeEnd;
			newScope->scopePins = pinList;

			return newScope;
		}
	};

	struct ReturnValueCommand : public GenerationCommand
	{
		ScriptMemberID origin;

		ParseResult Parse() override
		{
			return ParseResult(
				new CodeReturn(
					origin,
					GetReplaceableVariableName(origin)
				)
			);
		}
	};

	struct AssignValueCommand : public GenerationCommand
	{
		ScriptMemberID assignFrom;
		std::string assignTo;

		CodeOperation operation;

		ParseResult Parse() override
		{
			return ParseResult(
				new CodeAssign(
					assignFrom, 
					assignTo,
					std::format("{} {}", GetReplaceableVariableName(assignFrom), CodeMathOperationToString(operation))
				)
			);
		}
	};

	struct AddTextCommand : public GenerationCommand
	{
		std::string text;

		ParseResult Parse() override
		{
			return ParseResult(
				new CodeText(
					text
				)
			);
		}
	};

	//////////////////////
	//Command Container://
	//////////////////////
	struct CommandList
	{
	private:
		std::vector<GenerationCommand*> commands;
	public:
		template<typename T>
		T* Add() { T* madeCommand = new T(); commands.push_back(madeCommand); return (madeCommand); }
		std::vector<GenerationCommand*>& GetCommands() { return commands; }

		~CommandList()
		{
			for (auto& command : commands)
			{
				delete command;
			}
		}
	};

#pragma endregion

#pragma region RenderingContext

	/////////////////////
	//Rendering Context//
	/////////////////////

	struct CodeRenderingContext
	{
		Graphics* graphics;
		std::array<ID3D11ShaderResourceView*, 16> shaderTextures;

	};

#pragma endregion

	class IVariant
	{
	protected:
		unsigned int myVariantIndex = 0;
		unsigned int myVariantCount = 0;

	public:
		virtual ~IVariant() = default;

		virtual void SetVariantIndex(unsigned int aIndex) = 0;
		unsigned int GetVariantIndex() const { return myVariantIndex; }

		void SetVariantCount(unsigned int aCount) { myVariantCount = aCount; }
		unsigned int GetVariantCount() const { return myVariantCount; }
	};


	class CodeScriptNode : public ScriptNode
	{
	protected:

	public:
		CodeScriptNode() : ScriptNode() {};
		virtual ~CodeScriptNode() override = default;

		virtual const char* GetName() const override = 0;
		virtual NodeCategory GetCategory() const override = 0;
		virtual const char* GetDescription() const override = 0;
		virtual void Init() override = 0;
		virtual void Init(const LanguageDefinitionNew& aLanguage) { Init(); }

		virtual const char* GetCode() const { return ""; }
		virtual CommandList GetCommands(const LanguageDefinitionNew& aLanguage) { return CommandList(); }
	};

	//Dynamic nodes:
	class CodeFunctionNode : public CodeScriptNode, public IVariant
	{
	private:
		FunctionDefinition myFunctionDefinition;

		std::vector<CodePin> codePinData;

	public:

		const char* GetName() const override { return myFunctionDefinition.name.c_str(); }
		NodeCategory GetCategory() const override { return NodeCategory::CodeLogic; }
		const char* GetDescription() const override { return "dynamic node"; }

		void AddData(FunctionDefinition aFunction);

		virtual void Init() override;

		virtual CommandList GetCommands(const LanguageDefinitionNew& aLanguage) override;

		void SetVariantIndex(unsigned int aIndex) override;

		void ExtraSerialize(void* aOutJson, Script* aLoadingScript) override;
		void ExtraDeserialize(void* aInJson, Script* aLoadingScript) override;
	};

	class CodeStructNode : public CodeScriptNode
	{
	private:
		StructDefinition myStructDefinition;

		std::vector<CodePin> codePinData;

	public:
		const char* GetName() const override { return myStructDefinition.name.c_str(); }
		NodeCategory GetCategory() const override { return NodeCategory::CodeLogic; }
		const char* GetDescription() const override { return "dynamic node"; }

		void AddData(StructDefinition aStruct);

		virtual void Init() override;


		virtual CommandList GetCommands(const LanguageDefinitionNew& aLanguage) override;
	};

	class CodeBufferNode : public CodeScriptNode
	{
	private:
		BufferDefinition myBufferDefinition;

		std::vector<CodePin> codePinData;

	public:
		const char* GetName() const override { return myBufferDefinition.name.c_str(); }
		NodeCategory GetCategory() const override { return NodeCategory::CodeLogic; }
		const char* GetDescription() const override { return "dynamic node"; }

		void AddData(BufferDefinition aStruct);

		virtual void CustomRender() override;

		virtual void Init() override;


		virtual CommandList GetCommands(const LanguageDefinitionNew& aLanguage) override;
	};

	class CodeTextureNode : public CodeScriptNode
	{
	private:
		TextureDefinition myTextureDefinition;

		std::vector<CodePin> codePinData;

	public:
		const char* GetName() const override { return myTextureDefinition.name.c_str(); }
		NodeCategory GetCategory() const override { return NodeCategory::CodeLogic; }
		const char* GetDescription() const override { return "dynamic node"; }

		void AddData(TextureDefinition aTexture);

		virtual void CustomRender() override;

		virtual void Init() override;


		virtual CommandList GetCommands(const LanguageDefinitionNew& aLanguage) override;
	};

	class CodeEntryPointNode : public CodeScriptNode
	{
	private:
		EntrypointDefinition myEntrypointDefinition;
		std::string myName;
		std::vector<CodePin> codePinData;
	public:

		const char* GetName() const override { return myName.c_str(); }
		NodeCategory GetCategory() const override { return NodeCategory::CodeEntryPoint; }
		const char* GetDescription() const override { return "dynamic node"; }

		void AddData(const EntrypointDefinition& aEntrypoint);

		auto GetParameters() const;

		virtual void Init() override;

		virtual CommandList GetCommands(const LanguageDefinitionNew& aLanguage) override;
	};

	class CodeExitPointNode : public CodeScriptNode
	{
	private:
		EntrypointDefinition myEntrypointDefinition;

		std::string myName;

		std::vector<CodePin> codePinData;
	public:

		const char* GetName() const override { return myName.c_str(); }
		NodeCategory GetCategory() const override { return NodeCategory::CodeEntryPoint; }
		const char* GetDescription() const override { return "dynamic node"; }

		void AddData(const EntrypointDefinition& aEntrypoint);

		auto GetParameters() const;

		virtual void Init() override;

		virtual CommandList GetCommands(const LanguageDefinitionNew& aLanguage) override;
	};
}
