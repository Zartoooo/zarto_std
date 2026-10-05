#include "config.h"

#include <algorithm>
#include <fstream>
#include <system_error>

namespace config
{
	namespace
	{
		// -------------------------------------------------------------------
		// module state
		// -------------------------------------------------------------------
		cvar_registry& registry_storage()
		{
			static cvar_registry registry;
			return registry;
		}

		std::map<int, config_migrator>& migrator_storage()
		{
			static std::map<int, config_migrator> migrators;
			return migrators;
		}

		std::map<std::string, config>& cache_storage()
		{
			static std::map<std::string, config> cache;
			return cache;
		}

		std::string& error_storage()
		{
			static std::string error;
			return error;
		}

		bool set_error(const std::string& message)
		{
			error_storage() = message;
			return false;
		}

		// -------------------------------------------------------------------
		// "a.b.c" <-> nested json helpers
		// -------------------------------------------------------------------
		std::vector<std::string> split_name(const std::string& name)
		{
			std::vector<std::string> segments;
			std::string current;
			for (char c : name)
			{
				if (c == '.')
				{
					if (!current.empty()) segments.push_back(current);
					current.clear();
				}
				else
				{
					current.push_back(c);
				}
			}
			if (!current.empty()) segments.push_back(current);
			return segments;
		}

		std::string join_segments(const std::vector<std::string>& segments, size_t from, size_t to)
		{
			std::string joined;
			for (size_t i = from; i < to; ++i)
			{
				if (!joined.empty()) joined.push_back('.');
				joined += segments[i];
			}
			return joined;
		}

		// Resolves the json slot a cvar name maps to, creating intermediate
		// objects when `create` is set. The full dotted name always wins as a
		// single flat key, so "visuals.playeresp.color" stays flat, while
		// "aimbot.smoothing.x" nests when no "aimbot.smoothing" cvar exists.
		nlohmann::json* resolve_slot(nlohmann::json& root, const std::string& name, bool create)
		{
			const std::vector<std::string> segments = split_name(name);
			if (segments.empty()) return nullptr;

			nlohmann::json* current = &root;
			for (size_t i = 0; i < segments.size(); ++i)
			{
				const bool last = (i + 1 == segments.size());
				const bool is_var = (find_var(join_segments(segments, 0, i + 1)) != nullptr);

				if (last || is_var)
				{
					// the remainder (possibly dotted) is one flat key
					const std::string key = join_segments(segments, i, segments.size());
					if (create)
					{
						if (!current->is_object()) *current = nlohmann::json::object();
						return &(*current)[key];
					}
					auto it = current->find(key);
					return (it == current->end()) ? nullptr : &(*it);
				}

				if (create)
				{
					nlohmann::json& child = (*current)[segments[i]];
					if (child.is_null())
						child = nlohmann::json::object();
					else if (!child.is_object())
						return &(*current)[join_segments(segments, i, segments.size())]; // can't nest - keep it flat

					current = &child;
				}
				else
				{
					auto it = current->find(segments[i]);
					if (it != current->end() && it->is_object())
					{
						current = &(*it);
						continue;
					}

					// fall back to a flat dotted key at this level
					auto flat = current->find(join_segments(segments, i, segments.size()));
					return (flat == current->end()) ? nullptr : &(*flat);
				}
			}
			return nullptr;
		}

		// -------------------------------------------------------------------
		// files
		// -------------------------------------------------------------------
		bool is_valid_config_name(const std::string& name)
		{
			if (name.empty() || name == "." || name == "..") return false;
			for (char c : name)
				if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
					c == '"' || c == '<' || c == '>' || c == '|')
					return false;
			return true;
		}

		std::string lowercase_extension(const std::filesystem::path& path)
		{
			std::string extension = path.extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return extension;
		}

		bool read_document(const std::filesystem::path& path, nlohmann::json& out, std::string& error)
		{
			std::ifstream in(path, std::ios::binary);
			if (!in)
			{
				error = "could not open config file: " + path.string();
				return false;
			}

			out = nlohmann::json::parse(in, nullptr, false);
			if (out.is_discarded())
			{
				error = "could not parse config file: " + path.string();
				return false;
			}
			if (!out.is_object())
			{
				error = "config file is not a json object: " + path.string();
				return false;
			}
			return true;
		}

		template <typename Json>
		bool write_document(const std::filesystem::path& path, const Json& doc)
		{
			std::ofstream out(path, std::ios::binary | std::ios::trunc);
			if (!out) return false;
			out << doc.dump(4);
			return static_cast<bool>(out);
		}

		bool read_vars(const nlohmann::json& doc, nlohmann::json& vars)
		{
			auto it = doc.find("vars");
			if (it == doc.end() || !it->is_object()) return false;
			vars = *it;
			return true;
		}
	}

	// -----------------------------------------------------------------------
	// registry
	// -----------------------------------------------------------------------
	void register_var(i_config_var* var)
	{
		if (!var || var->get_name().empty()) return;
		registry_storage().emplace(var->get_name(), var); // first registration wins
	}

	void unregister_var(i_config_var* var)
	{
		if (!var) return;

		auto it = registry_storage().find(var->get_name());
		if (it != registry_storage().end() && it->second == var)
			registry_storage().erase(it);
	}

	i_config_var* find_var(const std::string& cvarname)
	{
		auto it = registry_storage().find(cvarname);
		return (it == registry_storage().end()) ? nullptr : it->second;
	}

	const cvar_registry& get_registry()
	{
		return registry_storage();
	}

	void reset_to_defaults()
	{
		for (auto& [name, var] : registry_storage())
			if (var) var->reset();
	}

	// -----------------------------------------------------------------------
	// (de)serialization
	// -----------------------------------------------------------------------
	nlohmann::json serialize_vars()
	{
		nlohmann::json vars = nlohmann::json::object();
		for (const auto& [name, var] : registry_storage())
		{
			if (!var) continue;
			if (nlohmann::json* slot = resolve_slot(vars, name, true))
				*slot = var->serialize();
		}
		return vars;
	}

	size_t apply_vars(const nlohmann::json& vars)
	{
		if (!vars.is_object()) return 0;

		nlohmann::json tree = vars; // resolve_slot needs a non-const tree
		size_t applied = 0;
		for (const auto& [name, var] : registry_storage())
		{
			if (!var) continue;
			if (nlohmann::json* slot = resolve_slot(tree, name, false))
				if (var->deserialize(*slot)) ++applied;
		}
		return applied;
	}

	config snapshot(const std::string& name)
	{
		config c;
		c.name = name;
		c.version = CURRENT_CFG_SYSTEM_VERSION;
		c.vars = serialize_vars();
		return c;
	}

	bool apply(const config& c)
	{
		apply_vars(c.vars);
		loaded_config = std::make_unique<config>(c);
		return true;
	}

	// -----------------------------------------------------------------------
	// versioning
	// -----------------------------------------------------------------------
	void register_migrator(int from_version, config_migrator migrator)
	{
		migrator_storage()[from_version] = std::move(migrator);
	}

	int get_config_version(const nlohmann::json& doc)
	{
		if (!doc.is_object()) return -1;

		auto it = doc.find("version");
		if (it == doc.end() || !it->is_number()) return -1;
		return it->get<int>();
	}

	bool upgrade_config(nlohmann::json& doc)
	{
		if (!doc.is_object())
			return set_error("config is not a json object");

		int version = get_config_version(doc);
		if (version < 0)
			return set_error("config is missing a valid \"version\" field");
		if (version > CURRENT_CFG_SYSTEM_VERSION)
			return set_error("config version " + std::to_string(version) +
				" is newer than supported version " + std::to_string(CURRENT_CFG_SYSTEM_VERSION));

		while (version < CURRENT_CFG_SYSTEM_VERSION)
		{
			auto it = migrator_storage().find(version);
			if (it == migrator_storage().end() || !it->second)
				return set_error("no migrator registered from config version " + std::to_string(version));

			try
			{
				it->second(doc);
			}
			catch (const std::exception& e)
			{
				return set_error(std::string("config migrator failed: ") + e.what());
			}

			++version;
			doc["version"] = version;
		}
		return true;
	}

	// -----------------------------------------------------------------------
	// files
	// -----------------------------------------------------------------------
	std::filesystem::path get_config_directory()
	{
		std::filesystem::path directory(CONFIG_DIRECTORY);

		std::error_code ec;
		if (!std::filesystem::exists(directory, ec))
			std::filesystem::create_directories(directory, ec);

		return directory;
	}

	std::filesystem::path get_config_path(const std::string& name)
	{
		return get_config_directory() / (name + CONFIG_FILE_EXTENSION);
	}

	std::vector<std::string> list_configs()
	{
		std::vector<std::string> names;

		std::error_code ec;
		for (const auto& entry : std::filesystem::directory_iterator(get_config_directory(), ec))
		{
			if (ec) break;
			if (!entry.is_regular_file()) continue;
			if (lowercase_extension(entry.path()) != CONFIG_FILE_EXTENSION) continue;

			names.push_back(entry.path().stem().string());
		}

		std::sort(names.begin(), names.end());
		return names;
	}

	bool config_exists(const std::string& name)
	{
		if (!is_valid_config_name(name)) return false;

		std::error_code ec;
		return std::filesystem::exists(get_config_path(name), ec);
	}

	bool save_config(const std::string& name)
	{
		if (!is_valid_config_name(name))
			return set_error("invalid config name: " + name);

		// ordered_json keeps config-name / version / vars in this order
		nlohmann::ordered_json doc;
		doc["config-name"] = name;
		doc["version"] = CURRENT_CFG_SYSTEM_VERSION;
		doc["vars"] = serialize_vars();

		const std::filesystem::path path = get_config_path(name);
		if (!write_document(path, doc))
			return set_error("could not write config file: " + path.string());

		cache_storage()[name] = config{ name, CURRENT_CFG_SYSTEM_VERSION, doc["vars"] };
		if (loaded_config && loaded_config->name == name)
		{
			loaded_config->version = CURRENT_CFG_SYSTEM_VERSION;
			loaded_config->vars = doc["vars"];
		}
		return true;
	}

	bool load_config(const std::string& name)
	{
		if (!is_valid_config_name(name))
			return set_error("invalid config name: " + name);

		// already validated/cached (e.g. by load_all_configs)
		auto cached = cache_storage().find(name);
		if (cached != cache_storage().end())
			return apply(cached->second);

		nlohmann::json doc;
		std::string error;
		if (!read_document(get_config_path(name), doc, error))
			return set_error(error);

		const int version_before = get_config_version(doc);
		if (!upgrade_config(doc))
			return false;

		nlohmann::json vars;
		if (!read_vars(doc, vars))
			return set_error("config \"" + name + "\" has no \"vars\" object");

		if (version_before < CURRENT_CFG_SYSTEM_VERSION)
			write_document(get_config_path(name), doc); // persist the upgrade

		config c;
		c.name = name;
		c.version = get_config_version(doc);
		c.vars = std::move(vars);

		cache_storage()[name] = c;
		return apply(c);
	}

	bool delete_config(const std::string& name)
	{
		if (!is_valid_config_name(name))
			return set_error("invalid config name: " + name);

		const std::filesystem::path path = get_config_path(name);
		std::error_code ec;
		if (!std::filesystem::exists(path, ec))
			return set_error("config does not exist: " + name);

		std::filesystem::remove(path, ec);
		if (ec)
			return set_error("could not remove config: " + name);

		cache_storage().erase(name);
		if (loaded_config && loaded_config->name == name)
			loaded_config.reset();
		return true;
	}

	std::vector<std::string> load_all_configs()
	{
		cache_storage().clear();

		std::vector<std::string> loaded;

		std::error_code ec;
		for (const auto& entry : std::filesystem::directory_iterator(get_config_directory(), ec))
		{
			if (ec) break;
			if (!entry.is_regular_file()) continue;
			if (lowercase_extension(entry.path()) != CONFIG_FILE_EXTENSION) continue;

			const std::string name = entry.path().stem().string();

			nlohmann::json doc;
			std::string error;
			if (!read_document(entry.path(), doc, error)) continue; // skip malformed files

			const int version_before = get_config_version(doc);
			if (!upgrade_config(doc)) continue;                     // wrong / unupgradeable / newer -> skip

			nlohmann::json vars;
			if (!read_vars(doc, vars)) continue;

			if (version_before < CURRENT_CFG_SYSTEM_VERSION)
				write_document(entry.path(), doc);                  // persist the auto-upgrade

			config c;
			c.name = name;
			c.version = get_config_version(doc);
			c.vars = std::move(vars);

			cache_storage()[name] = std::move(c);
			loaded.push_back(name);
		}

		std::sort(loaded.begin(), loaded.end());
		return loaded;
	}

	const std::map<std::string, config>& get_loaded_configs()
	{
		return cache_storage();
	}

	const std::string& last_error()
	{
		return error_storage();
	}
}
