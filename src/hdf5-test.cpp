
/*
#include <iostream>
#include "H5public.h"
#include "hdf5.h"

int main() {
    const char* filename = "example.h5";
    const char* dataset_name = "my_dataset";

    // 要写入的数据
    const int NX = 5;
    int data[NX] = {1, 2, 3, 4, 5};

    // 创建 HDF5 文件
    hid_t file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id < 0) {
        std::cerr << "Failed to create file." << std::endl;
        return 1;
    }

    // 创建数据空间
    hsize_t dims[1] = {NX};
    hid_t dataspace_id = H5Screate_simple(1, dims, NULL);

    // 创建数据集
    hid_t dataset_id = H5Dcreate(file_id, dataset_name, H5T_NATIVE_INT,
                                 dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    // 写入数据
    H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
             H5P_DEFAULT, data);

    // 关闭资源
    H5Dclose(dataset_id);
    H5Sclose(dataspace_id);
    H5Fclose(file_id);

    std::cout << "Data written to example.h5\n";

    // ----------------- 读取数据 -----------------
    int read_data[NX] = {0};

    file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    dataset_id = H5Dopen(file_id, dataset_name, H5P_DEFAULT);

    H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
            H5P_DEFAULT, read_data);

    std::cout << "Read data: ";
    for (int i = 0; i < NX; ++i) {
        std::cout << read_data[i] << " ";
    }
    std::cout << std::endl;

    H5Dclose(dataset_id);
    H5Fclose(file_id);

    return 0;
}
*/


#include <iostream>
#include <vector>
#include <string>
#include "tinyxml2.h"
#include "H5public.h"
#include "hdf5.h"

// 简化表示 Geometry 数据
struct GeometryData {
    std::string hdf5_path;    // e.g. "disk_2d.h5:/data0"
    int num_points = 0;
    int dim = 0;              // 2 for XY
};

// 简化表示 Topology 数据
struct TopologyData {
    std::string hdf5_path;    // e.g. "disk_2.h5:/data1"
    int num_elements = 0;
    int nodes_per_element = 0; // 4 for quadrilateral
};

// 网格
struct MeshData {
    GeometryData geometry;
    TopologyData topology;
};

// 从 <DataItem> 标签中提取 HDF5 路径和维度
bool ParseDataItem(tinyxml2::XMLElement* dataItemElem, std::string& out_path, int& out_dim0, int& out_dim1) {
    if (!dataItemElem) return false;

    const char* format = dataItemElem->Attribute("Format");
    if (!format || std::string(format) != "HDF") {
        std::cerr << "Only HDF format supported.\n";
        return false;
    }

    const char* dim_str = dataItemElem->Attribute("Dimensions");
    if (!dim_str) {
        std::cerr << "No Dimensions attribute found.\n";
        return false;
    }

    // 解析 Dimensions "43176 2"
    int d0 = 0, d1 = 0;
    sscanf(dim_str, "%d %d", &d0, &d1);

    const char* text = dataItemElem->GetText();
    if (!text) {
        std::cerr << "No text content for DataItem.\n";
        return false;
    }

    // 读取路径，去除前后空白
    std::string path = text;
    // 去空白
    size_t start = path.find_first_not_of(" \n\t\r");
    size_t end = path.find_last_not_of(" \n\t\r");
    path = path.substr(start, end - start + 1);

    out_path = path;
    out_dim0 = d0;
    out_dim1 = d1;

    return true;
}

// 解析 XDMF 文件，填充 Mesh 结构
bool ParseXdmf(const char* filename, MeshData& mesh) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError err = doc.LoadFile(filename);
    if (err != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to load XML file\n";
        return false;
    }

    auto root = doc.RootElement(); // Xdmf
    if (!root) {
        std::cerr << "No root element\n";
        return false;
    }

    auto domain = root->FirstChildElement("Domain");
    if (!domain) {
        std::cerr << "No Domain element\n";
        return false;
    }

    auto grid = domain->FirstChildElement("Grid");
    if (!grid) {
        std::cerr << "No Grid element\n";
        return false;
    }

    // 解析 Geometry
    auto geometry = grid->FirstChildElement("Geometry");
    if (!geometry) {
        std::cerr << "No Geometry element\n";
        return false;
    }
    const char* geom_type = geometry->Attribute("GeometryType");
    if (!geom_type) {
        std::cerr << "No GeometryType attribute\n";
        return false;
    }
    // 这里可以用 geom_type 做校验，如是否为 XY

    auto geom_data_item = geometry->FirstChildElement("DataItem");
    if (!geom_data_item) {
        std::cerr << "No Geometry DataItem\n";
        return false;
    }

    std::string geom_path;
    int geom_d0 = 0, geom_d1 = 0;
    if (!ParseDataItem(geom_data_item, geom_path, geom_d0, geom_d1)) {
        std::cerr << "Failed to parse Geometry DataItem\n";
        return false;
    }
    mesh.geometry.hdf5_path = geom_path;
    mesh.geometry.num_points = geom_d0;
    mesh.geometry.dim = geom_d1;

    // 解析 Topology
    auto topology = grid->FirstChildElement("Topology");
    if (!topology) {
        std::cerr << "No Topology element\n";
        return false;
    }

    int num_elements = 0;
    topology->QueryIntAttribute("NumberOfElements", &num_elements);

    int nodes_per_element = 0;
    topology->QueryIntAttribute("NodesPerElement", &nodes_per_element);

    auto topo_data_item = topology->FirstChildElement("DataItem");
    if (!topo_data_item) {
        std::cerr << "No Topology DataItem\n";
        return false;
    }

    std::string topo_path;
    int topo_d0 = 0, topo_d1 = 0;
    if (!ParseDataItem(topo_data_item, topo_path, topo_d0, topo_d1)) {
        std::cerr << "Failed to parse Topology DataItem\n";
        return false;
    }

    mesh.topology.hdf5_path = topo_path;
    mesh.topology.num_elements = num_elements;
    mesh.topology.nodes_per_element = nodes_per_element;

    // 简单校验 Dimensions 是否和属性匹配
    if (topo_d0 != num_elements || topo_d1 != nodes_per_element) {
        std::cerr << "Topology Dimensions mismatch\n";
        return false;
    }

    return true;
}

std::vector<double> ReadPoints(const std::string& filename, const std::string& dataset, int num_points, int dim) {
    std::vector<double> points(num_points * dim);

    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    hid_t dset = H5Dopen(file, dataset.c_str(), H5P_DEFAULT);

    H5Dread(dset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, points.data());

    H5Dclose(dset);
    H5Fclose(file);

    return points;
}

std::vector<uint64_t> ReadIndices(const std::string& filename, const std::string& dataset, int num_elements, int nodes_per_element) {
    std::vector<uint64_t> indices(num_elements * nodes_per_element);

    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    hid_t dset = H5Dopen(file, dataset.c_str(), H5P_DEFAULT);

    H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, indices.data());

    H5Dclose(dset);
    H5Fclose(file);

    return indices;
}

std::pair<std::string, std::string> ParseHDF5Path(const std::string& full_path) {
    size_t pos = full_path.find(":");
    if (pos == std::string::npos || pos + 1 >= full_path.size()) {
        throw std::runtime_error("Invalid HDF5 path format: " + full_path);
    }

    std::string filename = full_path.substr(0, pos);
    std::string dataset = full_path.substr(pos + 1);  // 不包括冒号

    return {filename, dataset};
}

int main() {
    MeshData mesh;
    bool ok = ParseXdmf("disk_2d.xdmf", mesh);
    if (!ok) {
        std::cerr << "Failed to parse XDMF\n";
        return -1;
    }

    std::cout << "Geometry: " << mesh.geometry.num_points << " points, dimension " << mesh.geometry.dim
              << ", data from " << mesh.geometry.hdf5_path << "\n";
    std::cout << "Topology: " << mesh.topology.num_elements << " elements, " << mesh.topology.nodes_per_element
              << " nodes per element, data from " << mesh.topology.hdf5_path << "\n";

    auto [filename, geo_dataset] = ParseHDF5Path(mesh.geometry.hdf5_path);
    std::vector<double> a = ReadPoints(filename, geo_dataset, mesh.geometry.num_points, mesh.geometry.dim);

    auto [filename2, top_dataset] = ParseHDF5Path(mesh.topology.hdf5_path);
    std::vector<uint64_t> b = ReadIndices(filename2, top_dataset, mesh.topology.num_elements, mesh.topology.nodes_per_element);

    std::getchar();
}
