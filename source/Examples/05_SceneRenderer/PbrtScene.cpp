#include <Resources/BowResources.h>
#include <CoreSystems/BowMath.h>
#include <Platform/BowFileReader.h>
#include "PbrtScene.h"

#include <sstream>
#include <iostream>
#include <algorithm> 
#include <cctype>
#include <locale>

struct Argument
{
	std::string key;
	std::string values;
};

// trim from start (in place)
static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

// trim from both ends (in place)
static inline std::vector<std::pair<std::string, std::string>> tokenize(const std::string& parameterString) 
{
	char waitForEnding = '\0';
	std::vector<std::pair<std::string, std::string>> tokens;
	for(unsigned int currentIndex = 0; currentIndex < parameterString.length(); currentIndex++)
	{
		char c = parameterString[currentIndex];
		if (waitForEnding == '\0')
		{
			if (c == '\"')
			{
				waitForEnding = '\"';
				tokens.push_back(std::pair<std::string, std::string>());
			}
			else if (c == '[')
			{
				waitForEnding = ']';
			}
			continue;
		}

		if (c != waitForEnding)
		{
			if (waitForEnding == '\"')
			{
				tokens.back().first.push_back(c);
			}
			else if(waitForEnding == ']')
			{
				tokens.back().second.push_back(c);
			}
		}
		else
		{
			waitForEnding = '\0';
		}
	}
	return tokens;
}

// trim from both ends (in place)
static inline std::vector<std::string> split(const std::string& line)
{
	std::vector<std::string> tokens;
	tokens.clear();

	auto begin = line.begin();
	const auto end = line.end();
	auto eot = begin;
	while (eot != end)
	{
		// Skip all delimiters.
		while (begin != end && *begin == ' ')
		{
			++begin;
		}
		eot = std::find(begin, end, ' ');

		tokens.emplace_back(begin, eot);
		if (eot != end)
		{
			// Move begin after delimiter.
			begin = eot + 1;
		}
	}

	return tokens;
}

std::string parseString(std::string argument)
{
	std::string result;
	char waitForEnding = '\0';
	for (unsigned int currentIndex = 0; currentIndex < argument.length(); currentIndex++)
	{
		char c = argument[currentIndex];
		if (waitForEnding == '\0')
		{
			if (c == '\"')
			{
				waitForEnding = '\"';
			}
			continue;
		}

		if (c != waitForEnding)
		{
			if (waitForEnding == '\"')
			{
				result.push_back(c);
			}
		}
		else
		{
			waitForEnding = '\0';
			return result;
		}
	}
	return "";
}

bow::Vector3<float> parseVec3(std::string argument)
{
	std::vector<std::string> values = split(argument);
	return bow::Vector3<float>(std::stod(values[0]), std::stod(values[1]), std::stod(values[2]));
}

float parseFloat(std::string argument)
{
	return (float)std::stod(argument);
}

float parseBool(std::string argument)
{
	std::string value = parseString(argument);
	return value == "true";
}


// Konstruktor: Default Werte setzen
PbrtScene::PbrtScene()
{

}

PbrtScene::~PbrtScene()
{

}


bool PbrtScene::parseFile(std::string filePath)
{
	bow::FileReader reader;

	char*	dataFromDisk; 
	size_t	sizeInBytes;
	if (reader.Open(filePath.c_str()))
	{
		sizeInBytes = reader.GetSizeOfFile();
		dataFromDisk = new char[sizeInBytes];
		dataFromDisk[sizeInBytes - 1] = '\0';

		unsigned int readedBytes = 0;
		char buffer[1024];

		reader.Seek(0);
		for (size_t i = 0; !reader.EndOfFile(); i += readedBytes)
		{
			readedBytes = (unsigned int)reader.Read(buffer, 1024);
			memcpy(dataFromDisk + i, buffer, readedBytes);
		}

		reader.Close();

		std::string path = filePath.substr(0, filePath.find_last_of('/') + 1);

		parse(path, dataFromDisk);

		delete[] dataFromDisk;
		return true;
	}
	else
	{
		LOG_ERROR("Could not open File '%s'!", filePath.c_str());
		return false;
	}
}

void PbrtScene::parse(const std::string& filepath, char* dataFromDisk)
{
	std::string line;
	std::istringstream dataStream(dataFromDisk);

	std::vector<std::pair<std::string, std::string>> tokens;
	while (safeGetline(dataStream, line))
	{
		auto begin = line.begin();
		const auto end = line.end();
		auto eot = begin;

		// Skip all delimiters.
		while (begin != end && *begin == ' ')
		{
			++begin;
		}
		eot = std::find(begin, end, ' ');

		std::string token = std::string(begin, eot);
		trim(token);

		if (token.size() > 0)
		{
			if (eot != end)
			{
				tokens.emplace_back(std::pair<std::string, std::string>(token, std::string(eot + 1, end)));
			}
			else
			{
				tokens.emplace_back(std::pair<std::string, std::string>(token, ""));
			}
		}
	}

	for (unsigned int lineIndex = 0; lineIndex < tokens.size(); lineIndex++) 
	{
		std::pair<std::string, std::string> token_params_pair = tokens[lineIndex];

		std::string token = token_params_pair.first;
		std::string parameters = token_params_pair.second;

		if (token.empty())
			continue;

		switch (token[0]) {
		case 'A':
			if (token == "AttributeBegin")
			{
				// do stuff here
			}
			else if (token == "AttributeEnd")
			{
				// do stuff here
			}
			else if (token == "ActiveTransform")
			{
				// do stuff here
			}
			else if (token == "AreaLightSource")
			{
				// do stuff here
			}
			else if (token == "Accelerator")
			{
				// do stuff here
			}
			else
			{
				// do stuff here
			}
			break;

		case 'C':
			if (token == "ConcatTransform")
			{
				// do stuff here
			}
			else if (token == "CoordinateSystem")
			{
				// do stuff here
			}
			else if (token == "CoordSysTransform")
			{
				// do stuff here
			}
			else if (token == "Camera")
			{
				// do stuff here
			}
			else
			{
				// do stuff here
			}
			break;

		case 'F':
			if (token == "Film")
			{
				// do stuff here
			}
			else
			{
				// do stuff here
			}
			break;

		case 'I':
			if (token == "Integrator")
			{
				// do stuff here
			}
			else if (token == "Include")
			{
				// do stuff here
			}
			else if (token == "Identity")
			{
				// do stuff here
			}
			else
			{
				// do stuff here
			}
			break;

		case 'L':
			if (token == "LightSource")
			{
				// do stuff here
			}
			else if (token == "LookAt")
			{
				// do stuff here
			}
			else
			{
				// do stuff here
			}
			break;

		case 'M':
			if (token == "MakeNamedMaterial")
			{
				loadMaterial(filepath, parameters);
			}
			else if (token == "MakeNamedMedium")
			{
				// do stuff here
			}
			else if (token == "Material")
			{
				// do stuff here
			}
			else if (token == "MediumInterface")
			{
				// do stuff here
			}
			else
			{
				// do stuff here
			}
			break;

		case 'N':
			if (token == "NamedMaterial")
			{
				std::vector<std::pair<std::string, std::string>> tokens = tokenize(parameters);
				m_currentMaterialName = tokens[0].first;
			}
			else
			{
				// do stuff here
			}
			break;

		case 'O':
			if (token == "ObjectBegin")
			{
				// do stuff here
			}
			else if (token == "ObjectEnd")
			{
				// do stuff here
			}
			else if (token == "ObjectInstance")
			{
				// do stuff here
			}
			else
			{
				// do stuff here
			}
			break;

		case 'P':
			if (token == "PixelFilter")
			{
				// do stuff here
			}
			else
			{
				// do stuff here
			}
			break;

		case 'R':
			if (token == "ReverseOrientation")
			{
				// do stuff here
			}
			else if (token == "Rotate")
			{
				// do stuff here
			}
			else
			{
				// do stuff here
			}
			break;

		case 'S':
			if (token == "Shape")
			{
				loadShape(filepath, parameters);
			}
			else if (token == "Sampler")
			{
				// do stuff here
			}
			else if (token == "Scale")
			{
				// do stuff here
			}
			else
			{
				// do stuff here
			}
			break;

		case 'T':
			if (token == "TransformBegin")
			{
				// do stuff here
			}
			else if (token == "TransformEnd")
			{
				// do stuff here
			}
			else if (token == "Transform")
			{
				// do stuff here
			}
			else if (token == "Translate")
			{
				// do stuff here
			}
			else if (token == "TransformTimes")
			{
				// do stuff here
			}
			else if (token == "Texture")
			{
				loadTexture(filepath, parameters);
			}
			else
			{
				// do stuff here
			}
			break;

		case 'W':
			if (token == "WorldBegin")
			{
				// do stuff here
			}
			else if (token == "WorldEnd")
			{
				// do stuff here
			}
			else
			{
				// do stuff here
			}
			break;

		default:
			{
				// do stuff here
			}
		}
	}
}

void PbrtScene::loadShape(const std::string& filepath, const std::string& parameters)
{
	std::vector<std::pair<std::string, std::string>> tokens = tokenize(parameters);
	if (tokens[0].first == "plymesh")
	{
		for (unsigned int i = 0; i < tokens.size(); i++)
		{
			if (tokens[i].first == "string filename")
			{
				std::string filename = parseString(tokens[i].second);
				bow::MeshPtr mesh = bow::MeshManager::GetInstance().Load(filepath + filename);
				AddToScene(mesh);
			}
		}
	}
	else if (tokens[0].first == "trianglemesh")
	{

	}
}

void PbrtScene::loadTexture(const std::string & filepath, const std::string & parameters)
{
	std::vector<std::pair<std::string, std::string>> tokens = tokenize(parameters);
	for (unsigned int i = 0; i < tokens.size(); i++)
	{
		if (tokens[i].first == "string filename")
		{
			std::string filename = parseString(tokens[i].second);
			bow::ImagePtr image = bow::ImageManager::GetInstance().Load(filepath + filename);
			AddToScene(tokens[0].first, image);
		}
	}
}

void PbrtScene::loadMaterial(const std::string & filepath, const std::string & parameters)
{
	std::vector<std::pair<std::string, std::string>> tokens = tokenize(parameters);

	PbrtMaterial material;
	material.name = tokens[0].first;
	for (unsigned int i = 0; i < tokens.size(); i++)
	{
		if (tokens[i].first == "string type")
		{
			material.type = parseString(tokens[i].second);
		} 
		else if (tokens[i].first == "rgb Kd")
		{
			material.Kd = parseVec3(tokens[i].second);
		}
		else if (tokens[i].first == "rgb Ks")
		{
			material.Ks = parseVec3(tokens[i].second);
		}
		else if (tokens[i].first == "rgb Kt")
		{
			material.Kt = parseVec3(tokens[i].second);
		}
		else if (tokens[i].first == "rgb k")
		{
			material.k = parseVec3(tokens[i].second);
		}
		else if (tokens[i].first == "rgb eta")
		{
			material.eta = parseVec3(tokens[i].second);
		}
		else if (tokens[i].first == "rgb opacity")
		{
			material.opacity = parseVec3(tokens[i].second);
		}
		else if (tokens[i].first == "float index")
		{
			material.index = parseFloat(tokens[i].second);
		}
		else if (tokens[i].first == "bool remaproughness")
		{
			material.remaproughness = parseBool(tokens[i].second);
		}
		else if (tokens[i].first == "float uroughness")
		{
			material.uroughness = parseFloat(tokens[i].second);
		}
		else if (tokens[i].first == "float vroughness")
		{
			material.vroughness = parseFloat(tokens[i].second);
		}
		else if (tokens[i].first == "float sigma")
		{
			material.sigma = parseFloat(tokens[i].second);
		}
		else if (tokens[i].first == "texture Kd")
		{
			material.tex_Kd = parseString(tokens[i].second);
		}
		else if (tokens[i].first == "texture bumpmap")
		{
			material.tex_bumpmap = parseString(tokens[i].second);
		}
		else if (tokens[i].first == "texture opacity")
		{
			material.tex_opacity = parseString(tokens[i].second);
		}
		else
		{
			if (i != 0)
			{
				LOG_FATAL("%s not handled", tokens[i].first);
			}
		}
	}

	AddToScene(tokens[0].first, material);
}

void PbrtScene::AddToScene(bow::MeshPtr mesh)
{
	m_meshes.push_back(mesh);
	std::vector<bow::SubMesh*> subMeshes = mesh->GetSubMeshes();
	for (unsigned int i = 0; i < subMeshes.size(); i++)
	{
		subMeshes[i]->SetMaterialName(m_currentMaterialName);
	}
	m_subMeshes.insert(m_subMeshes.end(), subMeshes.begin(), subMeshes.end());
}

void PbrtScene::AddToScene(const std::string& name, bow::ImagePtr image)
{
	m_textures.insert(std::pair<std::string, bow::ImagePtr>(name, image));
}


void PbrtScene::AddToScene(const std::string& name, PbrtMaterial& material)
{

}

// See
// http://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
std::istream& PbrtScene::safeGetline(std::istream &is, std::string &t)
{
	t.clear();

	// The characters in the stream are read one-by-one using a std::streambuf.
	// That is faster than reading them one-by-one using the std::istream.
	// Code that uses streambuf this way must be guarded by a sentry object.
	// The sentry object performs various tasks,
	// such as thread synchronization and updating the stream state.

	std::istream::sentry se(is, true);
	std::streambuf *sb = is.rdbuf();

	if (se)
	{
		for (;;)
		{
			int c = sb->sbumpc();
			switch (c)
			{
			case '\n':
				return is;
			case '\r':
				if (sb->sgetc() == '\n') sb->sbumpc();
				return is;
			case EOF:
				// Also handle the case when the last line has no line ending
				if (t.empty()) is.setstate(std::ios::eofbit);
				return is;
			default:
				t += static_cast<char>(c);
			}
		}
	}

	return is;
}