/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/export.h>

struct ly_ctx;
struct lys_module;
struct lysc_ident;
struct lysc_ext;
struct lysc_ext_instance;
struct lysp_feature;
struct lysp_submodule;

namespace libyang {
class Context;
class DataNode;
class DataNodeTerm;
class Extension;
class ExtensionInstance;
class Meta;
class Module;
class ChildInstanstiables;
class Identity;
class SchemaNode;
class SubmoduleParsed;
template <typename NodeType, IterationType ITER_TYPE>
class Collection;

namespace types {
class IdentityRef;
}

/**
 * @brief Represents a feature of a module.
 *
 * Wraps `lysp_feature`.
 */
class LIBYANG_CPP_EXPORT Feature {
public:
    std::string name() const;
    bool isEnabled() const;

    friend Module;

private:
    Feature(const lysp_feature* feature, std::shared_ptr<ly_ctx> ctx);

    const lysp_feature* m_feature;
    std::shared_ptr<ly_ctx> m_ctx;
};

/**
 * @brief Tag for enabling all features (as if using "*" from libyang).
 *
 * See Module::setImplemented for more information.
 */

struct LIBYANG_CPP_EXPORT AllFeatures {
};

/**
 * @brief libyang module class.
 */
class LIBYANG_CPP_EXPORT Module {
public:
    std::string name() const;
    std::optional<std::string> revision() const;
    std::string ns() const;
    std::optional<std::string> org() const;
    bool implemented() const;
    bool featureEnabled(const std::string& featureName) const;
    std::vector<Feature> features() const;
    std::vector<ExtensionInstance> extensionInstances() const;
    ExtensionInstance extensionInstance(const std::string& name) const;
    void setImplemented();
    void setImplemented(std::vector<std::string> features);
    void setImplemented(const AllFeatures);

    std::vector<Identity> identities() const;

    std::optional<SchemaNode> child() const;
    ChildInstanstiables childInstantiables() const;
    libyang::Collection<SchemaNode, IterationType::Dfs> childrenDfs() const;
    Collection<SchemaNode, IterationType::Sibling> immediateChildren() const;
    std::vector<SchemaNode> actionRpcs() const;

    std::string printStr(const SchemaOutputFormat format, const std::optional<SchemaPrintFlags> flags = std::nullopt, std::optional<size_t> lineLength = std::nullopt) const;

    bool operator==(const Module& other) const;

    friend Context;
    friend DataNode;
    friend Extension;
    friend ExtensionInstance;
    friend Meta;
    friend Identity;
    friend SchemaNode;
    friend SubmoduleParsed;

private:
    Module(lys_module* module, std::shared_ptr<ly_ctx> ctx);

    std::shared_ptr<ly_ctx> m_ctx;
    lys_module* m_module;
};

/**
 * @brief libyang parsed submodule class
 *
 * Wraps `lysp_submodule`.
 * The submodule becames part of `lys_module` in libyang. We can only wrap `lysp_submodule` which represents parsed schema of the submodule.
 */
class LIBYANG_CPP_EXPORT SubmoduleParsed {
public:
    std::string name() const;
    Module module() const;

    std::string printStr(const SchemaOutputFormat format, const std::optional<SchemaPrintFlags> flags = std::nullopt, std::optional<size_t> lineLength = std::nullopt) const;

    friend Context;

private:
    SubmoduleParsed(const lysp_submodule* submodule, std::shared_ptr<ly_ctx> ctx);

    std::shared_ptr<ly_ctx> m_ctx;
    const lysp_submodule* m_submodule;
};

/**
 * @brief Contains information about an identity.
 *
 * Wraps `lysc_ident`.
 */
class LIBYANG_CPP_EXPORT Identity {
public:
    friend DataNodeTerm;
    friend Module;
    friend types::IdentityRef;
    std::vector<Identity> derived() const;
    std::vector<Identity> derivedRecursive() const;
    Module module() const;
    std::string name() const;

    bool operator==(const Identity& other) const;

private:
    Identity(const lysc_ident* ident, std::shared_ptr<ly_ctx> ctx);

    const lysc_ident* m_ident;
    std::shared_ptr<ly_ctx> m_ctx;
};

/**
 * @brief Contains information about compiled extension.
 *
 * Wraps `lysc_ext_instance`
 */
class LIBYANG_CPP_EXPORT ExtensionInstance {
public:
    Module module() const;
    Extension definition() const;
    std::optional<std::string> argument() const;
    std::vector<ExtensionInstance> extensionInstances() const;

private:
    ExtensionInstance(const lysc_ext_instance* instance, std::shared_ptr<ly_ctx> ctx);

    const lysc_ext_instance* m_instance;
    std::shared_ptr<ly_ctx> m_ctx;

    friend Module;
    friend Context;
    friend DataNode;
    friend Extension;
    friend SchemaNode;
};

/**
 * @brief Contains information about an extension definition.
 *
 * Wraps `lysc_ext`
 */
class LIBYANG_CPP_EXPORT Extension {
public:
    Module module() const;
    std::string name() const;
    std::vector<ExtensionInstance> extensionInstances() const;

private:
    Extension(const lysc_ext* def, std::shared_ptr<ly_ctx> ctx);

    const lysc_ext* m_ext;
    std::shared_ptr<ly_ctx> m_ctx;

    friend ExtensionInstance;
};
}
