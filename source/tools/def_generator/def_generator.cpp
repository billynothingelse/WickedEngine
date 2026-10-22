// Generates a module definition (.def) file that exports every external symbol defined in a set of
// COFF object files or static libraries. Used to build the engine as a DLL on Windows without having
// to annotate the code with __declspec(dllexport), similar to CMake's WINDOWS_EXPORT_ALL_SYMBOLS.
//
//	def_generator <output.def> <library name> <input>...
//
// An input can be a .obj file, a .lib archive or a directory (all .obj files inside it are used).
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace Protean::DefGenerator
{
	using u8 = uint8_t;
	using u16 = uint16_t;
	using u32 = uint32_t;

	static constexpr u16 IMAGE_FILE_MACHINE_AMD64 = 0x8664;
	static constexpr u16 IMAGE_FILE_MACHINE_ARM64 = 0xAA64;
	static constexpr u8 IMAGE_SYM_CLASS_EXTERNAL = 2;
	static constexpr u16 IMAGE_SYM_DTYPE_FUNCTION = 2;

	// GUID of ANON_OBJECT_HEADER_BIGOBJ (/bigobj object files)
	static constexpr u8 BIGOBJ_CLASS_ID[16] = { 0xC7, 0xA1, 0xBA, 0xD1, 0xEE, 0xBA, 0xA9, 0x4B, 0xAF, 0x20, 0xFA, 0xF6, 0x6A, 0xA4, 0xDC, 0xB8 };

	// Exported symbol name -> is data
	using SymbolMap = std::map<std::string, bool>;

	template<typename T>
	static bool Read(const std::vector<u8>& data, size_t offset, T& value)
	{
		if (offset + sizeof(T) > data.size())
			return false;
		std::memcpy(&value, data.data() + offset, sizeof(T));
		return true;
	}

	static bool StartsWith(const std::string& value, const char* prefix)
	{
		return value.compare(0, std::strlen(prefix), prefix) == 0;
	}

	// Symbols that must not (or can not) be exported
	static bool IsExportable(const std::string& name, const std::string& sectionName)
	{
		if (name.empty())
			return false;
		static const char* skipPrefixes[] = {
			"__real@", "__xmm@", "__ymm@", "__zmm@", "__mask@", // floating point / SIMD constants
			"__imp_", // import thunks
			"??_C@", // string literals
			"??_G", "??_E", // scalar / vector deleting destructors
			"_RTC_", "__RTC_", "_TI", "_CTA", "_CT??", "__CT", "__TI", // runtime checks, exception info
			"__guard", "__security", "_load_config_used", "__chkstk",
			".", "$",
		};
		for (const char* prefix : skipPrefixes)
		{
			if (StartsWith(name, prefix))
				return false;
		}
		if (StartsWith(sectionName, ".tls")) // thread locals can not be imported
			return false;
		return true;
	}

	static void ParseObject(const std::vector<u8>& data, SymbolMap& symbols, const std::string& source)
	{
		u16 sig1 = 0, sig2 = 0;
		if (!Read(data, 0, sig1) || !Read(data, 2, sig2))
			return;

		bool bigobj = false;
		u16 machine = 0;
		u32 sectionCount = 0;
		u32 symbolTableOffset = 0;
		u32 symbolCount = 0;
		size_t sectionTableOffset = 0;

		if (sig1 == 0 && sig2 == 0xFFFF)
		{
			// Import object or bigobj
			u16 version = 0;
			Read(data, 4, version);
			if (version < 2 || data.size() < 56 || std::memcmp(data.data() + 12, BIGOBJ_CLASS_ID, 16) != 0)
				return; // short import object, nothing to export
			bigobj = true;
			Read(data, 6, machine);
			Read(data, 44, sectionCount);
			Read(data, 48, symbolTableOffset);
			Read(data, 52, symbolCount);
			sectionTableOffset = 56;
		}
		else
		{
			machine = sig1;
			u16 sections16 = 0, optionalHeaderSize = 0;
			Read(data, 2, sections16);
			Read(data, 8, symbolTableOffset);
			Read(data, 12, symbolCount);
			Read(data, 16, optionalHeaderSize);
			sectionCount = sections16;
			sectionTableOffset = 20 + optionalHeaderSize;
		}

		if (machine != IMAGE_FILE_MACHINE_AMD64 && machine != IMAGE_FILE_MACHINE_ARM64)
		{
			std::fprintf(stderr, "def_generator: skipping %s (not an x64/ARM64 COFF object, LTCG objects are not supported)\n", source.c_str());
			return;
		}

		const size_t symbolSize = bigobj ? 20 : 18;
		const size_t stringTableOffset = symbolTableOffset + size_t(symbolCount) * symbolSize;

		auto SectionName = [&](int32_t section) -> std::string {
			if (section <= 0 || u32(section) > sectionCount)
				return {};
			size_t offset = sectionTableOffset + size_t(section - 1) * 40;
			if (offset + 8 > data.size())
				return {};
			return std::string((const char*)data.data() + offset, strnlen((const char*)data.data() + offset, 8));
		};

		for (u32 i = 0; i < symbolCount; ++i)
		{
			size_t offset = symbolTableOffset + size_t(i) * symbolSize;
			if (offset + symbolSize > data.size())
				break;

			int32_t section = 0;
			u16 type = 0;
			u8 storageClass = 0;
			u8 auxCount = 0;
			// Symbol record: Name[8], Value(4), SectionNumber(2, bigobj: 4), Type(2), StorageClass(1), NumberOfAuxSymbols(1)
			if (bigobj)
			{
				Read(data, offset + 12, section);
				Read(data, offset + 16, type);
				Read(data, offset + 18, storageClass);
				Read(data, offset + 19, auxCount);
			}
			else
			{
				int16_t section16 = 0;
				Read(data, offset + 12, section16);
				section = section16;
				Read(data, offset + 14, type);
				Read(data, offset + 16, storageClass);
				Read(data, offset + 17, auxCount);
			}

			if (storageClass == IMAGE_SYM_CLASS_EXTERNAL && section > 0)
			{
				std::string name;
				u32 zeroes = 0;
				Read(data, offset, zeroes);
				if (zeroes == 0)
				{
					u32 stringOffset = 0;
					Read(data, offset + 4, stringOffset);
					size_t at = stringTableOffset + stringOffset;
					if (at < data.size())
						name = (const char*)data.data() + at;
				}
				else
				{
					name = std::string((const char*)data.data() + offset, strnlen((const char*)data.data() + offset, 8));
				}

				if (IsExportable(name, SectionName(section)))
				{
					const bool isFunction = ((type >> 4) & 0x3) == IMAGE_SYM_DTYPE_FUNCTION;
					auto it = symbols.find(name);
					if (it == symbols.end())
						symbols[name] = !isFunction;
					else if (isFunction)
						it->second = false;
				}
			}

			i += auxCount;
		}
	}

	static void ParseArchive(const std::vector<u8>& data, SymbolMap& symbols, const std::string& source)
	{
		size_t offset = 8; // "!<arch>\n"
		while (offset + 60 <= data.size())
		{
			const char* header = (const char*)data.data() + offset;
			std::string name(header, 16);
			std::string sizeText(header + 48, 10);
			size_t size = std::strtoull(sizeText.c_str(), nullptr, 10);
			size_t memberOffset = offset + 60;
			if (memberOffset + size > data.size())
				break;

			// "/" linker members and "//" long names member contain no objects
			if (name[0] != '/' || (name[1] >= '0' && name[1] <= '9'))
			{
				std::vector<u8> member(data.begin() + memberOffset, data.begin() + memberOffset + size);
				ParseObject(member, symbols, source);
			}

			offset = memberOffset + size;
			offset += offset & 1; // members are 2 byte aligned
		}
	}

	static bool ParseFile(const std::filesystem::path& path, SymbolMap& symbols)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			std::fprintf(stderr, "def_generator: can not open %s\n", path.string().c_str());
			return false;
		}
		std::vector<u8> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		if (data.size() >= 8 && std::memcmp(data.data(), "!<arch>\n", 8) == 0)
			ParseArchive(data, symbols, path.string());
		else
			ParseObject(data, symbols, path.string());
		return true;
	}

	static int Run(int argc, char** argv)
	{
		if (argc < 4)
		{
			std::fprintf(stderr, "usage: def_generator <output.def> <library name> <input.obj|input.lib|directory>...\n");
			return 1;
		}

		SymbolMap symbols;
		for (int i = 3; i < argc; ++i)
		{
			std::filesystem::path input = argv[i];
			if (std::filesystem::is_directory(input))
			{
				for (const auto& entry : std::filesystem::directory_iterator(input))
				{
					if (entry.is_regular_file() && entry.path().extension() == ".obj")
					{
						if (!ParseFile(entry.path(), symbols))
							return 1;
					}
				}
			}
			else if (!ParseFile(input, symbols))
			{
				return 1;
			}
		}

		if (symbols.size() > 65535)
		{
			std::fprintf(stderr, "def_generator: %zu symbols exceed the 65535 export limit of a DLL\n", symbols.size());
			return 1;
		}

		std::string text = "LIBRARY " + std::string(argv[2]) + "\nEXPORTS\n";
		for (const auto& [name, isData] : symbols)
		{
			text += "\t" + name + (isData ? " DATA\n" : "\n");
		}

		// Only rewrite when changed, so the linker does not relink needlessly
		std::ifstream existing(argv[1], std::ios::binary);
		std::string previous((std::istreambuf_iterator<char>(existing)), std::istreambuf_iterator<char>());
		existing.close();
		if (previous != text)
		{
			std::ofstream out(argv[1], std::ios::binary);
			if (!out)
			{
				std::fprintf(stderr, "def_generator: can not write %s\n", argv[1]);
				return 1;
			}
			out << text;
		}
		std::printf("def_generator: %zu symbols exported to %s\n", symbols.size(), argv[1]);
		return 0;
	}
}

int main(int argc, char** argv)
{
	return Protean::DefGenerator::Run(argc, argv);
}
