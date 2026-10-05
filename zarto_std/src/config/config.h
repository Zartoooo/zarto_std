#pragma once

// ---------------------------------------------------------------------------
// zarto_std :: config system
//
// defining variables (namespace / global scope, once):
//	CREATE_CVAR(bool,     "menu.open",               false);
//	CREATE_CVAR(int,      "aimbot.fov",              100);
//	CREATE_CVAR(float,    "aimbot.smoothing.x",      1.1f);
//	CREATE_CVAR(col_rgba, "visuals.playeresp.color", col_rgba{ 255, 255, 255, 255 });
//
// every config var also gets a value accessor:
//	config::get_var<bool>("menu.open") = true;
//
// saving / loading (files live in config::CONFIG_DIRECTORY, one json per config):
//	config::save_config("aimbot");
//	config::load_config("aimbot");
//
// scanning the whole directory (validates + auto-upgrades every file):
//	for (const std::string& name : config::load_all_configs())
//		std::cout << name << '\n';
//	config::load_config("one-of-them");   // applies one config to the live vars
//
// versioning: each file stores "version". files matching
// CURRENT_CFG_SYSTEM_VERSION load directly, older files are upgraded in place
// through register_migrator(), files that cannot be upgraded (or are newer
// than this build) are skipped.
//
// custom types: specialize config::config_codec<T> with to_json()/from_json().
// ---------------------------------------------------------------------------

#include <any>
#include <cctype>
#include <charconv>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "json.hpp"
#include "../types/col_rgb.h"
#include "../types/col_rgba.h"

namespace config
{
	// version of this config system, written into every saved file
	constexpr const int CURRENT_CFG_SYSTEM_VERSION = 1;

	// directory all configs are stored in. override with
	// /DZARTO_CONFIG_DIRECTORY=\"C:/some/path\" if you need to.
#ifndef ZARTO_CONFIG_DIRECTORY
#define ZARTO_CONFIG_DIRECTORY "configs"
#endif
	inline constexpr const char* CONFIG_DIRECTORY = ZARTO_CONFIG_DIRECTORY;
	inline constexpr const char* CONFIG_FILE_EXTENSION = ".json";

	class i_config_var;
	using cvar_registry = std::unordered_map<std::string, i_config_var*>;

	// -----------------------------------------------------------------------
	// registry - every CREATE_CVAR lives here
	// -----------------------------------------------------------------------
	void register_var(i_config_var* var);
	void unregister_var(i_config_var* var);
	i_config_var* find_var(const std::string& cvarname);
	const cvar_registry& get_registry();
	void reset_to_defaults();

	// -----------------------------------------------------------------------
	// config document
	// -----------------------------------------------------------------------
	struct config {
		std::string name;                                                  // == file name without extension
		int version = CURRENT_CFG_SYSTEM_VERSION;
		nlohmann::json vars = nlohmann::json::object();
	};

	// the config that was last applied with load_config() / apply()
	inline std::unique_ptr<config> loaded_config;

	// -----------------------------------------------------------------------
	// type names (used for error messages)
	// -----------------------------------------------------------------------
	template <typename T>
	inline const std::string& type_name()
	{
		static const std::string name = []() -> std::string {
			if constexpr (std::is_same_v<T, bool>)             return "bool";
			else if constexpr (std::is_same_v<T, int>)         return "int";
			else if constexpr (std::is_same_v<T, float>)       return "float";
			else if constexpr (std::is_same_v<T, double>)      return "double";
			else if constexpr (std::is_same_v<T, std::string>) return "string";
			else if constexpr (std::is_same_v<T, col_rgb>)     return "col_rgb";
			else if constexpr (std::is_same_v<T, col_rgba>)    return "col_rgba";
			else                                               return typeid(T).name();
		}();
		return name;
	}

	// -----------------------------------------------------------------------
	// json handlers. specialize config_codec<T> for custom types.
	// -----------------------------------------------------------------------
	template <typename T>
	struct config_codec
	{
		// relies on nlohmann's implicit conversions (numbers, bool, string, ...)
		static nlohmann::json to_json(const T& value) { return value; }

		static bool from_json(const nlohmann::json& j, T& out)
		{
			try { out = j.get<T>(); return true; }
			catch (const std::exception&) { return false; }
		}
	};

	// bool also accepts numbers and "true"/"false" style strings
	template <>
	struct config_codec<bool>
	{
		static nlohmann::json to_json(const bool& value) { return value; }

		static bool from_json(const nlohmann::json& j, bool& out)
		{
			if (j.is_boolean()) { out = j.get<bool>(); return true; }
			if (j.is_number())  { out = j.get<double>() != 0.0; return true; }
			if (j.is_string())
			{
				std::string s = j.get<std::string>();
				for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				if (s == "true" || s == "1" || s == "yes" || s == "on")   { out = true;  return true; }
				if (s == "false" || s == "0" || s == "no" || s == "off")  { out = false; return true; }
			}
			return false;
		}
	};

	// float is written with the shortest round-trippable representation so
	// 1.1f is stored as 1.1 instead of 1.100000023841858
	template <>
	struct config_codec<float>
	{
		static nlohmann::json to_json(const float& value)
		{
			char buffer[64];
			const std::to_chars_result result = std::to_chars(buffer, buffer + sizeof(buffer), value);
			if (result.ec == std::errc())
			{
				nlohmann::json parsed = nlohmann::json::parse(std::string_view(buffer, result.ptr - buffer), nullptr, false);
				if (!parsed.is_discarded())
					return parsed;
			}
			return value;
		}

		static bool from_json(const nlohmann::json& j, float& out)
		{
			if (j.is_number()) { out = static_cast<float>(j.get<double>()); return true; }
			if (j.is_string())
			{
				try { out = std::stof(j.get<std::string>()); return true; }
				catch (const std::exception&) { return false; }
			}
			return false;
		}
	};

	// { "r": .., "g": .., "b": .. }  or [r, g, b]
	template <>
	struct config_codec<col_rgb>
	{
		static nlohmann::json to_json(const col_rgb& value)
		{
			return nlohmann::json{ { "r", value.r }, { "g", value.g }, { "b", value.b } };
		}

		static bool from_json(const nlohmann::json& j, col_rgb& out)
		{
			try
			{
				if (j.is_array())
				{
					if (j.size() < 3) return false;
					out.r = j[0].get<uint8_t>();
					out.g = j[1].get<uint8_t>();
					out.b = j[2].get<uint8_t>();
					return true;
				}
				if (j.is_object())
				{
					if (!j.contains("r") || !j.contains("g") || !j.contains("b")) return false;
					out.r = j["r"].get<uint8_t>();
					out.g = j["g"].get<uint8_t>();
					out.b = j["b"].get<uint8_t>();
					return true;
				}
			}
			catch (const std::exception&) { return false; }
			return false;
		}
	};

	// { "r": .., "g": .., "b": .., "a": .. }  or [r, g, b] / [r, g, b, a] (alpha optional)
	template <>
	struct config_codec<col_rgba>
	{
		static nlohmann::json to_json(const col_rgba& value)
		{
			return nlohmann::json{ { "r", value.r }, { "g", value.g }, { "b", value.b }, { "a", value.a } };
		}

		static bool from_json(const nlohmann::json& j, col_rgba& out)
		{
			try
			{
				if (j.is_array())
				{
					if (j.size() < 3) return false;
					out.r = j[0].get<uint8_t>();
					out.g = j[1].get<uint8_t>();
					out.b = j[2].get<uint8_t>();
					if (j.size() >= 4) out.a = j[3].get<uint8_t>();
					return true;
				}
				if (j.is_object())
				{
					if (!j.contains("r") || !j.contains("g") || !j.contains("b")) return false;
					out.r = j["r"].get<uint8_t>();
					out.g = j["g"].get<uint8_t>();
					out.b = j["b"].get<uint8_t>();
					if (j.contains("a")) out.a = j["a"].get<uint8_t>();
					return true;
				}
			}
			catch (const std::exception&) { return false; }
			return false;
		}
	};

	// -----------------------------------------------------------------------
	// config variable
	// -----------------------------------------------------------------------
	class i_config_var
	{
	public:
		virtual ~i_config_var() = default;
		explicit i_config_var(std::string_view name) : m_name(name) {}

		virtual const std::string& get_name() const = 0;
		virtual const std::string& get_type_name() const = 0;

		virtual nlohmann::json serialize() const = 0;
		virtual bool deserialize(const nlohmann::json& j) = 0;

		// legacy std::any accessors
		virtual std::any get_value() const = 0;
		virtual void set_value(std::any v) = 0;

		virtual void reset() = 0;

	protected:
		std::string m_name;
	};

	template <typename T>
	class config_var : public i_config_var
	{
	public:
		config_var(std::string_view name, T def) :
			i_config_var(name),
			m_value(def),
			m_default(std::move(def))
		{
			register_var(this);
		}

		~config_var() override { unregister_var(this); }

		const std::string& get_name() const override { return m_name; }
		const std::string& get_type_name() const override { return type_name<T>(); }

		T& get() { return m_value; }
		const T& get() const { return m_value; }
		const T& get_default() const { return m_default; }

		void set(const T& value) { m_value = value; }
		void reset() override { m_value = m_default; }

		nlohmann::json serialize() const override { return config_codec<T>::to_json(m_value); }
		bool deserialize(const nlohmann::json& j) override { return config_codec<T>::from_json(j, m_value); }

		std::any get_value() const override { return m_value; }
		void set_value(std::any v) override
		{
			if (v.type() == typeid(T))
				m_value = std::any_cast<T>(v);
		}

	private:
		T m_value;
		T m_default;
	};

	// -----------------------------------------------------------------------
	// CREATE_CVAR(type, name, default) - define + register a config variable.
	// use at namespace / global scope. variadic so defaults may contain commas:
	//	CREATE_CVAR(bool,     "menu.open",   false);
	//	CREATE_CVAR(col_rgba, "esp.color",   col_rgba{ 255, 255, 255, 255 });
	//	CREATE_CVAR(col_rgb,  "esp.outline", col_rgb(255, 0, 0) /* ColorRGB(...) */);
	// -----------------------------------------------------------------------
#ifndef CREATE_CVAR
#define ZARTO_DETAIL_CONCAT_IMPL(a, b) a##b
#define ZARTO_DETAIL_CONCAT(a, b) ZARTO_DETAIL_CONCAT_IMPL(a, b)

#define CREATE_CVAR(type, name, ...) \
	[[maybe_unused]] static ::config::config_var<type> ZARTO_DETAIL_CONCAT(zarto_config_var_, __COUNTER__)(name, __VA_ARGS__)
#endif

	// -----------------------------------------------------------------------
	// typed lookup helpers
	// -----------------------------------------------------------------------
	template <typename T>
	inline config_var<T>* find_cvar(const std::string& cvarname)
	{
		return dynamic_cast<config_var<T>*>(find_var(cvarname));
	}

	template <typename T>
	inline T& get_var(const std::string& cvarname)
	{
		i_config_var* raw = find_var(cvarname);
		if (!raw)
			throw std::out_of_range("config var not found: " + cvarname);

		config_var<T>* var = dynamic_cast<config_var<T>*>(raw);
		if (!var)
			throw std::runtime_error("config var \"" + cvarname + "\" has type " + raw->get_type_name() +
				", requested " + type_name<T>());

		return var->get();
	}

	// -----------------------------------------------------------------------
	// (de)serialization
	// -----------------------------------------------------------------------
	nlohmann::json serialize_vars();                          // registry -> nested "vars" object
	size_t apply_vars(const nlohmann::json& vars);            // nested "vars" object -> registry, returns #applied
	config snapshot(const std::string& name);                 // registry -> config document
	bool apply(const config& c);                              // config document -> registry (+ loaded_config)

	// -----------------------------------------------------------------------
	// files
	// -----------------------------------------------------------------------
	std::filesystem::path get_config_directory();             // creates the directory if needed
	std::filesystem::path get_config_path(const std::string& name);

	std::vector<std::string> list_configs();                  // config names found on disk
	bool config_exists(const std::string& name);

	bool save_config(const std::string& name);                // registry -> file
	bool load_config(const std::string& name);                // file -> registry (auto-upgrades if needed)
	bool delete_config(const std::string& name);

	// validates + auto-upgrades every config in the directory and caches them.
	// returns the names of the configs that were loaded successfully.
	std::vector<std::string> load_all_configs();

	const std::map<std::string, config>& get_loaded_configs();

	// -----------------------------------------------------------------------
	// versioning
	// -----------------------------------------------------------------------
	// a migrator upgrades a document from `from_version` to `from_version + 1`
	using config_migrator = std::function<void(nlohmann::json&)>;

	void register_migrator(int from_version, config_migrator migrator);
	int  get_config_version(const nlohmann::json& doc);       // -1 when missing/invalid
	bool upgrade_config(nlohmann::json& doc);                 // in-place upgrade to CURRENT_CFG_SYSTEM_VERSION

	// -----------------------------------------------------------------------
	// last error message (for whichever call returned false)
	// -----------------------------------------------------------------------
	const std::string& last_error();
}
