/*****************************************************************************

  Example program for vtzero library.

  vtzero-show - Show content of vector tile

*****************************************************************************/

#include "utils.hpp"

#include <vtzero/output.hpp>
#include <vtzero/vector_tile.hpp>

#include <clara.hpp>

#include <cstdint>
#include <exception>
#include <iostream>
#include <string>

class geom_handler {

    std::string m_output{};
    const vtzero::layer& m_layer;
    vtzero::index_value m_scaling{};

    bool m_is_3d = false;

    void close_list() {
        if (m_output.empty()) {
            return;
        }
        if (m_output.back() == ',') {
            m_output.back() = ')';
        }
    }

    void append_point(const vtzero::point_3d& point) {
        m_output += std::to_string(point.x);
        m_output += ' ';
        m_output += std::to_string(point.y);
        if (m_is_3d) {
            m_output += ' ';
            m_output += std::to_string(point.z);
        }
        m_output += ',';
    }

public:

    constexpr static const int dimensions = 3;
    constexpr static const unsigned int max_geometric_attributes = 0;

    explicit geom_handler(const vtzero::layer& layer, bool is_3d) :
        m_layer(layer),
        m_is_3d(is_3d) {
    }

    void points_begin(const uint32_t /*count*/) const noexcept {
    }

    void points_point(const vtzero::point_3d point) const {
        std::cout << "      POINT(" << point.x << ' ' << point.y;
        if (m_is_3d) {
            std::cout << ' ' << point.z;
        }
        std::cout << ")\n";
    }

    void points_end() const noexcept {
    }

    void linestring_begin(const uint32_t count) {
        m_output = "      LINESTRING[count=";
        m_output += std::to_string(count);
        m_output += "](";
    }

    void linestring_point(const vtzero::point_3d point) {
        append_point(point);
    }

    void linestring_end() {
        close_list();
        m_output += '\n';
        std::cout << m_output;
    }

    void ring_begin(const uint32_t count) {
        m_output = "      RING[count=";
        m_output += std::to_string(count);
        m_output += "](";
    }

    void ring_point(const vtzero::point_3d point) {
        append_point(point);
    }

    void ring_end(const vtzero::ring_type rt) {
        close_list();
        switch (rt) {
            case vtzero::ring_type::outer:
                m_output += "[OUTER]\n";
                break;
            case vtzero::ring_type::inner:
                m_output += "[INNER]\n";
                break;
            default:
                m_output += "[INVALID]\n";
        }
        std::cout << m_output;
    }

    void controlpoints_begin(const uint32_t count) {
        m_output = "      SPLINE[count=";
        m_output += std::to_string(count);
        m_output += "](";
    }

    void controlpoints_point(const vtzero::point_3d point) {
        append_point(point);
    }

    void controlpoints_end() {
        close_list();
        m_output += ',';
    }

    void knots_begin(const uint32_t count, vtzero::index_value scaling) {
        m_scaling = scaling;
        m_output += "KNOTS[count=";
        m_output += std::to_string(count);
        m_output += ",scaling=";
        m_output += std::to_string(scaling.value());
        m_output += "](";
    }

    void knots_value(const int64_t value) {
        m_output += std::to_string(value);
        m_output += '(';
        m_output.append(std::to_string(m_layer.attribute_scaling(m_scaling).decode(value)));
        m_output += "),";
    }

    void knots_end() {
        close_list();
        m_output += '\n';
        std::cout << m_output;
    }

}; // class geom_handler

template <typename TChar, typename TTraits>
std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, vtzero::data_view value) {
    out.write(value.data(), static_cast<std::streamsize>(value.size()));
    return out;
}

struct print_value {

    template <typename T>
    void operator()(T value) const {
        std::cout << value;
    }

    void operator()(const vtzero::data_view value) const {
        std::cout << '"' << value << '"';
    }

}; // struct print_value

class print_handler {

    std::string m_output{};
    const vtzero::layer& m_layer;
    vtzero::index_value m_scaling{};
    int m_nested_list_count = 0;

    void print_nested_value(std::size_t depth) const {
        if (m_nested_list_count) {
            std::cout << "      ";
            for (; depth > 0; --depth) {
                std::cout << "    ";
            }
        }
    }

public:

    print_handler(const vtzero::layer& layer) :
        m_layer(layer) {
    }

    void key_index(vtzero::index_value index, std::size_t depth) const {
        for (; depth > 0; --depth) {
            std::cout << "    ";
        }
        std::cout << "      [" << index.value() << "] ";
    }

    void attribute_key(const vtzero::data_view& key, std::size_t /*depth*/) const {
        std::cout << '{' << key << "} = ";
    }

    void value_index(vtzero::index_value index, std::size_t /*depth*/) const {
        std::cout << '[' << index.value() << "] ";
    }

    void attribute_value(bool value, std::size_t depth) const {
        print_nested_value(depth);
        std::cout << (value ? "true\n" : "false\n");
    }

    void attribute_value(const vtzero::data_view& value, std::size_t depth) const {
        print_nested_value(depth);
        std::cout << '"' << value << '"' << '\n';
    }

    template <typename T>
    void attribute_value(T value, std::size_t depth) const {
        print_nested_value(depth);
        std::cout << value << '\n';
    }

    void start_list_attribute(std::size_t count, std::size_t depth) {
        print_nested_value(depth);
        std::cout << "LIST[count=" << count << "]\n";
        ++m_nested_list_count;
    }

    void end_list_attribute(std::size_t /*depth*/) {
        --m_nested_list_count;
    }

    void start_map_attribute(std::size_t count, std::size_t depth) {
        print_nested_value(depth);
        std::cout << "MAP[count=" << count << "]\n";
    }

    void start_number_list(std::size_t count, vtzero::index_value scaling, std::size_t depth) {
        print_nested_value(depth);
        std::cout << "NUMBER-LIST[count=" << count << ",scaling=" << scaling.value() << ",values=";
        m_scaling = scaling;
    }

    void number_list_value(int64_t value, std::size_t /*depth*/) {
        m_output.append(std::to_string(value));
        m_output += '(';
        m_output.append(std::to_string(m_layer.attribute_scaling(m_scaling).decode(value)));
        m_output += "),";
    }

    void number_list_null_value(std::size_t /*depth*/) {
        m_output += "null,";
    }

    void end_number_list(std::size_t /*depth*/) {
        if (!m_output.empty()) {
            m_output.back() = ']';
        }
        std::cout << m_output << '\n';
        m_output.clear();
    }

}; // class print_handler

static void print_scaling(const vtzero::scaling& scaling) {
    std::cout << "offset=" << scaling.offset() << " multiplier=" << scaling.multiplier() << " base=" << scaling.base() << '\n';
}

static void print_layer(const vtzero::layer& layer, bool print_tables, bool print_value_types, int& layer_num, int& feature_num) {
    std::cout << "=============================================================\n"
              << "layer: " << layer_num << '\n'
              << "  name: " << std::string(layer.name()) << '\n'
              << "  version: " << layer.version() << '\n'
              << "  extent: " << layer.extent() << '\n';

    const auto tile = layer.get_tile();
    if (tile.valid()) {
        std::cout << "  x: " << tile.x() << '\n';
        std::cout << "  y: " << tile.y() << '\n';
        std::cout << "  zoom: " << tile.zoom() << '\n';
    }

    const auto elev_scaling = layer.elevation_scaling();
    if (elev_scaling != vtzero::scaling{}) {
        std::cout << "  elevation scaling: ";
        print_scaling(elev_scaling);
    }

    if (layer.num_attribute_scalings() > 0) {
        std::cout << "  attribute scalings:\n";
        for (uint32_t n = 0; n < layer.num_attribute_scalings(); ++n) {
            std::cout << "    [" << n << "] ";
            print_scaling(layer.attribute_scaling(vtzero::index_value{n}));
        }
    }

    if (print_tables) {
        if (!layer.key_table().empty()) {
            std::cout << "  keys:\n";
            int n = 0;
            for (const auto& key : layer.key_table()) {
                std::cout << "    [" << n++ << "] {" << key << "}\n";
            }
        }
        if (!layer.value_table().empty()) {
            std::cout << "  values:\n";
            int n = 0;
            for (const vtzero::property_value& value : layer.value_table()) {
                std::cout << "    [" << n++ << "] ";
                vtzero::apply_visitor(print_value{}, value);
                if (print_value_types) {
                    std::cout << " [" << vtzero::property_value_type_name(value.type()) << "]\n";
                } else {
                    std::cout << '\n';
                }
            }
        }
        if (layer.version() == 3) {
            if (!layer.string_table().empty()) {
                std::cout << "  string values:\n";
                int n = 0;
                for (const vtzero::data_view& value : layer.string_table()) {
                    std::cout << "    [" << n++ << "] \"" << value << "\"\n";
                }
            }
            if (!layer.float_table().empty()) {
                std::cout << "  float values:\n";
                for (std::uint32_t n = 0; n < layer.float_table().size(); ++n) {
                    std::cout << "    [" << n << "] " << layer.float_table().at(n) << '\n';
                }
            }
            if (!layer.double_table().empty()) {
                std::cout << "  double values:\n";
                for (std::uint32_t n = 0; n < layer.double_table().size(); ++n) {
                    std::cout << "    [" << n << "] " << layer.double_table().at(n) << '\n';
                }
            }
            if (!layer.int_table().empty()) {
                std::cout << "  int values: uint / sint\n";
                for (std::uint32_t i = 0; i < layer.int_table().size(); ++i) {
                    const auto value = layer.int_table().at(i);
                    std::cout << "    [" << i << "] " << value << " / " << protozero::decode_zigzag64(value) << '\n';
                }
            }
        }
    }

    feature_num = 0;
    for (const auto feature : layer) {
        std::cout << "  feature: " << feature_num << '\n'
                  << "    id: ";
        if (feature.has_id()) {
            std::cout << feature.id() << '\n';
        } else if (feature.has_string_id()) {
            std::cout << feature.string_id() << '\n';
        } else {
            std::cout << "(none)\n";
        }
        std::cout << "    geomtype: "
                  << vtzero::geom_type_name(feature.geometry_type())
                  << ' '
                  << (feature.has_3d_geometry() ? '3' : '2')
                  << "D\n";
        if (feature.geometry_type() == vtzero::GeomType::SPLINE) {
            std::cout << "    spline degrees: " << feature.spline_degree() << "\n";
        }
        std::cout << "    geometry:\n";
        feature.decode_geometry(geom_handler{layer, feature.has_3d_geometry()});
        std::cout << "    attributes:\n";
        print_handler handler{layer};
        feature.decode_attributes(handler);

        if (layer.version() == 3) {
            std::cout << "    geometric attributes:\n";
            feature.decode_geometric_attributes(handler);
        }

        ++feature_num;
    }
}

static void print_layer_overview(const vtzero::layer& layer) {
    std::cout << layer.name() << ' ' << layer.num_features() << '\n';
}

int main(int argc, char* argv[]) {
    std::string filename;
    std::string layer_num_or_name;
    bool layer_overview = false;
    bool print_tables = false;
    bool print_value_types = false;
    bool help = false;

    const auto cli
        = clara::Opt(layer_overview)
            ["-l"]["--layers"]
            ("show layer overview with feature count")
        | clara::Opt(print_tables)
            ["-t"]["--tables"]
            ("also print key/value tables")
        | clara::Opt(print_value_types)
            ["-T"]["--value-types"]
            ("also show value types")
        | clara::Help(help)
        | clara::Arg(filename, "FILENAME").required()
            ("vector tile")
        | clara::Arg(layer_num_or_name, "LAYER-NUM|LAYER-NAME")
            ("layer");

    const auto result = cli.parse(clara::Args(argc, argv));
    if (!result) {
        std::cerr << "Error in command line: " << result.errorMessage() << '\n';
        return 1;
    }

    if (help) {
        std::cout << cli
                  << "\nShow contents of vector tile FILENAME.\n";
        return 0;
    }

    if (filename.empty()) {
        std::cerr << "Error in command line: Missing file name of vector tile to read\n";
        return 1;
    }

    int layer_num = 0;
    int feature_num = 0;
    try {
        const auto data = read_file(filename);

        const vtzero::vector_tile tile{data};

        if (layer_num_or_name.empty()) {
            for (const auto layer : tile) {
                if (layer_overview) {
                    print_layer_overview(layer);
                } else {
                    print_layer(layer, print_tables, print_value_types, layer_num, feature_num);
                }
                ++layer_num;
            }
        } else {
            const auto layer = get_layer(tile, layer_num_or_name);
            if (layer_overview) {
                print_layer_overview(layer);
            } else {
                print_layer(layer, print_tables, print_value_types, layer_num, feature_num);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in layer " << layer_num << " (feature " << feature_num << "): " << e.what() << '\n';
        return 1;
    }

    return 0;
}

