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
#include <string_view>
#include <vector>
#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/export.h>

struct ly_ctx;
struct lys_module;
struct lysc_ident;
struct lysc_ext;
struct lysc_ext_instance;
struct lysp_feature;
struct lysp_tpdf;

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

namespace types {
class IdentityRef;
class Type;
}

/**
 * @brief Represents a feature of a module.
 *
 * Wraps `lysp_feature`.
 */
class LIBYANG_CPP_EXPORT Feature {
public:
    std::string_view name() const;
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
 * @brief A `typedef` statement in the schema
 */
class LIBYANG_CPP_EXPORT Typedef {
public:
    std::string name() const;
    std::optional<std::string> description() const;
    std::optional<std::string> reference() const;
    std::optional<std::string> units() const;
    types::Type type() const;

    friend Module;

private:
    Typedef(const lysp_tpdf* tpdf, std::shared_ptr<ly_ctx> ctx);

    const lysp_tpdf* m_tpdf;
    std::shared_ptr<ly_ctx> m_ctx;
};

/**
 * @brief libyang module class.
 */
class LIBYANG_CPP_EXPORT Module {
public:
    std::string_view name() const;
    std::optional<std::string_view> revision() const;
    std::string_view ns() const;
    bool implemented() const;
    bool featureEnabled(const std::string& featureName) const;
    std::vector<Feature> features() const;
    std::vector<ExtensionInstance> extensionInstances() const;
    ExtensionInstance extensionInstance(const std::string& name) const;
    void setImplemented();
    void setImplemented(std::vector<std::string> features);
    void setImplemented(const AllFeatures);

    std::vector<Identity> identities() const;

    ChildInstanstiables childInstantiables() const;

    std::string printStr(const SchemaOutputFormat format, const std::optional<SchemaPrintFlags> flags = std::nullopt, std::optional<size_t> lineLength = std::nullopt) const;

    std::vector<Typedef> typedefs() const;

    friend Context;
    friend DataNode;
    friend Meta;
    friend Identity;
    friend SchemaNode;

private:
    Module(lys_module* module, std::shared_ptr<ly_ctx> ctx);

    std::shared_ptr<ly_ctx> m_ctx;
    lys_module* m_module;
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
    std::string_view name() const;

    bool operator==(const Identity& other) const;

private:
    Identity(const lysc_ident* ident, std::shared_ptr<ly_ctx> ctx);

    const lysc_ident* m_ident;
    std::shared_ptr<ly_ctx> m_ctx;
};

/**
 * @brief Contains information about compiled extension.
 *
 * Wraps `lysc_extension_instance`
 */
class LIBYANG_CPP_EXPORT ExtensionInstance {
public:
    Extension definition() const;
    std::string_view argument() const;

private:
    ExtensionInstance(const lysc_ext_instance* ext, std::shared_ptr<ly_ctx> ctx);

    const lysc_ext_instance* m_ext;
    std::shared_ptr<ly_ctx> m_ctx;

    friend Module;
    friend Context;
    friend DataNode;
};

/**
 * @brief Contains information about an extension definition.
 *
 * Wraps `lysc_ext`
 */
class LIBYANG_CPP_EXPORT Extension {
public:
    std::string_view name() const;

private:
    Extension(const lysc_ext* def, std::shared_ptr<ly_ctx> ctx);

    const lysc_ext* m_ext;
    std::shared_ptr<ly_ctx> m_ctx;

    friend ExtensionInstance;
};
}
