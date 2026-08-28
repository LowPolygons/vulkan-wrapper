#include "vulkan_wrapper/implementation_helpers/implementation_helpers.hh"

auto ImplementationHelp::FragApp::get_frag_app_indices()
    -> std::vector<uint16_t> {
  return std::vector<std::uint16_t>{{0, 1, 2, 2, 3, 0}};
}

auto ImplementationHelp::FragApp::get_frag_app_vertices()
    -> std::vector<Vertex> {
  return std::vector<Vertex>{
      {{-1.0, -1.0}}, {{1.0, -1.0}}, {{1.0, 1.0}}, {{-1.0, 1.0}}};
}
