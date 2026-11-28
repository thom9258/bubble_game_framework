#include "renderer.hpp"
#include <bubble/animation.hpp>
#include <bubble/model.hpp>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "stb.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <map>
#include <unordered_set>

namespace bubble {

auto maybe_insert(auto &map, std::string_view name, auto &&value) -> bool {
  if (map.contains(std::string(name))) {
    return false;
  }

  map.emplace(std::make_pair(name, value));
  return true;
}

auto GltfMetaData::set_int(std::string_view name, std::int32_t value) -> bool {
  return maybe_insert(_ints, name, value);
}

auto GltfMetaData::set_string(std::string_view name, const std::string &value)
    -> bool {
  return maybe_insert(_strings, name, value);
}

auto GltfMetaData::set_float(std::string_view name, float value) -> bool {
  return maybe_insert(_floats, name, value);
}

auto GltfMetaData::set_bool(std::string_view name, bool value) -> bool {
  return maybe_insert(_bools, name, value);
}

template <typename T>
auto maybe_get(auto &map, std::string_view name) -> std::optional<T> {
  auto found = map.find(std::string(name));
  if (found == map.end()) {
    return std::nullopt;
  }

  return found->second;
}

auto GltfMetaData::get_int(std::string_view name)
    -> std::optional<std::int32_t> {
  return maybe_get<std::int32_t>(_ints, name);
}

auto GltfMetaData::get_string(std::string_view name)
    -> std::optional<std::string> {
  return maybe_get<std::string>(_strings, name);
}

auto GltfMetaData::get_float(std::string_view name) -> std::optional<float> {
  return maybe_get<float>(_floats, name);
}

auto GltfMetaData::get_bool(std::string_view name) -> std::optional<bool> {
  return maybe_get<bool>(_bools, name);
}

struct CgltfDeleter {
  void operator()(cgltf_data *d) const noexcept { cgltf_free(d); }
};

using CgltfPtr = std::unique_ptr<cgltf_data, CgltfDeleter>;

struct StbDeleter {
  void operator()(stbi_uc *p) const noexcept { stbi_image_free(p); }
};

using StbPtr = std::unique_ptr<stbi_uc, StbDeleter>;

auto result_string(cgltf_result r) -> std::string_view {
  switch (r) {
  case cgltf_result_success:
    return "success";
  case cgltf_result_data_too_short:
    return "data too short";
  case cgltf_result_unknown_format:
    return "unknown format";
  case cgltf_result_invalid_json:
    return "invalid json";
  case cgltf_result_invalid_gltf:
    return "invalid gltf";
  case cgltf_result_invalid_options:
    return "invalid options";
  case cgltf_result_file_not_found:
    return "file not found";
  case cgltf_result_io_error:
    return "io error";
  case cgltf_result_out_of_memory:
    return "out of memory";
  case cgltf_result_legacy_gltf:
    return "legacy gltf (1.0) not supported";
  default:
    return "unknown error";
  }
}

static_assert(sizeof(bubble::Color) == 4 && alignof(bubble::Color) == 1,
              "Color must be 4 packed bytes to memcpy RGBA8 pixels into it");

auto make_bitmap(int width, int height, const stbi_uc *rgba) -> bubble::Bitmap {
  const auto w = static_cast<std::uint32_t>(width);
  const auto h = static_cast<std::uint32_t>(height);
  bubble::Bitmap bmp(w, h);
  std::memcpy(bmp.pixels(), rgba,
              static_cast<std::size_t>(w) * h * sizeof(bubble::Color));
  return bmp;
}

// ---------------------------------------------------------------------------
// Images: .glb buffer view, base64 data URI, or external file
// ---------------------------------------------------------------------------
auto load_image(const cgltf_image &img, const std::filesystem::path &base_dir)
    -> std::expected<bubble::Bitmap, std::string> {
  int w = 0, h = 0, c = 0;
  StbPtr px;

  if (img.buffer_view) {
    const auto *bv = img.buffer_view;
    const auto *bytes =
        static_cast<const stbi_uc *>(bv->buffer->data) + bv->offset;
    px.reset(stbi_load_from_memory(bytes, static_cast<int>(bv->size), &w, &h,
                                   &c, 4));
  } else if (img.uri && std::strncmp(img.uri, "data:", 5) == 0) {
    const char *marker = std::strstr(img.uri, ";base64,");
    if (!marker) {
      return std::unexpected("image data URI is not base64");
    }
    const char *b64 = marker + 8;
    const std::size_t len = std::strlen(b64);
    const std::size_t pad =
        (len >= 1 && b64[len - 1] == '=') + (len >= 2 && b64[len - 2] == '=');
    const std::size_t size = len / 4 * 3 - pad;

    cgltf_options opts{};
    void *decoded = nullptr;
    if (cgltf_load_buffer_base64(&opts, size, b64, &decoded) !=
        cgltf_result_success) {
      return std::unexpected("failed to decode base64 image");
    }
    px.reset(stbi_load_from_memory(static_cast<const stbi_uc *>(decoded),
                                   static_cast<int>(size), &w, &h, &c, 4));
    std::free(decoded); // cgltf's default allocator is malloc
  } else if (img.uri) {
    std::string uri = img.uri;
    cgltf_decode_uri(uri.data()); // undo %20 etc. in place
    uri.resize(std::strlen(uri.c_str()));
    const auto file = base_dir / uri;
    px.reset(stbi_load(file.string().c_str(), &w, &h, &c, 4));
    if (!px) {
      return std::unexpected(
          std::format("{}: {}", file.string(), stbi_failure_reason()));
    }
  } else {
    return std::unexpected("image has neither uri nor buffer view");
  }

  if (!px) {
    return std::unexpected(std::format("stb: {}", stbi_failure_reason()));
  }
  return make_bitmap(w, h, px.get());
}

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------
auto find_attribute(const cgltf_primitive &prim, cgltf_attribute_type type,
                    int set = 0) -> const cgltf_accessor * {
  for (cgltf_size i = 0; i < prim.attributes_count; ++i) {
    const auto &a = prim.attributes[i];
    if (a.type == type && a.index == set) {
      return a.data;
    }
  }
  return nullptr;
}

// Handles stride, normalized ints and sparse accessors.
auto unpack_floats(const cgltf_accessor *acc) -> std::vector<float> {
  std::vector<float> out(acc->count * cgltf_num_components(acc->type));
  cgltf_accessor_unpack_floats(acc, out.data(), out.size());
  return out;
}

// Triangle list indices for triangles / strips / fans, indexed or not.
// Empty result = not a triangle primitive.
auto triangle_list_indices(const cgltf_primitive &prim, cgltf_size vertex_count)
    -> std::vector<Index> {
  std::vector<Index> src;
  if (prim.indices) {
    src.resize(prim.indices->count);
    for (cgltf_size i = 0; i < src.size(); ++i) {
      src[i] = static_cast<Index>(cgltf_accessor_read_index(prim.indices, i));
    }
  } else {
    src.resize(vertex_count);
    for (cgltf_size i = 0; i < vertex_count; ++i) {
      src[i] = static_cast<Index>(i);
    }
  }

  std::vector<Index> out;
  switch (prim.type) {
  case cgltf_primitive_type_triangles:
    return src;
  case cgltf_primitive_type_triangle_strip:
    for (std::size_t i = 0; i + 2 < src.size(); ++i) {
      if (i & 1)
        out.insert(out.end(), {src[i + 1], src[i], src[i + 2]});
      else
        out.insert(out.end(), {src[i], src[i + 1], src[i + 2]});
    }
    break;
  case cgltf_primitive_type_triangle_fan:
    for (std::size_t i = 1; i + 1 < src.size(); ++i) {
      out.insert(out.end(), {src[i], src[i + 1], src[0]});
    }
    break;
  default:
    break;
  }
  return out;
}

// glTF spec: if NORMAL is missing, flat normals must be generated.
// Flat shading needs one normal per face, so the mesh is un-indexed:
// every triangle gets its own three vertices (bone data is duplicated along).
auto generate_flat_normals(GltfMesh &mesh) -> void {
  const bool has_bones = !mesh.bone_vertices.empty();
  std::vector<Vertex> vertices;
  std::vector<BoneVertex> bones;
  vertices.reserve(mesh.indices.size());
  if (has_bones)
    bones.reserve(mesh.indices.size());

  for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
    Vertex tri[3] = {mesh.vertices[mesh.indices[i]],
                     mesh.vertices[mesh.indices[i + 1]],
                     mesh.vertices[mesh.indices[i + 2]]};

    const glm::vec3 a = glm::make_vec3(tri[0].position);
    const glm::vec3 b = glm::make_vec3(tri[1].position);
    const glm::vec3 c = glm::make_vec3(tri[2].position);
    glm::vec3 n = glm::cross(b - a, c - a); // CCW front faces
    const float len = glm::length(n);
    n = len > 0.0f ? n / len
                   : glm::vec3{0.0f, 1.0f, 0.0f}; // degenerate triangle

    for (std::size_t k = 0; k < 3; ++k) {
      std::memcpy(tri[k].normal, &n[0], sizeof tri[k].normal);
      vertices.push_back(tri[k]);
      if (has_bones)
        bones.push_back(mesh.bone_vertices[mesh.indices[i + k]]);
    }
  }

  mesh.indices.resize(vertices.size());
  for (std::size_t i = 0; i < mesh.indices.size(); ++i) {
    mesh.indices[i] = static_cast<Index>(i);
  }
  mesh.vertices = std::move(vertices);
  if (has_bones)
    mesh.bone_vertices = std::move(bones);
}

// Returns nullopt for primitives we skip (points/lines).
auto convert_primitive(const cgltf_primitive &prim, const cgltf_data &data)
    -> std::expected<std::optional<GltfMesh>, std::string> {
  const auto *pos = find_attribute(prim, cgltf_attribute_type_position);
  if (!pos || pos->type != cgltf_type_vec3) {
    return std::unexpected("primitive without vec3 POSITION");
  }
  const auto *nrm = find_attribute(prim, cgltf_attribute_type_normal);
  const auto *uv = find_attribute(prim, cgltf_attribute_type_texcoord, 0);
  if (nrm && nrm->type != cgltf_type_vec3)
    nrm = nullptr;
  if (uv && uv->type != cgltf_type_vec2)
    uv = nullptr;

  auto indices = triangle_list_indices(prim, pos->count);
  if (indices.empty()) {
    return std::optional<GltfMesh>{};
  }

  const auto p = unpack_floats(pos);
  const auto n = nrm ? unpack_floats(nrm) : std::vector<float>{};
  const auto t = uv ? unpack_floats(uv) : std::vector<float>{};

  GltfMesh mesh;
  mesh.vertices.resize(pos->count); // value-initialized: missing normal/uv = 0
  for (cgltf_size i = 0; i < pos->count; ++i) {
    auto &v = mesh.vertices[i];
    std::memcpy(v.position, &p[i * 3], sizeof v.position);
    if (nrm)
      std::memcpy(v.normal, &n[i * 3], sizeof v.normal);
    if (uv)
      std::memcpy(v.texture_coordinate, &t[i * 2], sizeof v.texture_coordinate);
  }
  mesh.indices = std::move(indices);

  const auto *joints = find_attribute(prim, cgltf_attribute_type_joints);
  const auto *weights = find_attribute(prim, cgltf_attribute_type_weights);
  if (joints && weights && joints->type == cgltf_type_vec4 &&
      weights->type == cgltf_type_vec4 && joints->count == pos->count &&
      weights->count == pos->count) {
    const auto w = unpack_floats(weights);
    mesh.bone_vertices.resize(pos->count);
    for (cgltf_size i = 0; i < pos->count; ++i) {
      cgltf_uint j[4] = {};
      cgltf_accessor_read_uint(joints, i, j, 4);
      auto &b = mesh.bone_vertices[i];
      float sum = 0.0f;
      for (std::size_t k = 0; k < 4; ++k) {
        b.ids[k] = static_cast<int>(j[k]);
        b.weights[k] = w[i * 4 + k];
        sum += b.weights[k];
      }
      if (sum > 0.0f) {
        for (float &x : b.weights)
          x /= sum;
      } else {
        b.weights[0] = 1.0f;
      }
    }
  }

  // Must run last among the geometry steps (after bone weights, if read),
  // since it duplicates vertices.
  if (!nrm) {
    generate_flat_normals(mesh);
  }

  if (prim.material) {
    mesh.material = static_cast<std::size_t>(prim.material - data.materials);
  }

  return std::optional<GltfMesh>{std::move(mesh)};
}

// ---------------------------------------------------------------------------
// Node tree
// ---------------------------------------------------------------------------
struct CgltfContext {
  const cgltf_data *data;
  // glTF mesh index -> indices into GltfScene::meshes (one per kept primitive)
  std::vector<std::vector<std::size_t>> mesh_map;
  std::vector<std::string> node_names; // unique, indexed like data->nodes
};

auto node_index(const cgltf_data &data, const cgltf_node &node) -> std::size_t {
  return static_cast<std::size_t>(&node - data.nodes);
}

auto unique_node_names(const cgltf_data &data) -> std::vector<std::string> {
  std::vector<std::string> names;
  std::unordered_set<std::string> used;
  for (cgltf_size i = 0; i < data.nodes_count; ++i) {
    const auto &n = data.nodes[i];
    std::string name = (n.name && *n.name) ? n.name : std::format("node_{}", i);
    if (!used.insert(name).second) {
      name = std::format("{}#{}", name, i);
      used.insert(name);
    }
    names.push_back(std::move(name));
  }
  return names;
}

// ---------------------------------------------------------------------------
// Extras -> GltfMetaData
//
// Blender writes object custom properties into node.extras as a JSON object
// (glTF export: Include -> Data -> Custom Properties).
//   "tag": "door"        -> string
//   "key_id": 3          -> int   (no '.', 'e' or 'E'; fits in int32)
//   "speed": 2.5         -> float
//   "solid": true        -> bool
//   "door": {"id": 3}    -> flattened to "door.id" -> int
//   arrays, null         -> skipped (GltfMetaData has no array type)
// ---------------------------------------------------------------------------
class ExtrasParser {
public:
  ExtrasParser(std::string_view json, GltfMetaData &out)
      : _s(json), _out(out) {}

  auto parse() -> std::expected<void, std::string> {
    if (!consume('{')) {
      return fail("extras is not a JSON object");
    }
    return object_body("");
  }

private:
  std::string_view _s;
  std::size_t _i = 0;
  GltfMetaData &_out;

  auto fail(std::string_view what) const -> std::unexpected<std::string> {
    return std::unexpected(std::format("extras json, offset {}: {}", _i, what));
  }

  void ws() {
    while (_i < _s.size() && (_s[_i] == ' ' || _s[_i] == '\n' ||
                              _s[_i] == '\r' || _s[_i] == '\t')) {
      ++_i;
    }
  }

  auto peek() const -> char { return _i < _s.size() ? _s[_i] : '\0'; }

  auto consume(char c) -> bool {
    ws();
    if (peek() == c) {
      ++_i;
      return true;
    }
    return false;
  }

  auto literal(std::string_view lit) -> bool {
    if (_s.substr(_i, lit.size()) == lit) {
      _i += lit.size();
      return true;
    }
    return false;
  }

  // Called after '{' has been consumed.
  auto object_body(const std::string &prefix)
      -> std::expected<void, std::string> {
    if (consume('}')) {
      return {};
    }
    while (true) {
      ws();
      auto key = string();
      if (!key) {
        return std::unexpected(std::move(key.error()));
      }
      if (!consume(':')) {
        return fail("expected ':'");
      }
      const auto full = prefix.empty() ? *key : prefix + "." + *key;
      if (auto r = value(full); !r) {
        return r;
      }
      if (consume(','))
        continue;
      if (consume('}'))
        return {};
      return fail("expected ',' or '}'");
    }
  }

  auto value(const std::string &key) -> std::expected<void, std::string> {
    ws();
    switch (peek()) {
    case '"': {
      auto s = string();
      if (!s) {
        return std::unexpected(std::move(s.error()));
      }
      _out.set_string(key, *s);
      return {};
    }
    case '{':
      ++_i;
      return object_body(key);
    case '[':
      return skip();
    case 't':
      if (literal("true")) {
        _out.set_bool(key, true);
        return {};
      }
      break;
    case 'f':
      if (literal("false")) {
        _out.set_bool(key, false);
        return {};
      }
      break;
    case 'n':
      if (literal("null"))
        return {};
      break;
    default:
      return number(key);
    }
    return fail("invalid literal");
  }

  auto number(const std::string &key) -> std::expected<void, std::string> {
    const auto start = _i;
    bool is_float = false;
    while (_i < _s.size()) {
      const char c = _s[_i];
      if (c == '.' || c == 'e' || c == 'E') {
        is_float = true;
      } else if (!(c == '-' || c == '+' || (c >= '0' && c <= '9'))) {
        break;
      }
      ++_i;
    }
    const auto text = _s.substr(start, _i - start);
    if (text.empty()) {
      return fail("unexpected character");
    }
    const char *b = text.data();
    const char *e = b + text.size();

    if (!is_float) {
      std::int64_t v{};
      const auto [p, ec] = std::from_chars(b, e, v);
      if (ec == std::errc{} && p == e &&
          v >= std::numeric_limits<std::int32_t>::min() &&
          v <= std::numeric_limits<std::int32_t>::max()) {
        _out.set_int(key, static_cast<std::int32_t>(v));
        return {};
      }
      // out of int32 range: fall through and store as float
    }

    float f{};
    const auto [p, ec] = std::from_chars(b, e, f);
    if (ec != std::errc{} || p != e) {
      return fail("invalid number");
    }
    _out.set_float(key, f);
    return {};
  }

  auto hex4() -> std::optional<std::uint32_t> {
    if (_i + 4 > _s.size()) {
      return std::nullopt;
    }
    std::uint32_t v{};
    const auto [p, ec] =
        std::from_chars(_s.data() + _i, _s.data() + _i + 4, v, 16);
    if (ec != std::errc{} || p != _s.data() + _i + 4) {
      return std::nullopt;
    }
    _i += 4;
    return v;
  }

  static void append_utf8(std::string &out, std::uint32_t cp) {
    if (cp < 0x80) {
      out += static_cast<char>(cp);
    } else if (cp < 0x800) {
      out += static_cast<char>(0xC0 | (cp >> 6));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
      out += static_cast<char>(0xE0 | (cp >> 12));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
      out += static_cast<char>(0xF0 | (cp >> 18));
      out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    }
  }

  auto string() -> std::expected<std::string, std::string> {
    if (peek() != '"') {
      return fail("expected string");
    }
    ++_i;
    std::string out;
    while (_i < _s.size()) {
      const char c = _s[_i++];
      if (c == '"') {
        return out;
      }
      if (c != '\\') {
        out += c;
        continue;
      }
      if (_i >= _s.size()) {
        break;
      }
      switch (const char esc = _s[_i++]; esc) {
      case '"':
        out += '"';
        break;
      case '\\':
        out += '\\';
        break;
      case '/':
        out += '/';
        break;
      case 'b':
        out += '\b';
        break;
      case 'f':
        out += '\f';
        break;
      case 'n':
        out += '\n';
        break;
      case 'r':
        out += '\r';
        break;
      case 't':
        out += '\t';
        break;
      case 'u': {
        auto hi = hex4();
        if (!hi)
          return fail("bad \\u escape");
        std::uint32_t cp = *hi;
        if (cp >= 0xD800 && cp <= 0xDBFF && literal("\\u")) { // surrogate pair
          auto lo = hex4();
          if (!lo)
            return fail("bad \\u escape");
          cp = 0x10000 + ((cp - 0xD800) << 10) + (*lo - 0xDC00);
        }
        append_utf8(out, cp);
        break;
      }
      default:
        return fail("bad escape");
      }
    }
    return fail("unterminated string");
  }

  // Skips any JSON value (used for arrays).
  auto skip() -> std::expected<void, std::string> {
    ws();
    const char c = peek();
    if (c == '"') {
      auto s = string();
      if (!s)
        return std::unexpected(std::move(s.error()));
      return {};
    }
    if (c == '{' || c == '[') {
      const char close = (c == '{') ? '}' : ']';
      ++_i;
      if (consume(close)) {
        return {};
      }
      while (true) {
        if (c == '{') {
          ws();
          auto k = string();
          if (!k)
            return std::unexpected(std::move(k.error()));
          if (!consume(':'))
            return fail("expected ':'");
        }
        if (auto r = skip(); !r)
          return r;
        if (consume(','))
          continue;
        if (consume(close))
          return {};
        return fail("expected ',' or closing bracket");
      }
    }
    // number / true / false / null
    while (_i < _s.size() && _s[_i] != ',' && _s[_i] != '}' && _s[_i] != ']' &&
           _s[_i] != ' ' && _s[_i] != '\t' && _s[_i] != '\r' &&
           _s[_i] != '\n') {
      ++_i;
    }
    return {};
  }
};

// cgltf >= 1.14 fills extras.data; older versions only give offsets into the
// JSON.
auto extras_json(const cgltf_data &data, const cgltf_extras &extras)
    -> std::string_view {
  if (extras.data) {
    return extras.data;
  }
  if (extras.end_offset > extras.start_offset) {
    return {data.json + extras.start_offset,
            extras.end_offset - extras.start_offset};
  }
  return {};
}

auto convert_node(const cgltf_node &node, const CgltfContext &ctx)
    -> GltfModel {
  GltfModel model;
  const auto node_idx = static_cast<std::size_t>(&node - ctx.data->nodes);
  model.name = ctx.node_names[node_idx];

  float local[16];
  cgltf_node_transform_local(&node, local); // column-major, same as glm
  model.transform = glm::make_mat4(local);

  if (node.mesh) {
    const auto mesh_idx =
        static_cast<std::size_t>(node.mesh - ctx.data->meshes);
    model.meshes = ctx.mesh_map[mesh_idx];
  }
  // metadata from node.extras (JSON, e.g. Blender custom properties)
  if (const auto json = extras_json(*ctx.data, node.extras); !json.empty()) {
    ExtrasParser(json, model.metadata)
        .parse(); // returns std::expected<void, std::string>
  }

  model.children.reserve(node.children_count);
  for (cgltf_size i = 0; i < node.children_count; ++i) {
    model.children.push_back(convert_node(*node.children[i], ctx));
  }
  return model;
}

auto build_skeleton(const cgltf_node &node, const CgltfContext &ctx)
    -> animation::Skeleton {
  float local[16];
  cgltf_node_transform_local(&node, local);
  animation::Skeleton s(ctx.node_names[node_index(*ctx.data, node)],
                        glm::make_mat4(local));
  for (cgltf_size i = 0; i < node.children_count; ++i) {
    s.add_child(build_skeleton(*node.children[i], ctx));
  }
  return s;
}

// Inserted in joint order, so BoneInfo id == glTF joint index == BoneVertex id.
auto build_bone_infos(const cgltf_skin &skin, const CgltfContext &ctx)
    -> animation::BoneInfos {
  animation::BoneInfos infos;
  for (cgltf_size i = 0; i < skin.joints_count; ++i) {
    glm::mat4 ibm(1.0f);
    if (skin.inverse_bind_matrices) {
      cgltf_accessor_read_float(skin.inverse_bind_matrices, i,
                                glm::value_ptr(ibm), 16);
    }
    infos.insert_bone(ctx.node_names[node_index(*ctx.data, *skin.joints[i])],
                      ibm);
  }
  return infos;
}

// (time, value) pairs; handles all three glTF interpolation modes.
template <std::size_t N>
auto read_track(const cgltf_animation_sampler &s)
    -> std::vector<std::pair<double, std::array<float, N>>> {
  std::vector<std::pair<double, std::array<float, N>>> keys;
  const bool cubic = s.interpolation == cgltf_interpolation_type_cubic_spline;
  for (cgltf_size i = 0; i < s.input->count; ++i) {
    float t = 0.0f;
    cgltf_accessor_read_float(s.input, i, &t, 1);
    std::array<float, N> v{};
    // cubic spline stores [in-tangent, value, out-tangent] per key: take the
    // value
    cgltf_accessor_read_float(s.output, cubic ? i * 3 + 1 : i, v.data(), N);
    if (s.interpolation == cgltf_interpolation_type_step && !keys.empty()) {
      // hold the previous value until just before this key
      keys.push_back(
          {std::nextafter(static_cast<double>(t), 0.0), keys.back().second});
    }
    keys.push_back({t, v});
  }
  return keys;
}

// Bone::*_index() fails once time passes a track's last key, so every
// track must reach the end of the clip.
template <typename Key>
void extend_to(std::vector<Key> &keys, double duration) {
  if (keys.size() > 1 && keys.back().timeStamp < duration) {
    auto last = keys.back();
    last.timeStamp = duration;
    keys.push_back(last);
  }
}

auto load_animation(const cgltf_animation &anim, std::size_t index,
                    const CgltfContext &ctx, animation::Skeleton &skeleton,
                    animation::BoneInfos &bone_infos) -> GltfAnimation {
  double duration = 0.0;
  for (cgltf_size c = 0; c < anim.channels_count; ++c) {
    const auto *in = anim.channels[c].sampler->input;
    float last = 0.0f;
    cgltf_accessor_read_float(in, in->count - 1, &last, 1);
    duration = std::max(duration, static_cast<double>(last));
  }

  std::map<std::size_t, animation::Bone> bones; // one Bone per animated node
  for (cgltf_size c = 0; c < anim.channels_count; ++c) {
    const auto &ch = anim.channels[c];
    if (!ch.target_node || !ch.sampler)
      continue;
    const auto ni = node_index(*ctx.data, *ch.target_node);
    const auto &name = ctx.node_names[ni];
    auto &bone =
        bones.try_emplace(ni, name, bone_infos.find_bone_id(name).value_or(-1))
            .first->second;

    switch (ch.target_path) {
    case cgltf_animation_path_type_translation:
      for (const auto &[t, v] : read_track<3>(*ch.sampler))
        bone.m_Positions.push_back({glm::vec3(v[0], v[1], v[2]), t});
      break;
    case cgltf_animation_path_type_rotation:
      for (const auto &[t, v] :
           read_track<4>(*ch.sampler)) // glTF xyzw -> glm (w,x,y,z)
        bone.m_Rotations.push_back(
            {glm::normalize(glm::quat(v[3], v[0], v[1], v[2])), t});
      break;
    case cgltf_animation_path_type_scale:
      for (const auto &[t, v] : read_track<3>(*ch.sampler))
        bone.m_Scales.push_back({glm::vec3(v[0], v[1], v[2]), t});
      break;
    default: // morph target weights: not supported
      break;
    }
  }

  const double total = duration > 0.0 ? duration : 1.0; // avoid fmod(x, 0)
  auto clip = animation::Animation(animation::copy(skeleton),
                                   animation::TotalTicks(total),
                                   animation::TicksPerSecond(1.0));
  clip.set_bone_infos(bone_infos);

  for (auto &[ni, bone] : bones) {
    // Bone needs >= 1 key per path: use the node's rest pose for paths this
    // clip doesn't touch.
    const auto &node = ctx.data->nodes[ni];
    if (bone.m_Positions.empty())
      bone.m_Positions.push_back({node.has_translation
                                      ? glm::make_vec3(node.translation)
                                      : glm::vec3(0.0f),
                                  0.0});
    if (bone.m_Rotations.empty())
      bone.m_Rotations.push_back(
          {node.has_rotation ? glm::quat(node.rotation[3], node.rotation[0],
                                         node.rotation[1], node.rotation[2])
                             : glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
           0.0});
    if (bone.m_Scales.empty())
      bone.m_Scales.push_back(
          {node.has_scale ? glm::make_vec3(node.scale) : glm::vec3(1.0f), 0.0});

    extend_to(bone.m_Positions, total);
    extend_to(bone.m_Rotations, total);
    extend_to(bone.m_Scales, total);
    clip.add_bone(std::move(bone));
  }

  std::string name = (anim.name && *anim.name)
                         ? anim.name
                         : std::format("animation_{}", index);
  return GltfAnimation{std::move(name), std::move(clip)};
}

auto load_gltf_scene(std::filesystem::path path)
    -> std::expected<GltfScene, std::string> {
  if (!std::filesystem::is_regular_file(path)) {
    return std::unexpected(
        std::format("Invalid gltf path: '{}'", path.string()));
  }

  auto options = cgltf_options{};
  using cgltf_data_ptr = cgltf_data *;
  auto raw = cgltf_data_ptr{nullptr};

  const std::string path_str = path.string();
  const auto base_dir = path.parent_path();
  if (auto r = cgltf_parse_file(&options, path_str.c_str(), &raw);
      r != cgltf_result_success) {
    return std::unexpected(
        std::format("{}: parse failed: {}", path_str, result_string(r)));
  }

  CgltfPtr data(raw);
  if (auto r = cgltf_load_buffers(&options, data.get(), path_str.c_str());
      r != cgltf_result_success) {
    return std::unexpected(std::format("{}: loading buffers failed: {}",
                                       path_str, result_string(r)));
  }

  if (auto r = cgltf_validate(data.get()); r != cgltf_result_success) {
    return std::unexpected(
        std::format("{}: validation failed: {}", path_str, result_string(r)));
  }

  auto scene = GltfScene{};

  // --- Bitmaps: each glTF image decoded at most once, on first use --------
  std::vector<std::optional<std::size_t>> image_to_bitmap(data->images_count);

  auto get_bitmap =
      [&](const cgltf_image &img) -> std::expected<std::size_t, std::string> {
    const auto idx = static_cast<std::size_t>(&img - data->images);
    if (image_to_bitmap[idx]) {
      return *image_to_bitmap[idx];
    }
    auto bmp = load_image(img, base_dir);
    if (!bmp) {
      return std::unexpected(std::move(bmp.error()));
    }
    scene.bitmaps.push_back(std::move(*bmp));
    image_to_bitmap[idx] = scene.bitmaps.size() - 1;
    return scene.bitmaps.size() - 1;
  };

  // --- Materials: 1:1 with glTF materials, so primitive indices map directly
  scene.materials.reserve(data->materials_count);
  for (cgltf_size i = 0; i < data->materials_count; ++i) {
    const auto &m = data->materials[i];
    GltfMaterial mat;
    mat.name = (m.name && *m.name) ? m.name : std::format("material_{}", i);

    const cgltf_texture *tex = nullptr;
    if (m.has_pbr_metallic_roughness) {
      tex = m.pbr_metallic_roughness.base_color_texture.texture;
    } else if (m.has_pbr_specular_glossiness) {
      tex = m.pbr_specular_glossiness.diffuse_texture.texture;
    }
    if (tex && tex->image) {
      auto b = get_bitmap(*tex->image);
      if (!b) {
        return std::unexpected(std::format("{}: material '{}': {}", path_str,
                                           mat.name, b.error()));
      }
      mat.diffuse = *b;
    }
    scene.materials.push_back(std::move(mat));
  }

  // --- Meshes: every glTF primitive once; nodes reference them by index ---
  auto ctx = CgltfContext{data.get(), {}, {}};
  ctx.node_names = unique_node_names(*data); // new

  ctx.mesh_map.resize(data->meshes_count);
  for (cgltf_size mi = 0; mi < data->meshes_count; ++mi) {
    const auto &gm = data->meshes[mi];
    for (cgltf_size pi = 0; pi < gm.primitives_count; ++pi) {
      auto mesh = convert_primitive(gm.primitives[pi], *data);
      if (!mesh) {
        return std::unexpected(std::format("{}: mesh {} primitive {}: {}",
                                           path_str, mi, pi, mesh.error()));
      }
      if (*mesh) {
        scene.meshes.push_back(std::move(**mesh));
        ctx.mesh_map[mi].push_back(scene.meshes.size() - 1);
      }
    }
  }

  // --- Node tree: roots of the default scene --------------------------------
  // 1) Collect the root nodes once, so both the model tree and the skeleton
  //    are built from exactly the same roots.
  const cgltf_scene *gs =
      data->scene ? data->scene
                  : (data->scenes_count ? &data->scenes[0] : nullptr);

  std::vector<const cgltf_node *> root_nodes;
  if (gs) {
    for (cgltf_size i = 0; i < gs->nodes_count; ++i) {
      root_nodes.push_back(gs->nodes[i]);
    }
  } else {
    for (cgltf_size i = 0; i < data->nodes_count; ++i) {
      if (!data->nodes[i].parent) {
        root_nodes.push_back(&data->nodes[i]);
      }
    }
  }

  // 2) Model tree: same result as before.
  scene.roots.reserve(root_nodes.size());
  for (const auto *node : root_nodes) {
    scene.roots.push_back(convert_node(*node, ctx));
  }

  // 3) Animations: only if the file has a skin and clips.
  if (data->skins_count > 0 && data->animations_count > 0) {
    const auto &skin = data->skins[0];
    if (skin.joints_count > max_animation_bones) {
      return std::unexpected(
          std::format("{}: skin has {} joints, shader supports 100", path_str,
                      skin.joints_count));
    }

    // name -> {id, inverse bind matrix}, ids in glTF joint order
    auto bone_infos = build_bone_infos(skin, ctx);

    // the whole node hierarchy, under one root
    animation::Skeleton skeleton("__gltf_root__", glm::mat4(1.0f));
    for (const auto *node : root_nodes) {
      skeleton.add_child(build_skeleton(*node, ctx));
    }

    // one GltfAnimation per clip, each with its own copy of skeleton + bone
    // infos
    for (cgltf_size a = 0; a < data->animations_count; ++a) {
      scene.animations.push_back(
          load_animation(data->animations[a], a, ctx, skeleton, bone_infos));
    }
  }

  return scene;
}

namespace {

// indexed like GltfScene::meshes; exactly one of mesh / animated is set
struct UploadedMesh {
  std::optional<MeshRef> mesh;
  std::optional<AnimatedMeshRef> animated;
  TextureRef texture;
};

// `uploaded` is indexed like GltfScene::meshes.
auto convert_node(const GltfModel &model,
                  const std::vector<UploadedMesh> &uploaded) -> ModelTreeNode {
  ModelTreeNode node;
  node.name = model.name;
  node.transform = model.transform;

  for (const auto index : model.meshes) {
    const auto &u = uploaded[index];
    if (u.animated)
      node.skinned_meshes.push_back({*u.animated, u.texture});
    else
      node.meshes.push_back({*u.mesh, u.texture});
  }

  node.children.reserve(model.children.size());
  for (const auto &child : model.children) {
    node.children.push_back(convert_node(child, uploaded));
  }
  return node;
}

auto draw_node(const ModelTreeNode &node, glm::mat4 parent) -> void {
  const glm::mat4 world = parent * node.transform;

  for (const auto &m : node.meshes) {
    Renderer::draw_textured(world, m.mesh, m.texture);
  }
  for (const auto &child : node.children) {
    draw_node(child, world);
  }
}

} // namespace

auto create_model_tree(GltfScene &scene)
    -> std::expected<ModelTree, std::string> {
  ModelTree tree;

  auto fail = [&tree](std::string message) -> std::unexpected<std::string> {
    destroy_model_tree(tree); // release whatever was already uploaded
    return std::unexpected(std::move(message));
  };

  // --- Textures: one per bitmap (indexed like GltfScene::bitmaps) ----------
  tree.unique_textures.reserve(scene.bitmaps.size());
  for (std::size_t i = 0; i < scene.bitmaps.size(); ++i) {
    auto texture = Resources::create_texture(scene.bitmaps[i]);
    if (!texture) {
      return fail(std::format("failed to create texture for bitmap {}", i));
    }
    tree.unique_textures.push_back(*texture);
  }

  // --- Meshes: one per GltfMesh (indexed like GltfScene::meshes) -----------
  std::vector<UploadedMesh> uploaded;
  uploaded.reserve(scene.meshes.size());

  for (std::size_t i = 0; i < scene.meshes.size(); ++i) {
    const auto &mesh = scene.meshes[i];

    TextureRef texture = Resources::default_texture();
    if (mesh.material) {
      if (*mesh.material >= scene.materials.size()) {
        return fail(std::format("mesh {}: material index {} out of range", i,
                                *mesh.material));
      }

      if (const auto diffuse = scene.materials[*mesh.material].diffuse) {
        if (*diffuse >= tree.unique_textures.size()) {
          return fail(std::format("mesh {}: bitmap index {} out of range", i,
                                  *diffuse));
        }
        texture = tree.unique_textures[*diffuse];
      }
    }

    if (!mesh.bone_vertices.empty()) {
      auto ref = Resources::create_animated_mesh(
          mesh.vertices, mesh.bone_vertices, mesh.indices);
      if (!ref) {
        return fail(std::format("failed to create animated mesh {}", i));
      }
      tree.unique_animated_meshes.push_back(*ref);
      uploaded.push_back({std::nullopt, *ref, texture});
    } else {
      auto ref = Resources::create_mesh(mesh.vertices, mesh.indices);
      if (!ref) {
        return fail(std::format("failed to create mesh {}", i));
      }
      tree.unique_meshes.push_back(*ref);
      uploaded.push_back({*ref, std::nullopt, texture});
    }
  }

  // --- Node tree: indices resolved to handles -------------------------------
  tree.roots.reserve(scene.roots.size());
  for (const auto &root : scene.roots) {
    tree.roots.push_back(convert_node(root, uploaded));
  }

  return tree;
}

auto destroy_model_tree(ModelTree &tree) -> void {
  for (const auto &mesh : tree.unique_meshes) {
    Resources::destroy_mesh(mesh);
  }
  for (const auto &mesh : tree.unique_animated_meshes) {
    Resources::destroy_animated_mesh(mesh);
  }
  for (const auto &texture : tree.unique_textures) {
    Resources::destroy_texture(texture);
  }
  tree = {};
}

auto draw_model_tree(const ModelTree &tree, glm::mat4 transform) -> void {
  for (const auto &root : tree.roots) {
    draw_node(root, transform);
  }
}

auto load_model(std::filesystem::path path)
    -> std::expected<Model, std::string> {
  auto scene = load_gltf_scene(path);
  if (!scene.has_value()) {
    return std::unexpected(scene.error());
  }

  auto tree = create_model_tree(scene.value());
  if (!tree.has_value()) {
    return std::unexpected(tree.error());
  }

  auto model = Model{};
  model.scene = std::move(scene.value());
  model.tree = std::move(tree.value());
  return model;
}

Animator::Animator(const Model &model) : _model(&model) {
  const auto &clips = model.scene.animations;
  if (!clips.empty()) // identity joint matrices = bind pose
    _bones.assign(clips[0].animation.bone_infos().bone_count(),
                  glm::mat4(1.0f));
}

auto Animator::play(std::string_view name, PlayMode mode, bool restart)
    -> bool {
  const auto &clips = _model->scene.animations;
  auto it = std::ranges::find(clips, name, &GltfAnimation::name);
  if (it == clips.end())
    return false;

  const auto index = static_cast<std::size_t>(it - clips.begin());
  if (_clip == index && !restart) {
    _mode = mode;
    return true;
  } // safe to call every frame
  _clip = index;
  _mode = mode;
  _time = 0.0;
  _finished = false;
  return true;
}

auto Animator::update(float dt) -> void {
  if (!_clip)
    return;
  const auto &clip = _model->scene.animations[*_clip].animation;
  const double duration =
      clip.total_ticks().get() / clip.ticks_per_second().get();
  if (duration <= 0.0)
    return;

  _time += dt;
  if (_mode == PlayMode::Loop) {
    _time = std::fmod(_time, duration);
  } else if (_time >= duration) {
    _time = duration;
    _finished = true;
  }

  // Bone::*_index() uses `time < next.timeStamp`, so sampling exactly at the
  // end fails
  const double sample = std::min(_time, std::nextafter(duration, 0.0));
  auto pose = clip.animate(sample);
  if (!pose)
    return; // keep the last good pose
  auto state =
      animation::calculate_final_animation_state(clip.bone_infos(), *pose);
  if (!state)
    return;
  _bones.assign(state->matrices().begin(), state->matrices().end());
}

auto Animator::finished() const -> bool {
  return _mode == PlayMode::Once && _finished;
}

auto Animator::bone_matrices() const -> std::span<const glm::mat4> {
  return _bones;
}

namespace {
auto draw_node(const ModelTreeNode &node, glm::mat4 parent, glm::mat4 transform,
               std::span<const glm::mat4> bones) -> void {
  const glm::mat4 world = parent * node.transform;
  for (const auto &m : node.meshes)
    Renderer::draw_textured(world, m.mesh, m.texture);
  for (const auto &m :
       node.skinned_meshes) // glTF: skinned mesh ignores its node transform
    Renderer::draw_animated(transform, m.mesh, m.texture, bones);
  for (const auto &child : node.children)
    draw_node(child, world, transform, bones);
}
} // namespace

auto draw_model(const Model &model, glm::mat4 transform) -> void {
  static const std::vector<glm::mat4> bind_pose(max_animation_bones,
                                                glm::mat4(1.0f));
  for (const auto &root : model.tree.roots)
    draw_node(root, transform, transform, bind_pose);
}

auto draw_model(const Model &model, glm::mat4 transform,
                const Animator &animator) -> void {
  for (const auto &root : model.tree.roots)
    draw_node(root, transform, transform, animator.bone_matrices());
}

auto unload_model(Model &model) -> void {
  //TODO: This is a very bad fix for deleting stuff that is still in use.
  ContextImpl::device().waitIdle();
  destroy_model_tree(model.tree);
  model = {};
}

} // namespace bubble
