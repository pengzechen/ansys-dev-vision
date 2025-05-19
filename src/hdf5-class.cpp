

#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <stdexcept>
#include <tinyxml2.h>
#include <hdf5.h>

// 简化表示 Geometry 数据
struct GeometryData {
    std::string hdf5_path; // e.g. "disk_2d.h5:/data0"
    int num_points = 0;
    int dim = 0;
};

// 简化表示 Topology 数据
struct TopologyData {
    std::string hdf5_path; // e.g. "disk_2d.h5:/data1"
    int num_elements = 0;
    int nodes_per_element = 0;
};

// 网格数据结构
struct MeshData {
    GeometryData geometry;
    TopologyData topology;
};

// XdmfMeshLoader 类
class XdmfMeshLoader {
public:
    bool Load(const std::string& xdmf_filename);

    const MeshData& GetMeshData() const { return mesh; }

    std::vector<double> ReadGeometryData() const;
    std::vector<uint64_t> ReadTopologyData() const;

private:
    MeshData mesh;

    static bool ParseDataItem(tinyxml2::XMLElement* dataItemElem, std::string& out_path, int& out_dim0, int& out_dim1);

    static std::pair<std::string, std::string> ParseHDF5Path(const std::string& full_path);

    static std::vector<double> ReadPoints(const std::string& filename, const std::string& dataset, int num_points, int dim);
    static std::vector<uint64_t> ReadIndices(const std::string& filename, const std::string& dataset, int num_elements, int nodes_per_element);
};

// 读取并解析 XDMF 文件
bool XdmfMeshLoader::Load(const std::string& xdmf_filename) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(xdmf_filename.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to load XML file\n";
        return false;
    }

    auto root = doc.RootElement();
    if (!root || std::string(root->Name()) != "Xdmf") {
        std::cerr << "Invalid XDMF root element\n";
        return false;
    }

    auto domain = root->FirstChildElement("Domain");
    auto grid = domain ? domain->FirstChildElement("Grid") : nullptr;
    if (!grid) {
        std::cerr << "Missing Domain or Grid\n";
        return false;
    }

    // Geometry
    auto geometry = grid->FirstChildElement("Geometry");
    if (!geometry) {
        std::cerr << "No Geometry element\n";
        return false;
    }
    auto geom_data_item = geometry->FirstChildElement("DataItem");
    std::string geom_path;
    int geom_d0 = 0, geom_d1 = 0;
    if (!ParseDataItem(geom_data_item, geom_path, geom_d0, geom_d1)) return false;
    mesh.geometry = {geom_path, geom_d0, geom_d1};

    // Topology
    auto topology = grid->FirstChildElement("Topology");
    auto topo_data_item = topology ? topology->FirstChildElement("DataItem") : nullptr;
    if (!topology || !topo_data_item) {
        std::cerr << "Missing Topology or its DataItem\n";
        return false;
    }

    int num_elements = 0;
    int nodes_per_element = 0;
    topology->QueryIntAttribute("NumberOfElements", &num_elements);
    topology->QueryIntAttribute("NodesPerElement", &nodes_per_element);

    std::string topo_path;
    int topo_d0 = 0, topo_d1 = 0;
    if (!ParseDataItem(topo_data_item, topo_path, topo_d0, topo_d1)) return false;

    if (topo_d0 != num_elements || topo_d1 != nodes_per_element) {
        std::cerr << "Topology dimensions mismatch\n";
        return false;
    }

    mesh.topology = {topo_path, num_elements, nodes_per_element};
    return true;
}

bool XdmfMeshLoader::ParseDataItem(tinyxml2::XMLElement* dataItemElem, std::string& out_path, int& out_dim0, int& out_dim1) {
    if (!dataItemElem) return false;

    const char* format = dataItemElem->Attribute("Format");
    if (!format || std::string(format) != "HDF") {
        std::cerr << "Only HDF format supported.\n";
        return false;
    }

    const char* dim_str = dataItemElem->Attribute("Dimensions");
    if (!dim_str) {
        std::cerr << "No Dimensions attribute.\n";
        return false;
    }

    sscanf(dim_str, "%d %d", &out_dim0, &out_dim1);

    const char* text = dataItemElem->GetText();
    if (!text) return false;

    std::string path = text;
    size_t start = path.find_first_not_of(" \t\r\n");
    size_t end = path.find_last_not_of(" \t\r\n");
    out_path = path.substr(start, end - start + 1);

    return true;
}

std::pair<std::string, std::string> XdmfMeshLoader::ParseHDF5Path(const std::string& full_path) {
    size_t pos = full_path.find(":");
    if (pos == std::string::npos) throw std::runtime_error("Invalid HDF5 path: " + full_path);
    return {full_path.substr(0, pos), full_path.substr(pos + 1)};
}

std::vector<double> XdmfMeshLoader::ReadPoints(const std::string& filename, const std::string& dataset, int num_points, int dim) {
    std::vector<double> points(num_points * dim);
    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    hid_t dset = H5Dopen(file, dataset.c_str(), H5P_DEFAULT);
    H5Dread(dset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, points.data());
    H5Dclose(dset);
    H5Fclose(file);
    return points;
}

std::vector<uint64_t> XdmfMeshLoader::ReadIndices(const std::string& filename, const std::string& dataset, int num_elements, int nodes_per_element) {
    std::vector<uint64_t> indices(num_elements * nodes_per_element);
    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    hid_t dset = H5Dopen(file, dataset.c_str(), H5P_DEFAULT);
    H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, indices.data());
    H5Dclose(dset);
    H5Fclose(file);
    return indices;
}

std::vector<double> XdmfMeshLoader::ReadGeometryData() const {
    auto [filename, dataset] = ParseHDF5Path(mesh.geometry.hdf5_path);
    return ReadPoints(filename, dataset, mesh.geometry.num_points, mesh.geometry.dim);
}

std::vector<uint64_t> XdmfMeshLoader::ReadTopologyData() const {
    auto [filename, dataset] = ParseHDF5Path(mesh.topology.hdf5_path);
    return ReadIndices(filename, dataset, mesh.topology.num_elements, mesh.topology.nodes_per_element);
}

// 示例主函数
int main() {
    XdmfMeshLoader loader;
    if (!loader.Load("disk_2d.xdmf")) {
        std::cerr << "Failed to load mesh.\n";
        return -1;
    }

    const auto& mesh = loader.GetMeshData();
    std::cout << "Geometry: " << mesh.geometry.num_points << " points, "
              << mesh.geometry.dim << "D, from " << mesh.geometry.hdf5_path << "\n";
    std::cout << "Topology: " << mesh.topology.num_elements << " elements, "
              << mesh.topology.nodes_per_element << " nodes/elem, from "
              << mesh.topology.hdf5_path << "\n";

    auto points = loader.ReadGeometryData();
    auto indices = loader.ReadTopologyData();

    std::cout << "Read " << points.size() << " point coordinates and "
              << indices.size() << " topology indices.\n";

    std::getchar();
    return 0;
}
