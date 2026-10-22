#pragma once

void ImportModel_OBJ(const std::string& fileName, Protean::Scene& scene);
void ImportModel_GLTF(const std::string& fileName, Protean::Scene& scene);
void ExportModel_GLTF(const std::string& filename, Protean::Scene& scene);
void ImportModel_FBX(const std::string& fileName, Protean::Scene& scene);
void ImportModel_PLY(const std::string& fileName, Protean::Scene& scene);
