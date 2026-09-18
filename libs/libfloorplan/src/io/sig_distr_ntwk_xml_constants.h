#pragma once

/* Constants required by XML parser */

namespace arkangel {  // Begin namespace arkangel

constexpr const char* XML_SIG_DISTR_NTWK_ROOT_NODE_NAME = "signal_distribution_network";
constexpr const char* XML_SIG_DISTR_NTWK_NODE_NTWK = "network";
constexpr const char* XML_SIG_DISTR_NTWK_NTWK_ATTR_PORT = "port";
constexpr const char* XML_SIG_DISTR_NTWK_NTWK_NODE_BUF = "buf";
constexpr const char* XML_SIG_DISTR_NTWK_NTWK_BUF_ATTR_NAME = "name";
constexpr const char* XML_SIG_DISTR_NTWK_NTWK_BUF_ATTR_CELL = "cell";
constexpr const char* XML_SIG_DISTR_NTWK_NTWK_BUF_ATTR_X = "x";
constexpr const char* XML_SIG_DISTR_NTWK_NTWK_BUF_ATTR_Y = "y";
constexpr const char* XML_SIG_DISTR_NTWK_NTWK_BUF_ATTR_PARENT = "parent";
constexpr const char* XML_SIG_DISTR_NTWK_NTWK_BUF_NODE_CHILD = "child";
constexpr const char* XML_SIG_DISTR_NTWK_NTWK_BUF_CHILD_ATTR_NAME = "name";
constexpr const char* XML_SIG_DISTR_NTWK_NTWK_BUF_NODE_LEAF_TILE = "leaf_tile";
constexpr const char* XML_SIG_DISTR_NTWK_NTWK_BUF_LEAF_TILE_ATTR_X = "x";
constexpr const char* XML_SIG_DISTR_NTWK_NTWK_BUF_LEAF_TILE_ATTR_Y = "y";

}  // End of namespace arkangel
