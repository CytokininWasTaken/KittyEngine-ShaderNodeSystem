#pragma once
#include "Script.h"
#include "LanguageData.h"

#define SCRIPT_LOAD_PATH "Data/InternalAssets/Scripts/"
namespace KE
{
	class ScriptManager
	{
	private:
		std::unordered_map<std::string, Script*> myScripts;

	public:
		ScriptManager() = default;
		~ScriptManager();

		void Init();

		Script* CreateScript(const std::string& aName);
		Script* GetScript(const std::string& aName) const;
		Script* GetOrLoadScript(const std::string& aName);

		void LoadScript(const std::string& aName, const std::string& aPath);
		void SaveScript(const std::string& aName, const std::string& aPath);
	};
}