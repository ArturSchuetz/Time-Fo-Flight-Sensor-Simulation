#pragma once
#include <RenderDevice/BowRenderer.h>
#include <CoreSystems/BowCoreSystems.h>

#include <map>

struct PbrtMaterial
{
	std::string name;
	std::string type;

	bow::Vector3<float> Kd;
	bow::Vector3<float> Ks;
	bow::Vector3<float> Kt;
	bow::Vector3<float> k;
	bow::Vector3<float> eta;
	bow::Vector3<float> opacity;

	float index;
	bool remaproughness;
	float uroughness;
	float vroughness;
	float sigma;

	std::string tex_Kd;
	std::string tex_bumpmap;
	std::string tex_opacity;

	PbrtMaterial()
	{
		name = "";
		type = "";

		Kd = bow::Vector3<float>::Zero();
		Ks = bow::Vector3<float>::Zero();
		Kt = bow::Vector3<float>::Zero();
		k = bow::Vector3<float>::Zero();
		eta = bow::Vector3<float>::Zero();
		opacity = bow::Vector3<float>::Zero();

		index = 0.0f;
		remaproughness = false;
		uroughness = 0.0f;
		vroughness = 0.0f;
		sigma = 0.0f;

		tex_Kd = "";
		tex_bumpmap = "";
		tex_opacity = "";
	}
};

class PbrtScene
{
public:
	PbrtScene();
	~PbrtScene();

	bool parseFile(std::string filePath);

	std::vector<bow::SubMesh*>& GetSubMeshes() {
		return m_subMeshes;
	}

	std::vector<bow::MeshPtr>& GetMeshes() {
		return m_meshes;
	}
	
private:
	std::istream& safeGetline(std::istream &is, std::string &t);
	void parse(const std::string& filepath, char* dataFromDisk);

	void loadShape(const std::string& filepath, const std::string& parameters);
	void loadTexture(const std::string& filepath, const std::string& parameters);
	void loadMaterial(const std::string & filepath, const std::string & parameters);

	void AddToScene(bow::MeshPtr mesh);
	void AddToScene(const std::string& name, bow::ImagePtr image);
	void AddToScene(const std::string& name, PbrtMaterial& material);

	std::string								m_currentMaterialName;
	std::map<std::string, bow::ImagePtr>	m_textures;
	std::map<std::string, PbrtMaterial>		m_materials;
	std::vector<bow::SubMesh*>				m_subMeshes;
	std::vector<bow::MeshPtr>				m_meshes;
};